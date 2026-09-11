#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "rbtree.h"

static const char *const kFruitKeys[] = {"mango", "kiwi",  "pear",
                                          "fig",   "lime",  "apricot"};
static const size_t kFruitKeyCount = sizeof(kFruitKeys) / sizeof(kFruitKeys[0]);

static rbtree_t *create_test_tree(rb_value_free_fn value_free) {
    rbtree_t *t = rb_create(value_free);
    assert(t != NULL);
    return t;
}

static void test_size_empty_tree(void) {
    rbtree_t *t = create_test_tree(NULL);
    assert(rb_size(t) == 0);
    rb_destroy(t);
}

static void test_destroy_null_is_safe(void) {
    rb_destroy(NULL);
}

static void test_destroy_empty_tree(void) {
    rbtree_t *t = create_test_tree(NULL);
    rb_destroy(t);
}

static void test_find_empty_tree(void) {
    rbtree_t *t = create_test_tree(NULL);
    assert(rb_find(t, "anything") == NULL);
    rb_destroy(t);
}

static void test_find_present_key(void) {
    rbtree_t *t = create_test_tree(NULL);
    int value = 42;
    int rc = rb_insert(t, "key", &value);
    assert(rc == 0);
    assert(rb_find(t, "key") == &value);
    rb_destroy(t);
}

static void test_find_absent_key(void) {
    rbtree_t *t = create_test_tree(NULL);
    int value = 42;
    int rc = rb_insert(t, "key", &value);
    assert(rc == 0);
    assert(rb_find(t, "nope") == NULL);
    rb_destroy(t);
}

static void test_find_multiple_keys(void) {
    rbtree_t *t = create_test_tree(NULL);
    int a = 1, b = 2, c = 3, d = 4;
    int rc;
    rc = rb_insert(t, "banana", &a);
    assert(rc == 0);
    rc = rb_insert(t, "apple", &b);
    assert(rc == 0);
    rc = rb_insert(t, "cherry", &c);
    assert(rc == 0);
    rc = rb_insert(t, "date", &d);
    assert(rc == 0);

    assert(rb_find(t, "banana") == &a);
    assert(rb_find(t, "apple") == &b);
    assert(rb_find(t, "cherry") == &c);
    assert(rb_find(t, "date") == &d);
    assert(rb_find(t, "elderberry") == NULL);
    rb_destroy(t);
}

static void test_insert_single_is_findable(void) {
    rbtree_t *t = create_test_tree(NULL);
    int value = 7;
    int rc = rb_insert(t, "only", &value);
    assert(rc == 0);
    assert(rb_size(t) == 1);
    assert(rb_find(t, "only") == &value);
    rb_destroy(t);
}

