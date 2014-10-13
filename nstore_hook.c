#include "postgres.h"

#include "executor/executor.h"
#include "nodes/print.h"
#include "voltdb_bridge.hpp"

#include <stdio.h>

PG_MODULE_MAGIC;

/* Saved hook values in case of unload */
static ExecutorStart_hook_type prev_ExecutorStart = NULL;
static ExecutorRun_hook_type prev_ExecutorRun = NULL;
static ExecutorFinish_hook_type prev_ExecutorFinish = NULL;
static ExecutorEnd_hook_type prev_ExecutorEnd = NULL;

void		_PG_init(void);
void		_PG_fini(void);

static void nstore_ExecutorStart(QueryDesc *queryDesc, int eflags);
static void nstore_ExecutorRun(
        QueryDesc *queryDesc,
        ScanDirection direction,
        long count);
static void nstore_ExecutorFinish(QueryDesc *queryDesc);
static void nstore_ExecutorEnd(QueryDesc *queryDesc);

static ee_t *ee;

void
_PG_init(void)
{
	/* Install hooks. */
	prev_ExecutorStart = ExecutorStart_hook;
	ExecutorStart_hook = nstore_ExecutorStart;
	prev_ExecutorRun = ExecutorRun_hook;
	ExecutorRun_hook = nstore_ExecutorRun;
	prev_ExecutorFinish = ExecutorFinish_hook;
	ExecutorFinish_hook = nstore_ExecutorFinish;
	prev_ExecutorEnd = ExecutorEnd_hook;
	ExecutorEnd_hook = nstore_ExecutorEnd;
    printf("Intalling nstore hook.\n");
    ee = new_ee();
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	/* Uninstall hooks. */
	ExecutorStart_hook = prev_ExecutorStart;
	ExecutorRun_hook = prev_ExecutorRun;
	ExecutorFinish_hook = prev_ExecutorFinish;
	ExecutorEnd_hook = prev_ExecutorEnd;
    //printf("Uninstalling nstore hook.");
    delete_ee(ee);
}

//TODO Currently segfaulting if we don't run the standard executor functions.

/*
 * ExecutorStart hook: start up logging if needed
 */
static void
nstore_ExecutorStart(QueryDesc *queryDesc, int eflags)
{
    //pprint(queryDesc->plannedstmt);
    standard_ExecutorStart(queryDesc, eflags);
}

/*
 * ExecutorRun hook: all we need do is track nesting depth
 */
static void
nstore_ExecutorRun(QueryDesc *queryDesc, ScanDirection direction, long count)
{
    // Plan is typedef'd to an int and is ignored atm.
    plan_t plan = 0;
    ee_execute_plan(ee, &plan);
    standard_ExecutorRun(queryDesc, direction, count);
}

/*
 * ExecutorFinish hook: all we need do is track nesting depth
 */
static void
nstore_ExecutorFinish(QueryDesc *queryDesc)
{
    standard_ExecutorFinish(queryDesc);
}

/*
 * ExecutorEnd hook: log results if needed
 */
static void
nstore_ExecutorEnd(QueryDesc *queryDesc)
{
    standard_ExecutorEnd(queryDesc);    
}
