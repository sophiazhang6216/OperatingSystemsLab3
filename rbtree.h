//This code is complete AI generated
//Prompt: can you make a redblack tree with the same interface as <linux/rbtree.h> so rb_node rb_root rb_left rb_right rb_link_node rb_insert_color rb_erase
//Here is the response:



// rbtree.h - Linux-compatible userspace red-black tree

#pragma once
#include <stddef.h>
#include <string.h>
#include <unistd.h>

// ── types ────────────────────────────────────────────────────────────────────

struct rb_node {
    unsigned long  __rb_parent_color;   // parent pointer + color packed in low bit
    struct rb_node *rb_right;
    struct rb_node *rb_left;
};

struct rb_root {
    struct rb_node *rb_node;
};

// ── color/parent packing (mirrors kernel) ────────────────────────────────────

#define RB_RED   0
#define RB_BLACK 1

#define __rb_parent(pc)       ((struct rb_node *)((pc) & ~3UL))
#define __rb_color(pc)        ((pc) & 1)
#define __rb_is_black(pc)     __rb_color(pc)
#define __rb_is_red(pc)       (!__rb_color(pc))
#define rb_color(rb)          __rb_color((rb)->__rb_parent_color)
#define rb_is_red(rb)         __rb_is_red((rb)->__rb_parent_color)
#define rb_is_black(rb)       __rb_is_black((rb)->__rb_parent_color)
#define rb_parent(rb)         __rb_parent((rb)->__rb_parent_color)

#define __rb_set_parent_color(rb, p, c) \
    ((rb)->__rb_parent_color = (unsigned long)(p) | (c))
#define rb_set_parent_color(rb, p, c) __rb_set_parent_color(rb, p, c)
#define rb_set_parent(rb, p) \
    ((rb)->__rb_parent_color = rb_color(rb) | (unsigned long)(p))

// ── initializers ─────────────────────────────────────────────────────────────

#define RB_ROOT  (struct rb_root){ NULL }
#define RB_EMPTY_ROOT(root) ((root)->rb_node == NULL)
#define RB_EMPTY_NODE(node) ((node)->__rb_parent_color == (unsigned long)(node))
#define RB_CLEAR_NODE(node) ((node)->__rb_parent_color = (unsigned long)(node))

// ── container_of ─────────────────────────────────────────────────────────────

#define rb_entry(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define rb_entry_safe(ptr, type, member) \
    ({ typeof(ptr) ____ptr = (ptr); \
       ____ptr ? rb_entry(____ptr, type, member) : NULL; })

// ── iteration ────────────────────────────────────────────────────────────────

static inline struct rb_node *rb_first(const struct rb_root *root) {
    struct rb_node *n = root->rb_node;
    if (!n) return NULL;
    while (n->rb_left) n = n->rb_left;
    return n;
}

static inline struct rb_node *rb_last(const struct rb_root *root) {
    struct rb_node *n = root->rb_node;
    if (!n) return NULL;
    while (n->rb_right) n = n->rb_right;
    return n;
}

static inline struct rb_node *rb_next(const struct rb_node *node) {
    struct rb_node *parent;
    if (RB_EMPTY_NODE(node)) return NULL;
    if (node->rb_right) {
        node = node->rb_right;
        while (node->rb_left) node = node->rb_left;
        return (struct rb_node *)node;
    }
    while ((parent = rb_parent(node)) && node == parent->rb_right)
        node = parent;
    return parent;
}

static inline struct rb_node *rb_prev(const struct rb_node *node) {
    struct rb_node *parent;
    if (RB_EMPTY_NODE(node)) return NULL;
    if (node->rb_left) {
        node = node->rb_left;
        while (node->rb_right) node = node->rb_right;
        return (struct rb_node *)node;
    }
    while ((parent = rb_parent(node)) && node == parent->rb_left)
        node = parent;
    return parent;
}

#define rbtree_for_each(pos, root) \
    for ((pos) = rb_first(root); (pos); (pos) = rb_next(pos))

// ── insert helpers (caller does the BST walk, we do the rebalance) ────────────

static inline void rb_link_node(struct rb_node *node, struct rb_node *parent,
                                struct rb_node **rb_link) {
    node->__rb_parent_color = (unsigned long)parent; // color = RED (0)
    node->rb_left = node->rb_right = NULL;
    *rb_link = node;
}

// ── rebalance after insert ────────────────────────────────────────────────────

