#include <assert.h>
#include <stdio.h>
#include <string.h>

/* White-box tests: pull in rbtree.c's internals (rb_node_t, rbtree_t,
 * RB_RED/RB_BLACK, rb_malloc) directly so hand-built, deliberately invalid
 * trees can be fed to rb_validate. The public API (include/rbtree.h) never
 * produces an invalid tree on its own -- rb_insert's and rb_delete's fixups
 * always leave a correct tree -- so there is no way to reach these cases
 * through the frozen header alone. Hand-built trees also let delete tests
 * hit a specific delete_fixup branch deterministically instead of hoping a
 * sequence of public-API calls lands on it. */
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

static void test_delete_two_children_root_triggers_fixup(void) {
    rb_node_t *b = make_node("b", RB_BLACK);
    rb_node_t *f = make_node("f", RB_BLACK);
    rb_node_t *d = make_node("d", RB_BLACK);
    d->left = b;
    b->parent = d;
    d->right = f;
    f->parent = d;
    rbtree_t *t = make_tree(d, 3);
    assert(rb_validate(t) == 0);

    /* Successor of "d" is "f" (z's direct right child, itself black), so
     * y_original_color is BLACK and delete_fixup actually runs: case 2
     * fires on x_parent=f/w=b (both of b's children are NIL/black), which
     * should recolor b red and leave f as the new black root. */
    assert(rb_delete(t, "d") == 0);
    assert(rb_size(t) == 2);
    assert(rb_validate(t) == 0);
    assert(strcmp(t->root->key, "f") == 0);
    assert(t->root->color == RB_BLACK);
    assert(t->root->left != NULL && strcmp(t->root->left->key, "b") == 0);
    assert(t->root->left->color == RB_RED);
    assert(t->root->right == NULL);
    rb_destroy(t);
}

static void test_delete_case1_red_sibling(void) {
    rb_node_t *b = make_node("b", RB_BLACK);
    rb_node_t *e = make_node("e", RB_BLACK);
    rb_node_t *g = make_node("g", RB_BLACK);
    rb_node_t *f = make_node("f", RB_RED);
    rb_node_t *d = make_node("d", RB_BLACK);
    d->left = b;
    b->parent = d;
    d->right = f;
    f->parent = d;
    f->left = e;
    e->parent = f;
    f->right = g;
    g->parent = f;
    rbtree_t *t = make_tree(d, 5);
    assert(rb_validate(t) == 0);

    /* Deleting "b" leaves x=NIL/x_parent=d with a red sibling "f" -- case 1
     * rotates left at d (f black, d red, new sibling becomes e), then
     * case 2 fires immediately on e (both of e's children are NIL/black),
     * recoloring e red and moving the "extra black" up to d. The loop
     * then exits because d is red, and the trailing fixup blackens it. */
    assert(rb_delete(t, "b") == 0);
    assert(rb_size(t) == 4);
    assert(rb_validate(t) == 0);
    assert(strcmp(t->root->key, "f") == 0);
    assert(t->root->color == RB_BLACK);
    assert(strcmp(t->root->left->key, "d") == 0);
    assert(t->root->left->color == RB_BLACK);
    assert(t->root->left->left == NULL);
    assert(t->root->left->right != NULL && strcmp(t->root->left->right->key, "e") == 0);
    assert(t->root->left->right->color == RB_RED);
    assert(strcmp(t->root->right->key, "g") == 0);
    assert(t->root->right->color == RB_BLACK);
    rb_destroy(t);
}

static void test_delete_case3_case4_cascade(void) {
    rb_node_t *a = make_node("a", RB_BLACK);
    rb_node_t *e = make_node("e", RB_RED);
    rb_node_t *h = make_node("h", RB_BLACK);
    rb_node_t *c = make_node("c", RB_BLACK);
    c->left = a;
    a->parent = c;
    c->right = h;
    h->parent = c;
    h->left = e;
    e->parent = h;
    rbtree_t *t = make_tree(c, 4);
    assert(rb_validate(t) == 0);

    /* Deleting "a" leaves x=NIL/x_parent=c with black sibling "h" whose
     * near-nephew "e" is red and far-nephew (h->right, NIL) is black --
     * Case 3 rotates right at h (e black, h red), then falls through into
     * Case 4 immediately: e takes c's color, c and h (new far-nephew) go
     * black, rotate left at c, loop exits via x = t->root. */
    assert(rb_delete(t, "a") == 0);
    assert(rb_size(t) == 3);
    assert(rb_validate(t) == 0);
    assert(strcmp(t->root->key, "e") == 0);
    assert(t->root->color == RB_BLACK);
    assert(strcmp(t->root->left->key, "c") == 0);
    assert(t->root->left->color == RB_BLACK);
    assert(t->root->left->left == NULL);
    assert(t->root->left->right == NULL);
    assert(strcmp(t->root->right->key, "h") == 0);
    assert(t->root->right->color == RB_BLACK);
    assert(t->root->right->left == NULL);
    assert(t->root->right->right == NULL);
    rb_destroy(t);
}

