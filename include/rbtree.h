#ifndef RBTREE_H
#define RBTREE_H
#include <stddef.h>

/* Red-black tree keyed by C strings (strcmp order); values are opaque
 * void *. See CLAUDE.md for the full ownership contract. */

typedef struct rbtree rbtree_t;
typedef void (*rb_value_free_fn)(void *value);

/* Returns NULL on allocation failure.
 * value_free may be NULL (values not owned). */
rbtree_t *rb_create(rb_value_free_fn value_free);

/* Copies key (tree owns the copy).
 * Takes ownership of value ONLY on success.
 * Overwriting an existing key frees the old value via value_free.
 * Returns 0 on success, -1 on allocation failure (tree unchanged, value NOT
 * consumed; caller still owns it). */
int rb_insert(rbtree_t *t, const char *key, void *value);

/* Returns the value, or NULL if absent. Tree retains ownership. */
void *rb_find(const rbtree_t *t, const char *key);

/* Removes key; frees the key copy and the value.
 * Returns 0, or -1 if absent. */
int rb_delete(rbtree_t *t, const char *key);

size_t rb_size(const rbtree_t *t);

/* In-order traversal. */
void rb_foreach(const rbtree_t *t,
                void (*fn)(const char *key, void *value, void *ctx),
                void *ctx);

/* Returns 0 iff all red-black invariants hold:
 * root is black; no red node has a red child; every root-to-NIL path
 * crosses the same number of black nodes; in-order traversal is strictly
 * increasing under strcmp; rb_size matches the actual node count. */
int rb_validate(const rbtree_t *t);

/* Frees all nodes, key copies, and (if owned) values. NULL-safe. */
void rb_destroy(rbtree_t *t);

#endif
