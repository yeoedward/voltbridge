#include "voltdb_bridge.hpp"

// Hardcoded strings we are feeding VoltDB atm.
#include "hardcoded.h"

// VoltDB includes.
#include <common/serializeio.h>
#include <execution/VoltDBEngine.h>
#include <storage/table.h>

// Postgres includes.
extern "C" {
#include <postgres.h>
#include <access/htup_details.h>
#include <access/tupdesc.h>
#include <catalog/pg_type.h>
#include <executor/execdesc.h>
#include <executor/tuptable.h>
#include <nodes/print.h>
}

// Stdlib includes.
#include <iostream>
#include <string>

using namespace std;

struct ee_t {};

class VoltDB : public ee_t {
    public:
        VoltDB();
        ~VoltDB();
        void executePlan(QueryDesc *queryDesc);
    private:
        voltdb::VoltDBEngine m_engine;
        void sendBuffer(
                const char *raw_buffer,
                bool sendTuples,
                DestReceiver *dest);
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
    m_engine.initialize(0, 0, 0, 0, "", 10);
    m_engine.loadCatalog(0, getCatalogString());
}

VoltDB::~VoltDB() {
    // Do nothing.
}

// Has to be a fixed size buffer? What happens if too big?
// There's some fallback buffer stuff in voltdb.
// Allocating on the stack for now until we figure out the memory
// context stuff in postgres.
#define BUFSIZE 4096
void VoltDB::executePlan(QueryDesc *queryDesc) {
    // Ignore postgres plan for now.

    // Create buffers for voltdb to pass results back in.
    char parameterBuffer[BUFSIZE] = { 0 };
    char resultBuffer[BUFSIZE] = { 0 };
    char exceptionBuffer[BUFSIZE] = { 0 };
    m_engine.setBuffers(
        parameterBuffer, BUFSIZE,
        resultBuffer, BUFSIZE,
        exceptionBuffer, BUFSIZE);

    m_engine.resetReusedResultOutputBuffer();
    
    // Rewrite so that it doesn't need all these default values.
    voltdb::Table *table =
        m_engine.executeSerializedPlan(1, 0, 0, 0, 0, 0, true, true, getPlan());
    (void) table;

    // Print out result for checking.
    // 58 is a known size.
    for (int i = 0; i < 58; i++) {
        cout << ((int) resultBuffer[i]) << ",";
    }
    cout << endl << flush;

    CmdType operation = queryDesc->operation;
    DestReceiver *dest = queryDesc->dest;
    bool sendTuples = (queryDesc->operation == CMD_SELECT ||
            queryDesc->plannedstmt->hasReturning);

    // Startup dest.
    if (sendTuples)
        (*dest->rStartup) (dest, operation, queryDesc->tupDesc);

    // sendBuffer will send slots to dest.
    sendBuffer(resultBuffer, sendTuples, queryDesc->dest);

    // Shutdown dest.
    if (sendTuples)
        (*dest->rShutdown) (dest);

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
    int attdim = -1;
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
