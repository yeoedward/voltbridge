#ifndef _VOLTDB_BRIDGE_HPP_
#define _VOLTDB_BRIDGE_HPP_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ee_t ee_t;
struct QueryDesc;
typedef struct QueryDesc QueryDesc;

ee_t *new_ee();
void delete_ee(ee_t *ee);
void ee_execute_plan(ee_t *ee, QueryDesc *queryDesc);

#ifdef __cplusplus
}
#endif

#endif /* _VOLTDB_BRIDGE_HPP_ */
