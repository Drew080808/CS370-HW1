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

int main(void) {
    test_size_empty_tree();
    test_destroy_null_is_safe();
    test_destroy_empty_tree();
    printf("all tests passed\n");
    return 0;
}
