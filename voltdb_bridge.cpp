#include "voltdb_bridge.hpp"

// Hardcoded strings we are feeding VoltDB atm.
#include "hardcoded.h"

// VoltDB includes.
#include <common/serializeio.h>
#include <execution/VoltDBEngine.h>

// Postgres includes.
extern "C" {
#include <postgres.h>
#include <access/tupdesc.h>
#include <catalog/pg_type.h>
#include <executor/tuptable.h>
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
        void executePlan(plan_t *plan);
    private:
        voltdb::VoltDBEngine m_engine;
        void sendBuffer(const char *raw_buffer);
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

void ee_execute_plan(ee_t *ee, plan_t *plan) {
    real(ee)->executePlan(plan);
}

/*
 * VoltDB bridging code.
 */

VoltDB::VoltDB() : m_engine(NULL, NULL) {
    // TODO rewrite initialize so we don't need
    // all these dummy values.
    m_engine.initialize(0, 0, 0, 0, "eddy", 10);
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
void VoltDB::executePlan(plan_t *pg_plan) {
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
    m_engine.executeSerializedPlan(1, 0, 0, 0, 0, 0, true, true, getPlan());

    // Print out result for checking.
    // 58 is a known size.
    for (int i = 0; i < 58; i++) {
        cout << ((int) resultBuffer[i]) << ",";
    }
    cout << endl << flush;
    sendBuffer(resultBuffer);
}

void VoltDB::sendBuffer(const char *raw_buffer) {
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
    const char *serializedTable = buffer.getRawPointer(tableSize);
    (void) serializedTable;
    //TODO Make sure that the slot is allocated in the right pg memory context.
    int natts = 1;
    TupleDesc tupdesc = CreateTemplateTupleDesc(natts, false);
    int varchar_len = 100;
    // 0 because varchar is not an array type.
    int attdim = 0;
    TupleDescInitEntry(tupdesc,
            1,
            "First name",
            VARCHAROID,
            varchar_len,
            attdim);
    TupleDescInitEntry(tupdesc,
            2,
            "Last name",
            VARCHAROID,
            varchar_len,
            attdim);
    TupleTableSlot *slot = MakeSingleTupleTableSlot(tupdesc);
    ExecSetSlotDescriptor(slot, tupdesc);
    //TODO Create heaptuple using serializedTable and add it to the slot.
    // Look at the seqscan case in the executor.
    //TODO Send the slot off to dest. Need to comment out dest stuff in main pg executor.
}
