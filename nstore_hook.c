#include "postgres.h"

#include "executor/executor.h"
#include "nodes/print.h"
#include "voltdb_bridge.hpp"

PG_MODULE_MAGIC;

/* Saved hook values in case of unload */
static ExecutorStart_hook_type prev_ExecutorStart = NULL;
static ExecutorRun_hook_type prev_ExecutorRun = NULL;
static ExecutorFinish_hook_type prev_ExecutorFinish = NULL;
static ExecutorEnd_hook_type prev_ExecutorEnd = NULL;

void		_PG_init(void);
void		_PG_fini(void);

static void nstore_ExecutorStart(QueryDesc *queryDesc, int eflags);
static void nstore_ExecutorRun(QueryDesc *queryDesc,
        ScanDirection direction,
        long count);
static void nstore_ExecutorFinish(QueryDesc *queryDesc);
static void nstore_ExecutorEnd(QueryDesc *queryDesc);

static duck *d;

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
    d = new_duck(2);
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
    delete_duck(d);
}

/*
 * ExecutorStart hook: start up logging if needed
 */
static void
nstore_ExecutorStart(QueryDesc *queryDesc, int eflags)
{
	if (prev_ExecutorStart)
		prev_ExecutorStart(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);
    pprint(queryDesc->plannedstmt);
}

/*
 * ExecutorRun hook: all we need do is track nesting depth
 */
static void
nstore_ExecutorRun(QueryDesc *queryDesc, ScanDirection direction, long count)
{
    if (prev_ExecutorRun)
        prev_ExecutorRun(queryDesc, direction, count);
    else
        standard_ExecutorRun(queryDesc, direction, count);
    duck_quack(d, 1000);
}

/*
 * ExecutorFinish hook: all we need do is track nesting depth
 */
static void
nstore_ExecutorFinish(QueryDesc *queryDesc)
{
    if (prev_ExecutorFinish)
        prev_ExecutorFinish(queryDesc);
    else
        standard_ExecutorFinish(queryDesc);
}

/*
 * ExecutorEnd hook: log results if needed
 */
static void
nstore_ExecutorEnd(QueryDesc *queryDesc)
{
	if (prev_ExecutorEnd)
		prev_ExecutorEnd(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}
