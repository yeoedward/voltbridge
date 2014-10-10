#ifdef __cplusplus
extern "C" {
#endif

typedef struct duck duck;

duck* new_duck(int feet);
void delete_duck(duck* d);
void duck_quack(duck* d, float volume);

#ifdef __cplusplus
}
#endif
