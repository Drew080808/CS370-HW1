#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "rbtree.h"

#define MAX_KEYS 500
#define VALIDATE_EVERY 100

typedef struct {
    int present;
    int value;
} ref_entry_t;

static void free_int(void *value) {
    free(value);
}

int main(int argc, char **argv) {
    long op_count = (argc >= 2) ? strtol(argv[1], NULL, 10) : 100000;
    unsigned seed = (argc >= 3) ? (unsigned)strtoul(argv[2], NULL, 10) : 12345u;
    printf("fuzz: %ld ops, seed=%u\n", op_count, seed);
    srand(seed);

    static ref_entry_t ref[MAX_KEYS];
    long ref_size = 0;

    rbtree_t *t = rb_create(free_int);
    assert(t != NULL);

    for (long i = 0; i < op_count; i++) {
        int id = rand() % MAX_KEYS;
        char key[16];
        snprintf(key, sizeof(key), "k%d", id);
        int op = rand() % 3;

        if (op == 0) {
            int new_value = rand();
            int *value = malloc(sizeof(*value));
            assert(value != NULL);
            *value = new_value;
            if (rb_insert(t, key, value) != 0) {
                free(value);
            } else {
                if (!ref[id].present) {
                    ref[id].present = 1;
                    ref_size++;
                }
                ref[id].value = new_value;
            }
        } else if (op == 1) {
            void *found = rb_find(t, key);
            if (ref[id].present) {
                assert(found != NULL);
                assert(*(int *)found == ref[id].value);
            } else {
                assert(found == NULL);
            }
        } else {
            int result = rb_delete(t, key);
            if (ref[id].present) {
                assert(result == 0);
                ref[id].present = 0;
                ref_size--;
            } else {
                assert(result == -1);
            }
        }

        assert((long)rb_size(t) == ref_size);
        if ((i + 1) % VALIDATE_EVERY == 0) {
            assert(rb_validate(t) == 0);
        }
    }

    assert(rb_validate(t) == 0);
    rb_destroy(t);
    printf("fuzz: %ld ops OK\n", op_count);
    return 0;
}