static void test_delete_case4_direct(void) {
    rb_node_t *h = make_node("h", RB_BLACK);
    rb_node_t *m = make_node("m", RB_RED);
    rb_node_t *l = make_node("l", RB_BLACK);
    rb_node_t *j = make_node("j", RB_BLACK);
    j->left = h;
    h->parent = j;
    j->right = l;
    l->parent = j;
    l->right = m;
    m->parent = l;
    rbtree_t *t = make_tree(j, 4);
    assert(rb_validate(t) == 0);

    /* Deleting "h" leaves x=NIL/x_parent=j with black sibling "l" whose
     * far-nephew "m" is already red on the first check -- Case 1 and
     * Case 3 both skip, landing directly in Case 4: l takes j's color,
     * j and m go black, rotate left at j, loop exits via x = t->root. */
    assert(rb_delete(t, "h") == 0);
    assert(rb_size(t) == 3);
    assert(rb_validate(t) == 0);
    assert(strcmp(t->root->key, "l") == 0);
    assert(t->root->color == RB_BLACK);
    assert(strcmp(t->root->left->key, "j") == 0);
    assert(t->root->left->color == RB_BLACK);
    assert(t->root->left->left == NULL);
    assert(t->root->left->right == NULL);
    assert(strcmp(t->root->right->key, "m") == 0);
    assert(t->root->right->color == RB_BLACK);
    assert(t->root->right->left == NULL);
    assert(t->root->right->right == NULL);
    rb_destroy(t);
}

static void test_delete_case4_mirror_direct(void) {
    rb_node_t *n = make_node("n", RB_RED);
    rb_node_t *p = make_node("p", RB_BLACK);
    rb_node_t *t_node = make_node("t", RB_BLACK);
    rb_node_t *r = make_node("r", RB_BLACK);
    r->left = p;
    p->parent = r;
    r->right = t_node;
    t_node->parent = r;
    p->left = n;
    n->parent = p;
    rbtree_t *tr = make_tree(r, 4);
    assert(rb_validate(tr) == 0);

    /* Deleting "t" leaves x=NIL/x_parent=r with r->left=p non-NULL, so
     * fixup takes the mirror branch (w = x_parent->left = p). p is black
     * with far-nephew (mirror far-nephew is w->left) "n" already red on
     * the first check -- Case 1 and Case 3 mirror both skip, landing
     * directly in Case 4 mirror: p takes r's color, r and n go black,
     * rotate right at r, loop exits via x = t->root. */
    assert(rb_delete(tr, "t") == 0);
    assert(rb_size(tr) == 3);
    assert(rb_validate(tr) == 0);
    assert(strcmp(tr->root->key, "p") == 0);
    assert(tr->root->color == RB_BLACK);
    assert(strcmp(tr->root->left->key, "n") == 0);
    assert(tr->root->left->color == RB_BLACK);
    assert(tr->root->left->left == NULL);
    assert(tr->root->left->right == NULL);
    assert(strcmp(tr->root->right->key, "r") == 0);
    assert(tr->root->right->color == RB_BLACK);
    assert(tr->root->right->left == NULL);
    assert(tr->root->right->right == NULL);
    rb_destroy(tr);
}

int main(void) {
    test_validate_rejects_red_root();
    test_validate_rejects_red_red_violation();
    test_validate_rejects_black_height_mismatch();
    test_validate_rejects_out_of_order_keys();
    test_validate_rejects_size_mismatch();
    test_validate_accepts_hand_built_valid_tree();
    test_delete_two_children_root_triggers_fixup();
    test_delete_case1_red_sibling();
    test_delete_case3_case4_cascade();
    test_delete_case4_direct();
    test_delete_case4_mirror_direct();
    printf("all whitebox tests passed\n");
    return 0;
}