static void test_insert_multiple_findable(void) {
    rbtree_t *t = create_test_tree(NULL);
    int values[6];

    for (size_t i = 0; i < kFruitKeyCount; i++) {
        values[i] = (int)i;
        int rc = rb_insert(t, kFruitKeys[i], &values[i]);
        assert(rc == 0);
        assert(rb_validate(t) == 0);
    }
    assert(rb_size(t) == kFruitKeyCount);
    for (size_t i = 0; i < kFruitKeyCount; i++) {
        assert(rb_find(t, kFruitKeys[i]) == &values[i]);
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
    assert(oc->count < sizeof(oc->keys) / sizeof(oc->keys[0]));
    oc->keys[oc->count++] = key;
}

static void test_insert_foreach_sorted_order(void) {
    rbtree_t *t = create_test_tree(NULL);
    int values[6];

    for (size_t i = 0; i < kFruitKeyCount; i++) {
        values[i] = (int)i;
        int rc = rb_insert(t, kFruitKeys[i], &values[i]);
        assert(rc == 0);
    }

    order_ctx_t oc = {.count = 0};
    rb_foreach(t, collect_key, &oc);
    assert(oc.count == kFruitKeyCount);
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
    rbtree_t *t = create_test_tree(mark_freed);
    tracked_value_t old_value = {.freed = 0};
    tracked_value_t new_value = {.freed = 0};

    int rc = rb_insert(t, "dup", &old_value);
    assert(rc == 0);
    assert(rb_size(t) == 1);
    rc = rb_insert(t, "dup", &new_value);
    assert(rc == 0);

    assert(old_value.freed == 1);
    assert(new_value.freed == 0);
    assert(rb_size(t) == 1);
    assert(rb_find(t, "dup") == &new_value);
    assert(rb_validate(t) == 0);
    rb_destroy(t);
}

static void test_insert_ascending_run_findable(void) {
    rbtree_t *t = create_test_tree(NULL);
    char keys[26][3];
    int values[26];

    for (int i = 0; i < 26; i++) {
        keys[i][0] = (char)('a' + i);
        keys[i][1] = '\0';
        values[i] = i;
        int rc = rb_insert(t, keys[i], &values[i]);
        assert(rc == 0);
        assert(rb_validate(t) == 0);
    }
    assert(rb_size(t) == 26);
    for (int i = 0; i < 26; i++) {
        assert(rb_find(t, keys[i]) == &values[i]);
    }
    rb_destroy(t);
}

/* insert_fixup has three shapes for the "uncle is black" case: a straight
 * outer rotation (LL/RR, already covered by the ascending run above) and
 * two inner zigzag double-rotations (LR, RL). These sequences are traced
 * by hand to land in each remaining branch of src/rbtree.c:94-136. */
static void test_insert_triggers_left_left_rotation(void) {
    rbtree_t *t = create_test_tree(NULL);
    const char *keys[] = {"c", "b", "a"};
    int values[3];

    for (size_t i = 0; i < 3; i++) {
        values[i] = (int)i;
        int rc = rb_insert(t, keys[i], &values[i]);
        assert(rc == 0);
        assert(rb_validate(t) == 0);
    }
    assert(rb_size(t) == 3);
    rb_destroy(t);
}

static void test_insert_triggers_left_right_zigzag(void) {
    rbtree_t *t = create_test_tree(NULL);
    const char *keys[] = {"c", "a", "b"};
    int values[3];

    for (size_t i = 0; i < 3; i++) {
        values[i] = (int)i;
        int rc = rb_insert(t, keys[i], &values[i]);
        assert(rc == 0);
        assert(rb_validate(t) == 0);
    }
    assert(rb_size(t) == 3);
    rb_destroy(t);
}

static void test_insert_triggers_right_left_zigzag(void) {
    rbtree_t *t = create_test_tree(NULL);
    const char *keys[] = {"a", "c", "b"};
    int values[3];

    for (size_t i = 0; i < 3; i++) {
        values[i] = (int)i;
        int rc = rb_insert(t, keys[i], &values[i]);
        assert(rc == 0);
        assert(rb_validate(t) == 0);
    }
    assert(rb_size(t) == 3);
    rb_destroy(t);
}

static void test_validate_empty_tree(void) {
    rbtree_t *t = create_test_tree(NULL);
    assert(rb_validate(t) == 0);
    rb_destroy(t);
}

static void test_validate_single_node(void) {
    rbtree_t *t = create_test_tree(NULL);
    int value = 1;
    int rc = rb_insert(t, "root", &value);
    assert(rc == 0);
    assert(rb_validate(t) == 0);
    rb_destroy(t);
}

/* Delete slice 1 covers only the BST-splice orchestration; delete_fixup is
 * still a no-op stub (see src/rbtree.c and PROMPTLOG.md), so only cases
 * where y_original_color ends up RED -- or the tree ends up empty -- can
 * pass rb_validate this slice. Cases needing real fixup (black leaf with a
 * red sibling, black node with exactly one red child) are deferred. */

static void test_delete_absent_key(void) {
    rbtree_t *t = create_test_tree(NULL);
    int value = 1;
    int rc = rb_insert(t, "present", &value);
    assert(rc == 0);
    assert(rb_delete(t, "missing") == -1);
    assert(rb_size(t) == 1);
    assert(rb_validate(t) == 0);
    rb_destroy(t);
}

static void test_delete_red_leaf(void) {
    rbtree_t *t = create_test_tree(NULL);
    int values[3];
    const char *keys[] = {"c", "b", "a"};
    for (size_t i = 0; i < 3; i++) {
        values[i] = (int)i;
        assert(rb_insert(t, keys[i], &values[i]) == 0);
    }
    /* c(B) root, b(B) left, a(R) left-left leaf -- delete the red leaf. */
    assert(rb_validate(t) == 0);
    assert(rb_delete(t, "a") == 0);
    assert(rb_size(t) == 2);
    assert(rb_find(t, "a") == NULL);
    assert(rb_validate(t) == 0);
    rb_destroy(t);
}

static void test_delete_sole_node(void) {
    rbtree_t *t = create_test_tree(NULL);
    int value = 1;
    assert(rb_insert(t, "only", &value) == 0);
    assert(rb_delete(t, "only") == 0);
    assert(rb_size(t) == 0);
    assert(rb_find(t, "only") == NULL);
    assert(rb_validate(t) == 0);
    rb_destroy(t);
}

static void test_delete_root_two_children_fixup_free(void) {
    rbtree_t *t = create_test_tree(NULL);
    int values[3];
    const char *keys[] = {"d", "b", "f"};
    for (size_t i = 0; i < 3; i++) {
        values[i] = (int)i;
        assert(rb_insert(t, keys[i], &values[i]) == 0);
    }
    /* d(B) root, b(R) left leaf, f(R) right leaf. Successor of "d" is "f"
     * (z's direct right child, a red leaf) -- fixup-free. */
    assert(rb_validate(t) == 0);
    assert(rb_delete(t, "d") == 0);
    assert(rb_size(t) == 2);
    assert(rb_find(t, "d") == NULL);
    assert(rb_find(t, "b") == &values[1]);
    assert(rb_find(t, "f") == &values[2]);
    assert(rb_validate(t) == 0);
    rb_destroy(t);
}

static void test_delete_two_children_deep_successor_fixup_free(void) {
    rbtree_t *t = create_test_tree(NULL);
    int values[4];
    const char *keys[] = {"d", "f", "b", "e"};
    for (size_t i = 0; i < 4; i++) {
        values[i] = (int)i;
        assert(rb_insert(t, keys[i], &values[i]) == 0);
    }
    /* d(B) root, b(B) left leaf, f(B) right with f->left = e(R) leaf.
     * Successor of "d" is "e" -- not z's direct child (y->parent != z),
     * exercising the deeper-successor splice sub-branch. "e" is a red
     * leaf, so this stays fixup-free. */
    assert(rb_validate(t) == 0);
    assert(rb_delete(t, "d") == 0);
    assert(rb_size(t) == 3);
    assert(rb_find(t, "d") == NULL);
    assert(rb_find(t, "b") == &values[2]);
    assert(rb_find(t, "f") == &values[1]);
    assert(rb_find(t, "e") == &values[3]);
    assert(rb_validate(t) == 0);
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
    test_insert_triggers_left_left_rotation();
    test_insert_triggers_left_right_zigzag();
    test_insert_triggers_right_left_zigzag();
    test_validate_empty_tree();
    test_validate_single_node();
    test_delete_absent_key();
    test_delete_red_leaf();
    test_delete_sole_node();
    test_delete_root_two_children_fixup_free();
    test_delete_two_children_deep_successor_fixup_free();
    printf("all tests passed\n");
    return 0;
}