static inline void __rb_rotate_left(struct rb_root *root, struct rb_node *x) {
    struct rb_node *y = x->rb_right;
    x->rb_right = y->rb_left;
    if (y->rb_left) rb_set_parent(y->rb_left, x);
    y->rb_left = x;
    struct rb_node *xp = rb_parent(x);
    __rb_set_parent_color(y, xp, rb_color(x));
    if (xp) {
        if (xp->rb_left == x) xp->rb_left = y;
        else                  xp->rb_right = y;
    } else {
        root->rb_node = y;
    }
    rb_set_parent_color(x, y, RB_RED);
}

static inline void __rb_rotate_right(struct rb_root *root, struct rb_node *x) {
    struct rb_node *y = x->rb_left;
    x->rb_left = y->rb_right;
    if (y->rb_right) rb_set_parent(y->rb_right, x);
    y->rb_right = x;
    struct rb_node *xp = rb_parent(x);
    __rb_set_parent_color(y, xp, rb_color(x));
    if (xp) {
        if (xp->rb_right == x) xp->rb_right = y;
        else                   xp->rb_left = y;
    } else {
        root->rb_node = y;
    }
    rb_set_parent_color(x, y, RB_RED);
}

static inline void rb_insert_color(struct rb_node *node, struct rb_root *root) {
    struct rb_node *parent, *gparent, *uncle;
    while ((parent = rb_parent(node)) && rb_is_red(parent)) {
        gparent = rb_parent(parent);
        if (!gparent) break;
        if (parent == gparent->rb_left) {
            uncle = gparent->rb_right;
            if (uncle && rb_is_red(uncle)) {
                rb_set_parent_color(uncle,  gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                rb_set_parent_color(gparent, rb_parent(gparent), RB_RED);
                node = gparent;
                continue;
            }
            if (node == parent->rb_right) {
                __rb_rotate_left(root, parent);
                node = parent;
                parent = rb_parent(node);
            }
            rb_set_parent_color(parent, gparent, RB_BLACK);
            rb_set_parent_color(gparent, rb_parent(gparent), RB_RED);
            __rb_rotate_right(root, gparent);
        } else {
            uncle = gparent->rb_left;
            if (uncle && rb_is_red(uncle)) {
                rb_set_parent_color(uncle,  gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                rb_set_parent_color(gparent, rb_parent(gparent), RB_RED);
                node = gparent;
                continue;
            }
            if (node == parent->rb_left) {
                __rb_rotate_right(root, parent);
                node = parent;
                parent = rb_parent(node);
            }
            rb_set_parent_color(parent, gparent, RB_BLACK);
            rb_set_parent_color(gparent, rb_parent(gparent), RB_RED);
            __rb_rotate_left(root, gparent);
        }
    }
    rb_set_parent_color(root->rb_node, NULL, RB_BLACK);
}

// ── erase ─────────────────────────────────────────────────────────────────────

static inline void __rb_erase_color(struct rb_root *root, struct rb_node *node,
                                    struct rb_node *parent) {
    struct rb_node *sibling;
    while ((!node || rb_is_black(node)) && node != root->rb_node) {
        if (parent->rb_left == node) {
            sibling = parent->rb_right;
            if (rb_is_red(sibling)) {
                rb_set_parent_color(sibling, parent, RB_BLACK);
                rb_set_parent_color(parent, rb_parent(parent), RB_RED);
                __rb_rotate_left(root, parent);
                sibling = parent->rb_right;
            }
            if ((!sibling->rb_left  || rb_is_black(sibling->rb_left)) &&
                (!sibling->rb_right || rb_is_black(sibling->rb_right))) {
                rb_set_parent_color(sibling, parent, RB_RED);
                node = parent;
                parent = rb_parent(node);
            } else {
                if (!sibling->rb_right || rb_is_black(sibling->rb_right)) {
                    if (sibling->rb_left)
                        rb_set_parent_color(sibling->rb_left, sibling, RB_BLACK);
                    rb_set_parent_color(sibling, parent, RB_RED);
                    __rb_rotate_right(root, sibling);
                    sibling = parent->rb_right;
                }
                rb_set_parent_color(sibling, rb_parent(parent), rb_color(parent));
                rb_set_parent_color(parent, sibling, RB_BLACK);
                if (sibling->rb_right)
                    rb_set_parent_color(sibling->rb_right, sibling, RB_BLACK);
                __rb_rotate_left(root, parent);
                node = root->rb_node;
                break;
            }
        } else {
            sibling = parent->rb_left;
            if (rb_is_red(sibling)) {
                rb_set_parent_color(sibling, parent, RB_BLACK);
                rb_set_parent_color(parent, rb_parent(parent), RB_RED);
                __rb_rotate_right(root, parent);
                sibling = parent->rb_left;
            }
            if ((!sibling->rb_right || rb_is_black(sibling->rb_right)) &&
                (!sibling->rb_left  || rb_is_black(sibling->rb_left))) {
                rb_set_parent_color(sibling, parent, RB_RED);
                node = parent;
                parent = rb_parent(node);
            } else {
                if (!sibling->rb_left || rb_is_black(sibling->rb_left)) {
                    if (sibling->rb_right)
                        rb_set_parent_color(sibling->rb_right, sibling, RB_BLACK);
                    rb_set_parent_color(sibling, parent, RB_RED);
                    __rb_rotate_left(root, sibling);
                    sibling = parent->rb_left;
                }
                rb_set_parent_color(sibling, rb_parent(parent), rb_color(parent));
                rb_set_parent_color(parent, sibling, RB_BLACK);
                if (sibling->rb_left)
                    rb_set_parent_color(sibling->rb_left, sibling, RB_BLACK);
                __rb_rotate_right(root, parent);
                node = root->rb_node;
                break;
            }
        }
    }
    if (node) rb_set_parent_color(node, rb_parent(node), RB_BLACK);
}

static inline void rb_erase(struct rb_node *node, struct rb_root *root) {
    struct rb_node *child, *parent;
    int color;

    if (!node->rb_left) {
        child = node->rb_right;
    } else if (!node->rb_right) {
        child = node->rb_left;
    } else {
        // two children: replace with in-order successor
        struct rb_node *successor = node->rb_right;
        while (successor->rb_left) successor = successor->rb_left;

        child  = successor->rb_right;
        parent = rb_parent(successor);
        color  = rb_color(successor);

        if (child) rb_set_parent(child, parent);

        if (parent == node) {
            parent->rb_right = child;
            parent = successor;
        } else {
            parent->rb_left = child;
        }

        successor->rb_right = node->rb_right;
        successor->rb_left  = node->rb_left;
        __rb_set_parent_color(successor, rb_parent(node), rb_color(node));

        if (rb_parent(node)) {
            if (rb_parent(node)->rb_left == node)
                rb_parent(node)->rb_left = successor;
            else
                rb_parent(node)->rb_right = successor;
        } else {
            root->rb_node = successor;
        }

        rb_set_parent(node->rb_left, successor);
        if (node->rb_right) rb_set_parent(node->rb_right, successor);

        if (color == RB_BLACK) __rb_erase_color(root, child, parent);
        return;
    }

    parent = rb_parent(node);
    color  = rb_color(node);

    if (child) rb_set_parent(child, parent);

    if (parent) {
        if (parent->rb_left == node) parent->rb_left = child;
        else                         parent->rb_right = child;
    } else {
        root->rb_node = child;
    }

    if (color == RB_BLACK) __rb_erase_color(root, child, parent);
}

//rest of code is human written by me :)

#define BUF_SIZE 2048
#define TRUE 1
#define FALSE 0

typedef struct tree_node {
    struct rb_node node;
    size_t cur_size;
    char line[BUF_SIZE];
} tree_node;

struct tree_node *tree_get_and_remove(struct rb_root *root, char *string) //code from kernel.org: official linux kernel archive
{
    struct rb_node *node = root->rb_node;

    while (node) {
        tree_node *data = rb_entry(node, tree_node, node);
        int result;

        result = strcmp(string, data->line);

        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            rb_erase(&data->node, root);
            return data;
    }
    return NULL;
}

int tree_print(struct rb_root *root, int fd)
{
    struct rb_node *n;
    tree_node *data;
    size_t len;
    size_t off;
    ssize_t w;
    char nl;

    nl = '\n';
    for (n = rb_first(root); n; n = rb_next(n)) {
        data = rb_entry(n, tree_node, node);
        len = data->cur_size;
        off = 0;
        while (off < len) {
            w = write(fd, data->line + off, len - off);
            if (w < 0) return -1;
            off += (size_t)w;
        }
        if (len == 0 || data->line[len - 1] != '\n') {
            if (write(fd, &nl, 1) < 0) return -1;
        }
    }
    return 0;
}

int tree_insert(struct rb_root *root, tree_node *data) //code from kernel.org: official linux kernel archive
{
      struct rb_node **link = &(root->rb_node), *parent = NULL;

      /* Figure out where to put link node */
      while (*link) {
            tree_node *this = rb_entry(*link, tree_node, node);
            int result = strcmp(data->line, this->line);

            parent = *link;
            if (result < 0)
                link = &((*link)->rb_left);
            else if (result > 0)
                link = &((*link)->rb_right);
            else
                return FALSE;
      }

      /* Add link node and rebalance tree. */
      rb_link_node(&data->node, parent, link);
      rb_insert_color(&data->node, root);

      return TRUE;
}