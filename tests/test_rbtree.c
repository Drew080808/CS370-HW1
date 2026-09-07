#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "rbtree.h"

static void test_size_empty_tree(void) {
    rbtree_t *t = rb_create(NULL);
    assert(t != NULL);
    assert(rb_size(t) == 0);
    rb_destroy(t);
}

static void test_destroy_null_is_safe(void) {
    rb_destroy(NULL);
}

static void test_destroy_empty_tree(void) {
    rbtree_t *t = rb_create(NULL);
    assert(t != NULL);
    rb_destroy(t);
}

static void test_find_empty_tree(void) {
    rbtree_t *t = rb_create(NULL);
    assert(t != NULL);
    assert(rb_find(t, "anything") == NULL);
    rb_destroy(t);
}

static void test_find_present_key(void) {
    rbtree_t *t = rb_create(NULL);
    assert(t != NULL);
    int value = 42;
    assert(rb_insert(t, "key", &value) == 0);
    assert(rb_find(t, "key") == &value);
    rb_destroy(t);
}

static void test_find_absent_key(void) {
    rbtree_t *t = rb_create(NULL);
    assert(t != NULL);
    int value = 42;
    assert(rb_insert(t, "key", &value) == 0);
    assert(rb_find(t, "nope") == NULL);
    rb_destroy(t);
}

static void test_find_multiple_keys(void) {
    rbtree_t *t = rb_create(NULL);
    assert(t != NULL);
    int a = 1, b = 2, c = 3, d = 4;
    assert(rb_insert(t, "banana", &a) == 0);
    assert(rb_insert(t, "apple", &b) == 0);
    assert(rb_insert(t, "cherry", &c) == 0);
    assert(rb_insert(t, "date", &d) == 0);

    assert(rb_find(t, "banana") == &a);
    assert(rb_find(t, "apple") == &b);
    assert(rb_find(t, "cherry") == &c);
    assert(rb_find(t, "date") == &d);
    assert(rb_find(t, "elderberry") == NULL);
    rb_destroy(t);
}

static void test_insert_single_is_findable(void) {
    rbtree_t *t = rb_create(NULL);
    assert(t != NULL);
    int value = 7;
    assert(rb_insert(t, "only", &value) == 0);
    assert(rb_size(t) == 1);
    assert(rb_find(t, "only") == &value);
    rb_destroy(t);
}

static void test_insert_multiple_findable(void) {
    rbtree_t *t = rb_create(NULL);
    assert(t != NULL);
    const char *keys[] = {"mango", "kiwi", "pear", "fig", "lime", "apricot"};
    size_t n = sizeof(keys) / sizeof(keys[0]);
    int values[6];

    for (size_t i = 0; i < n; i++) {
        values[i] = (int)i;
        assert(rb_insert(t, keys[i], &values[i]) == 0);
    }
    assert(rb_size(t) == n);
    for (size_t i = 0; i < n; i++) {
        assert(rb_find(t, keys[i]) == &values[i]);
    }
    rb_destroy(t);
}

typedef struct {
    const char *keys[64];
    size_t count;
} order_ctx_t;

static void collect_key(const char *key, void *value, void *ctx) {
    (void)value;
    order_ctx_t *oc = ctx;
    oc->keys[oc->count++] = key;
}

static void test_insert_foreach_sorted_order(void) {
    rbtree_t *t = rb_create(NULL);
    assert(t != NULL);
    const char *keys[] = {"mango", "kiwi", "pear", "fig", "lime", "apricot"};
    size_t n = sizeof(keys) / sizeof(keys[0]);
    int values[6];

    for (size_t i = 0; i < n; i++) {
        values[i] = (int)i;
        assert(rb_insert(t, keys[i], &values[i]) == 0);
    }

    order_ctx_t oc = {.count = 0};
    rb_foreach(t, collect_key, &oc);
    assert(oc.count == n);
    for (size_t i = 1; i < oc.count; i++) {
        assert(strcmp(oc.keys[i - 1], oc.keys[i]) < 0);
    }
    rb_destroy(t);
}

typedef struct {
    int freed;
} tracked_value_t;

static void mark_freed(void *value) {
    ((tracked_value_t *)value)->freed = 1;
}

static void test_insert_overwrite_frees_old_value(void) {
    rbtree_t *t = rb_create(mark_freed);
    assert(t != NULL);
    tracked_value_t old_value = {.freed = 0};
    tracked_value_t new_value = {.freed = 0};

    assert(rb_insert(t, "dup", &old_value) == 0);
    assert(rb_size(t) == 1);
    assert(rb_insert(t, "dup", &new_value) == 0);

    assert(old_value.freed == 1);
    assert(new_value.freed == 0);
    assert(rb_size(t) == 1);
    assert(rb_find(t, "dup") == &new_value);
    rb_destroy(t);
}

static void test_insert_ascending_run_findable(void) {
    rbtree_t *t = rb_create(NULL);
    assert(t != NULL);
    char keys[26][3];
    int values[26];

    for (int i = 0; i < 26; i++) {
        keys[i][0] = (char)('a' + i);
        keys[i][1] = '\0';
        values[i] = i;
        assert(rb_insert(t, keys[i], &values[i]) == 0);
    }
    assert(rb_size(t) == 26);
    for (int i = 0; i < 26; i++) {
        assert(rb_find(t, keys[i]) == &values[i]);
    }
    rb_destroy(t);
}

int main(void) {
    test_size_empty_tree();
    test_destroy_null_is_safe();
    test_destroy_empty_tree();
    test_find_empty_tree();
    test_find_present_key();
    test_find_absent_key();
    test_find_multiple_keys();
    test_insert_single_is_findable();
    test_insert_multiple_findable();
    test_insert_foreach_sorted_order();
    test_insert_overwrite_frees_old_value();
    test_insert_ascending_run_findable();
    printf("all tests passed\n");
    return 0;
}
