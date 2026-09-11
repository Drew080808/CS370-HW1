#include <stdlib.h>
#include <string.h>

#include "rbtree.h"

/* Internal node/tree layout. NIL is represented by plain NULL (treated as
 * black wherever color is read), not a shared sentinel node. Nodes carry
 * an explicit parent pointer. */

typedef enum { RB_RED, RB_BLACK } rb_color_t;

typedef struct rb_node {
    char           *key;    /* owned copy */
    void           *value;  /* owned iff value_free != NULL, per contract */
    rb_color_t      color;
    struct rb_node *parent; /* NULL at root */
    struct rb_node *left;
    struct rb_node *right;
} rb_node_t;

struct rbtree {
    rb_node_t        *root; /* NULL = empty tree */
    size_t             size;
    rb_value_free_fn   value_free;
};

/* All heap allocation in this file goes through these two wrappers so a
 * future fault-injection test harness can make allocation fail on demand
 * without touching call sites. */
static void *rb_malloc(size_t size) {
    return malloc(size);
}

static void rb_free(void *ptr) {
    free(ptr);
}

/* Stub implementations below are scaffolding so the header's contract
 * links and the demo driver compiles/runs. Each remaining body is a safe
 * placeholder to be replaced one function at a time (see CLAUDE.md:
 * "implement in small slices"). */

rbtree_t *rb_create(rb_value_free_fn value_free) {
    rbtree_t *t = rb_malloc(sizeof(*t));
    if (t == NULL) {
        return NULL;
    }
    t->root = NULL;
    t->size = 0;
    t->value_free = value_free;
    return t;
}

static rb_color_t node_color(const rb_node_t *n) {
    return (n == NULL) ? RB_BLACK : n->color;
}

