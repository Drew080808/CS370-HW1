#include <assert.h>
#include <stdio.h>
#include <string.h>

/* White-box tests: pull in rbtree.c's internals (rb_node_t, rbtree_t,
 * RB_RED/RB_BLACK, rb_malloc) directly so hand-built, deliberately invalid
 * trees can be fed to rb_validate. The public API (include/rbtree.h) never
 * produces an invalid tree on its own -- rb_insert's fixup always leaves a
 * correct tree, and rb_delete isn't implemented yet -- so there is no way
 * to reach these cases through the frozen header alone. */
#include "../src/rbtree.c"

static rb_node_t *make_node(const char *key, rb_color_t color) {
    rb_node_t *n = rb_malloc(sizeof(*n));
    assert(n != NULL);
    size_t klen = strlen(key) + 1;
    n->key = rb_malloc(klen);
    assert(n->key != NULL);
    memcpy(n->key, key, klen);
    n->value = NULL;
    n->color = color;
    n->parent = NULL;
    n->left = NULL;
    n->right = NULL;
    return n;
}

static rbtree_t *make_tree(rb_node_t *root, size_t size) {
    rbtree_t *t = rb_create(NULL);
    assert(t != NULL);
    t->root = root;
    t->size = size;
    return t;
}

static void test_validate_rejects_red_root(void) {
    rb_node_t *root = make_node("a", RB_RED);
    rbtree_t *t = make_tree(root, 1);
    assert(rb_validate(t) != 0);
    rb_destroy(t);
}

static void test_validate_rejects_red_red_violation(void) {
    rb_node_t *a = make_node("a", RB_RED);
    rb_node_t *b = make_node("b", RB_RED);
    rb_node_t *c = make_node("c", RB_BLACK);
    c->left = b;
    b->parent = c;
    b->left = a;
    a->parent = b;
    rbtree_t *t = make_tree(c, 3);
    assert(rb_validate(t) != 0);
    rb_destroy(t);
}

static void test_validate_rejects_black_height_mismatch(void) {
    rb_node_t *a = make_node("a", RB_BLACK);
    rb_node_t *b = make_node("b", RB_BLACK);
    b->left = a;
    a->parent = b;
    rbtree_t *t = make_tree(b, 2);
    assert(rb_validate(t) != 0);
    rb_destroy(t);
}

static void test_validate_rejects_out_of_order_keys(void) {
    rb_node_t *left = make_node("z", RB_BLACK);
    rb_node_t *right = make_node("a", RB_BLACK);
    rb_node_t *root = make_node("b", RB_BLACK);
    root->left = left;
    left->parent = root;
    root->right = right;
    right->parent = root;
    rbtree_t *t = make_tree(root, 3);
    assert(rb_validate(t) != 0);
    rb_destroy(t);
}

static void test_validate_rejects_size_mismatch(void) {
    rb_node_t *root = make_node("a", RB_BLACK);
    rbtree_t *t = make_tree(root, 2);
    assert(rb_validate(t) != 0);
    rb_destroy(t);
}

/* Control: a valid hand-built tree (the same shape rb_insert would produce
 * for keys c/b/a) must still validate, confirming the failures above come
 * from the specific defect introduced and not from make_node/make_tree
 * producing garbage that fails for unrelated reasons. */
static void test_validate_accepts_hand_built_valid_tree(void) {
    rb_node_t *left = make_node("a", RB_RED);
    rb_node_t *right = make_node("c", RB_RED);
    rb_node_t *root = make_node("b", RB_BLACK);
    root->left = left;
    left->parent = root;
    root->right = right;
    right->parent = root;
    rbtree_t *t = make_tree(root, 3);
    assert(rb_validate(t) == 0);
    rb_destroy(t);
}

int main(void) {
    test_validate_rejects_red_root();
    test_validate_rejects_red_red_violation();
    test_validate_rejects_black_height_mismatch();
    test_validate_rejects_out_of_order_keys();
    test_validate_rejects_size_mismatch();
    test_validate_accepts_hand_built_valid_tree();
    printf("all whitebox tests passed\n");
    return 0;
}
