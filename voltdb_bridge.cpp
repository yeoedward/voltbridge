#include "voltdb_bridge.hpp"
#include "hardcoded.h"

#include <execution/VoltDBEngine.h>
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

VoltDB::VoltDB() {
    // We might want to rewrite initialize so we don't need
    // all these dummy values.
    m_engine.initialize(0, 0, 0, 0, "eddy", 10);
    m_engine.loadCatalog(0, getCatalogString());
}

VoltDB::~VoltDB() {
    // Do nothing.
}

// Has to be a fixed size buffer? What happens if too big?
// There's some fallback buffer stuff in voltdb.
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
    m_engine.executeSerializedPlan(1, 0, 0, 0, 0, 0, false, false, getPlan());

    // Print out result for checking.
    // 58 is a known size.
    for (int i = 0; i < 58; i++) {
        cout << resultBuffer[i] << ",";
    }
    cout << endl;
}
