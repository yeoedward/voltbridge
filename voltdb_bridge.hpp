#ifdef __cplusplus
extern "C" {
#endif

typedef struct ee_t ee_t;
typedef int plan_t;

ee_t *new_ee();
void delete_ee(ee_t *ee);
void ee_execute_plan(ee_t *ee, plan_t *plan);

#ifdef __cplusplus
}
#endif
