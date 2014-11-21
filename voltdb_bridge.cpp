#include "voltdb_bridge.hpp"

// Hardcoded strings we are feeding VoltDB atm.
#include "hardcoded.h"

// VoltDB includes.
#include <common/NValue.hpp>
#include <common/serializeio.h>
#include <common/tabletuple.h>
#include <common/TupleSchema.h>
#include <common/types.h>
#include <common/valuevector.h>
#include <execution/VoltDBEngine.h>
#include <stdint.h>
#include <storage/table.h>
#include <storage/tableiterator.h>

// Postgres includes.
extern "C" {
#include <postgres.h>
#include <access/htup_details.h>
#include <access/tupdesc.h>
#include <catalog/pg_type.h>
#include <executor/execdesc.h>
#include <executor/tuptable.h>
#include <nodes/print.h>
#include <utils/builtins.h>
}

// Stdlib includes.
#include <iostream>
#include <string>

#define BUFSIZE 4096

using namespace std;

struct ee_t {};

class VoltDB : public ee_t {
    public:
        VoltDB();
        ~VoltDB();
        void executePlan(QueryDesc *queryDesc);
    private:
        void sendTupleToPG(
                QueryDesc *queryDesc,
                const voltdb::TupleSchema *schema,
                voltdb::TableTuple& tuple);

        void sendBuffer(
                const char *raw_buffer,
                bool sendTuples,
                DestReceiver *dest);

        voltdb::VoltDBEngine m_engine;

        // Create buffers for voltdb to pass results back in.
        char parameterBuffer[BUFSIZE];
        char resultBuffer[BUFSIZE];
        char exceptionBuffer[BUFSIZE];
};

/*
 * C functions.
 */

inline VoltDB* real(ee_t *ee) {
    return static_cast<VoltDB *>(ee);
}

ee_t *new_ee() {
    return new VoltDB();
}

void delete_ee(ee_t *ee) {
    delete real(ee);
}

void ee_execute_plan(ee_t *ee, QueryDesc *queryDesc) {
    real(ee)->executePlan(queryDesc);
}

/*
 * VoltDB bridging code.
 */

VoltDB::VoltDB() : m_engine(NULL, NULL) {
    // TODO rewrite initialize so we don't need
    // all these dummy values.
    m_engine.initialize(0, 0, 0, 0, "", 10 * 1024 * 1024);
    m_engine.loadCatalog(0, getCatalogString());
    m_engine.setBuffers(
        parameterBuffer, BUFSIZE,
        resultBuffer, BUFSIZE,
        exceptionBuffer, BUFSIZE);
    m_engine.resetReusedResultOutputBuffer();
    voltdb::NValueArray params(2);
    int first_name = 1;
    params[0] = voltdb::NValue::initFromTupleStorage(&first_name, voltdb::VALUE_TYPE_INTEGER, true);
    const char *second_name = "\3lol";
    params[1] = voltdb::NValue::initFromTupleStorage(second_name, voltdb::VALUE_TYPE_VARCHAR, true);
    m_engine.setStaticParams(params);
    voltdb::Table *table = m_engine.executeSerializedPlan(1, 0, 0, 0, 0, 0, true, true, getInsertPlan());
    assert(table);

    // Second
    first_name = 2;
    params[0] = voltdb::NValue::initFromTupleStorage(&first_name, voltdb::VALUE_TYPE_INTEGER, true);
    second_name = "\x4haha";
    params[1] = voltdb::NValue::initFromTupleStorage(second_name, voltdb::VALUE_TYPE_VARCHAR, true);
    m_engine.setStaticParams(params);

    table = m_engine.executeSerializedPlan(1, 0, 0, 0, 0, 0, true, true, getInsertPlan());
    assert(table);
    //TODO This seg faults because the receive executor makes a call to TopEnd.
    //assert(m_engine.executeSerializedPlan(1, 0, 0, 0, 0, 0, true, true, getPostInsertPlan()));
}

VoltDB::~VoltDB() {
    // Do nothing.
}

void VoltDB::executePlan(QueryDesc *queryDesc) {
    // Ignore postgres plan for now.
    
    // Rewrite so that it doesn't need all these default values.
    //TODO Review memory management. I'm guessing we own this table?
    m_engine.resetReusedResultOutputBuffer();
    voltdb::Table *table =
        m_engine.executeSerializedPlan(1, 0, 0, 0, 0, 0, true, true, getSelectPlan());
    voltdb::TableIterator& it = table->iterator();
    voltdb::TableTuple tuple(table->schema());

    CmdType operation = queryDesc->operation;
    DestReceiver *dest = queryDesc->dest;
    bool sendTuples = (queryDesc->operation == CMD_SELECT ||
            queryDesc->plannedstmt->hasReturning);

    if (sendTuples) {
        // Startup dest.
        (*dest->rStartup) (dest, operation, queryDesc->tupDesc);

        while (it.next(tuple)) {
            sendTupleToPG(queryDesc, table->schema(), tuple);
        }

        // Shutdown dest.
        (*dest->rShutdown) (dest);
    }
    /*

    // sendBuffer will send slots to dest.
    sendBuffer(resultBuffer, sendTuples, queryDesc->dest);

     */
}