static void rotate_left(rbtree_t *t, rb_node_t *x) {
    rb_node_t *y = x->right;
    x->right = y->left;
    if (y->left != NULL) {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == NULL) {
        t->root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

static void rotate_right(rbtree_t *t, rb_node_t *x) {
    rb_node_t *y = x->left;
    x->left = y->right;
    if (y->right != NULL) {
        y->right->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == NULL) {
        t->root = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }
    y->right = x;
    x->parent = y;
}

static void insert_fixup(rbtree_t *t, rb_node_t *z) {
    while (z->parent != NULL && z->parent->color == RB_RED) {
        rb_node_t *p = z->parent;
        rb_node_t *g = p->parent; /* p is red => p isn't root => g exists */

        if (p == g->left) {
            rb_node_t *uncle = g->right;
            if (node_color(uncle) == RB_RED) {
                p->color = RB_BLACK;
                uncle->color = RB_BLACK;
                g->color = RB_RED;
                z = g;
            } else {
                if (z == p->right) {
                    z = p;
                    rotate_left(t, z);
                    p = z->parent;
                }
                p->color = RB_BLACK;
                g->color = RB_RED;
                rotate_right(t, g);
            }
        } else {
            rb_node_t *uncle = g->left;
            if (node_color(uncle) == RB_RED) {
                p->color = RB_BLACK;
                uncle->color = RB_BLACK;
                g->color = RB_RED;
                z = g;
            } else {
                if (z == p->left) {
                    z = p;
                    rotate_right(t, z);
                    p = z->parent;
                }
                p->color = RB_BLACK;
                g->color = RB_RED;
                rotate_left(t, g);
            }
        }
    }
    t->root->color = RB_BLACK;
}

int rb_insert(rbtree_t *t, const char *key, void *value) {
    rb_node_t *parent = NULL;
    rb_node_t *cur = t->root;
    int cmp = 0;

    while (cur != NULL) {
        cmp = strcmp(key, cur->key);
        if (cmp == 0) {
            if (t->value_free != NULL) {
                t->value_free(cur->value);
            }
            cur->value = value;
            return 0;
        }
        parent = cur;
        cur = (cmp < 0) ? cur->left : cur->right;
    }

    rb_node_t *node = rb_malloc(sizeof(*node));
    if (node == NULL) {
        return -1;
    }

    size_t klen = strlen(key) + 1;
    char *kcopy = rb_malloc(klen);
    if (kcopy == NULL) {
        rb_free(node);
        return -1;
    }
    memcpy(kcopy, key, klen);

    node->key = kcopy;
    node->value = value;
    node->color = RB_RED;
    node->left = NULL;
    node->right = NULL;
    node->parent = parent;

    if (parent == NULL) {
        t->root = node;
    } else if (cmp < 0) {
        parent->left = node;
    } else {
        parent->right = node;
    }

    t->size++;
    insert_fixup(t, node);
    return 0;
}

void *rb_find(const rbtree_t *t, const char *key) {
    rb_node_t *cur = t->root;
    while (cur != NULL) {
        int cmp = strcmp(key, cur->key);
        if (cmp == 0) {
            return cur->value;
        }
        cur = (cmp < 0) ? cur->left : cur->right;
    }
    return NULL;
}

/* Leftmost node of a subtree; used to find the in-order successor when the
 * node being deleted has two children. */
static rb_node_t *subtree_min(rb_node_t *n) {
    while (n->left != NULL) {
        n = n->left;
    }
    return n;
}

/* Replaces the subtree rooted at u with the subtree rooted at v (v may be
 * NULL). Guards v == NULL since NIL has no node object here to carry a
 * parent pointer -- callers that need it track v's new parent separately. */
static void transplant(rbtree_t *t, rb_node_t *u, rb_node_t *v) {
    if (u->parent == NULL) {
        t->root = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    if (v != NULL) {
        v->parent = u->parent;
    }
}

/* x is the node that moved into the spliced-out position (may be NULL);
 * x_parent is threaded through explicitly since x may be NULL and can't
 * carry its own parent pointer. */
static void delete_fixup(rbtree_t *t, rb_node_t *x, rb_node_t *x_parent) {
    (void)t;
    (void)x;
    (void)x_parent;
    /* TODO(M2 slice 2+): rebalance. */
}

int rb_delete(rbtree_t *t, const char *key) {
    rb_node_t *z = t->root;
    while (z != NULL) {
        int cmp = strcmp(key, z->key);
        if (cmp == 0) {
            break;
        }
        z = (cmp < 0) ? z->left : z->right;
    }
    if (z == NULL) {
        return -1;
    }

    rb_node_t *y = z;
    rb_color_t y_original_color = y->color;
    rb_node_t *x;
    rb_node_t *x_parent;

    if (z->left == NULL) {
        x = z->right;
        x_parent = z->parent;
        transplant(t, z, z->right);
    } else if (z->right == NULL) {
        x = z->left;
        x_parent = z->parent;
        transplant(t, z, z->left);
    } else {
        y = subtree_min(z->right);
        y_original_color = y->color;
        x = y->right;
        if (y->parent == z) {
            x_parent = y;
        } else {
            x_parent = y->parent;
            transplant(t, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        transplant(t, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    rb_free(z->key);
    if (t->value_free != NULL) {
        t->value_free(z->value);
    }
    rb_free(z);
    t->size--;

    if (y_original_color == RB_BLACK) {
        delete_fixup(t, x, x_parent);
    }
    return 0;
}

size_t rb_size(const rbtree_t *t) {
    return t->size;
}

static void foreach_subtree(const rb_node_t *node,
                            void (*fn)(const char *key, void *value, void *ctx),
                            void *ctx) {
    if (node == NULL) {
        return;
    }
    foreach_subtree(node->left, fn, ctx);
    fn(node->key, node->value, ctx);
    foreach_subtree(node->right, fn, ctx);
}

void rb_foreach(const rbtree_t *t,
                void (*fn)(const char *key, void *value, void *ctx),
                void *ctx) {
    foreach_subtree(t->root, fn, ctx);
}

/* Returns the black-height of node's subtree (NIL counts as black height
 * 0), or -1 if a red node has a red child or the two subtrees disagree on
 * black-height. */
static int check_black_height(const rb_node_t *node) {
    if (node == NULL) {
        return 0;
    }
    if (node->color == RB_RED &&
        (node_color(node->left) == RB_RED || node_color(node->right) == RB_RED)) {
        return -1;
    }
    int left_bh = check_black_height(node->left);
    if (left_bh < 0) {
        return -1;
    }
    int right_bh = check_black_height(node->right);
    if (right_bh < 0 || left_bh != right_bh) {
        return -1;
    }
    return left_bh + (node->color == RB_BLACK ? 1 : 0);
}

/* In-order walk checking strict increase under strcmp. *last tracks the
 * most recently visited key (NULL before the first). */
static int check_strictly_increasing(const rb_node_t *node, const char **last) {
    if (node == NULL) {
        return 1;
    }
    if (!check_strictly_increasing(node->left, last)) {
        return 0;
    }
    if (*last != NULL && strcmp(*last, node->key) >= 0) {
        return 0;
    }
    *last = node->key;
    return check_strictly_increasing(node->right, last);
}

static size_t count_subtree(const rb_node_t *node) {
    if (node == NULL) {
        return 0;
    }
    return 1 + count_subtree(node->left) + count_subtree(node->right);
}

int rb_validate(const rbtree_t *t) {
    if (node_color(t->root) != RB_BLACK) {
        return -1;
    }
    if (check_black_height(t->root) < 0) {
        return -1;
    }
    const char *last = NULL;
    if (!check_strictly_increasing(t->root, &last)) {
        return -1;
    }
    if (count_subtree(t->root) != t->size) {
        return -1;
    }
    return 0;
}

static void destroy_subtree(rb_node_t *node, rb_value_free_fn value_free) {
    if (node == NULL) {
        return;
    }
    destroy_subtree(node->left, value_free);
    destroy_subtree(node->right, value_free);
    rb_free(node->key);
    if (value_free != NULL) {
        value_free(node->value);
    }
    rb_free(node);
}

void rb_destroy(rbtree_t *t) {
    if (t == NULL) {
        return;
    }
    destroy_subtree(t->root, t->value_free);
    rb_free(t);
}
