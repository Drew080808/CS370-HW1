#include <assert.h>
#include <stdio.h>

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

int main(void) {
    test_size_empty_tree();
    test_destroy_null_is_safe();
    test_destroy_empty_tree();
    test_find_empty_tree();
    test_find_present_key();
    test_find_absent_key();
    test_find_multiple_keys();
    printf("all tests passed\n");
    return 0;
}