void VoltDB::sendTupleToPG(
        QueryDesc *queryDesc,
        const voltdb::TupleSchema *schema,
        voltdb::TableTuple& tuple) {
    int tupleSize = tuple.sizeInValues();
    Datum values[tupleSize];
    bool isnull[tupleSize];
    memset(isnull, 0, tupleSize * sizeof (bool));
    for (int i = 0; i < tupleSize; i++) {
        voltdb::NValue value = tuple.getNValue(i);
        switch (schema->columnType(i)) {
            case voltdb::VALUE_TYPE_INTEGER:
                values[i] = Int32GetDatum(value.getInteger());
                break;
            case voltdb::VALUE_TYPE_VARCHAR:
            {
                const char *str = reinterpret_cast<const char *>(value.getObjectValue());
                if (str == NULL) {
                    isnull[i] = true;
                } else {
                    int32_t len = value.getObjectLength_withoutNull();
                    values[i] = PointerGetDatum(cstring_to_text_with_len(str, len));
                }
                break;
            }
            default:
                cout << "Invalid column type to send to PG." << endl << flush;
                assert(0);
        }
    }
    TupleTableSlot *slot = MakeSingleTupleTableSlot(queryDesc->tupDesc);
    ExecSetSlotDescriptor(slot, queryDesc->tupDesc);
    HeapTuple heapTuple = heap_form_tuple(queryDesc->tupDesc, values, isnull);
    ExecStoreTuple(heapTuple, slot, InvalidBuffer, true);
    (queryDesc->dest->receiveSlot) (slot, queryDesc->dest);
}

//TODO Change to send table.
void VoltDB::sendBuffer(
        const char *raw_buffer,
        bool sendTuples,
        DestReceiver *dest) {
    // TODO Check if fallback buffer was used?
    voltdb::ReferenceSerializeInputBE buffer(raw_buffer, BUFSIZE);
    int totalSize = buffer.readInt();
    bool dirty = buffer.readBool();
    int numdeps = buffer.readInt();
    int depid = buffer.readInt();
    int tableSize = buffer.readInt();
    cout << "totalSize = " << totalSize << ", "
         << "dirty = " << dirty << ", "
         << "numdeps = " << numdeps << ", "
         << "depid = " << depid << ", "
         << "tableSize = " << tableSize << endl << flush;

    // Print out result for checking.
    // 58 is a known size.
    for (int i = 0; i < tableSize; i++) {
        cout << resultBuffer[i] << ",";
    }
    cout << endl << flush;

    /*
     * Deserializing logic from VoltTable.java.
     * TODO Use voltdb::Table instead. No need to deserialize buffer.
     */

    // rowstart represents offset to the start of row data, but non-inclusive
    // of the length of the header, so add 4 bytes.
    int rowStart = buffer.readInt() + 4;
    buffer.setPosition(5);
    short int colCount = buffer.readShort();
    buffer.setPosition(rowStart);
    int rowCount = buffer.readInt();
    (void) colCount;
    (void) rowCount;

    //TODO Make sure that the slot is allocated in the right pg memory context.
    int natts = 2;
    TupleDesc tupdesc = CreateTemplateTupleDesc(natts, false);
    int typmod = -1;
    // 0 because varchar is not an array type.
    int attdim = 0;
    TupleDescInitEntry(tupdesc,
            1,
            "First name",
            CSTRINGOID,
            typmod,
            attdim);
    TupleDescInitEntry(tupdesc,
            2,
            "Last name",
            CSTRINGOID,
            typmod,
            attdim);
    TupleTableSlot *slot = MakeSingleTupleTableSlot(tupdesc);
    ExecSetSlotDescriptor(slot, tupdesc);
    Datum values[] = {CStringGetDatum("Justin"), CStringGetDatum("Bieber")};
    bool isnull[] = {false, false};
    HeapTuple tuple = heap_form_tuple(tupdesc, values, isnull);
    ExecStoreTuple(tuple, slot, InvalidBuffer, true);
    if (sendTuples)
        (*dest->receiveSlot) (slot, dest);
}
