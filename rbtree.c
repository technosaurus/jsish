/* Copyright (c) 2010 the authors listed at the following URL, and/or
the authors of referenced articles or incorporated external code:
http://en.literateprograms.org/Red-black_tree_(C)?action=history&offset=20090121005050
took too many lines, deleted
*/

#include <assert.h>
#include <stdlib.h>

#include "rbtree.h"
#include "mempool.h"

typedef rbtree_node node;
typedef enum rbtree_node_color color;

static rb_compare_func global_compare_func;
static rb_vfree_func global_vfree_func;
static rb_kfree_func global_kfree_func;
static rb_vreplace_func global_vreplace_func;
static rb_lookup_helper global_lookup_helper;
static rb_insert_helper global_insert_helper;

static mpool_t rbtree_pool;
static mpool_t node_pool;

static node grandparent(node n) {
	assert (n != NULL);
	assert (n->parent != NULL); /* Not the root node */
	assert (n->parent->parent != NULL); /* Not child of root */
	return n->parent->parent;
}

static node sibling(node n) {
	assert (n != NULL);
	assert (n->parent != NULL); /* Root node has no sibling */
	if (n == n->parent->left)
		return n->parent->right;
	else
		return n->parent->left;
}

static node uncle(node n) {
	assert (n != NULL);
	assert (n->parent != NULL); /* Root node has no uncle */
	assert (n->parent->parent != NULL); /* Children of root have no uncle */
	return sibling(n->parent);
}

static color node_color(node n) {
	return n == NULL ? BLACK : n->color;
}

static node new_node(void* key, void* value, color node_color, node left, node right) {
	node result = mpool_alloc(node_pool);
	result->key = key;
	result->value = value;
	result->color = node_color;
	result->left = left;
	result->right = right;
	if (left  != NULL)  left->parent = result;
	if (right != NULL) right->parent = result;
	result->parent = NULL;
	return result;
}

static node lookup_node(rbtree t, void* key, rb_compare_func compare, void *userdata) {
	node n = t->root;
	while (n != NULL) {
		int comp_result = compare(key, n->key);
		if (comp_result == 0) {
			if (global_lookup_helper) global_lookup_helper(n->key, userdata);
			return n;
		} else if (comp_result < 0) {
			n = n->left;
		} else {
			assert(comp_result > 0);
			n = n->right;
		}
	}
	return n;
}

static void replace_node(rbtree t, node oldn, node newn) {
	if (oldn->parent == NULL) {
		t->root = newn;
	} else {
		if (oldn == oldn->parent->left)
			oldn->parent->left = newn;
		else
			oldn->parent->right = newn;
	}
	if (newn != NULL) {
		newn->parent = oldn->parent;
	}
}

static void rotate_left(rbtree t, node n) {
	node r = n->right;
	replace_node(t, n, r);
	n->right = r->left;
	if (r->left != NULL) {
		r->left->parent = n;
	}
	r->left = n;
	n->parent = r;
}

static void rotate_right(rbtree t, node n) {
	node L = n->left;
	replace_node(t, n, L);
	n->left = L->right;
	if (L->right != NULL) {
		L->right->parent = n;
	}
	L->right = n;
	n->parent = L;
}

static void insert_case5(rbtree t, node n) {
	n->parent->color = BLACK;
	grandparent(n)->color = RED;
	if (n == n->parent->left && n->parent == grandparent(n)->left) {
		rotate_right(t, grandparent(n));
	} else {
		assert (n == n->parent->right && n->parent == grandparent(n)->right);
		rotate_left(t, grandparent(n));
	}
}

static void insert_case4(rbtree t, node n) {
	if (n == n->parent->right && n->parent == grandparent(n)->left) {
		rotate_left(t, n->parent);
		n = n->left;
	} else if (n == n->parent->left && n->parent == grandparent(n)->right) {
		rotate_right(t, n->parent);
		n = n->right;
	}
	insert_case5(t, n);
}

static void insert_case1(rbtree t, node n);

static void insert_case3(rbtree t, node n) {
	if (node_color(uncle(n)) == RED) {
		n->parent->color = BLACK;
		uncle(n)->color = BLACK;
		grandparent(n)->color = RED;
		insert_case1(t, grandparent(n));
	} else {
		insert_case4(t, n);
	}
}

static void insert_case2(rbtree t, node n) {
	if (node_color(n->parent) == BLACK)
		return; /* Tree is still valid */
	else
		insert_case3(t, n);
}

static void insert_case1(rbtree t, node n) {
	if (n->parent == NULL)
		n->color = BLACK;
	else
		insert_case2(t, n);
}

static node maximum_node(node n) {
	assert (n != NULL);
	while (n->right != NULL) {
		n = n->right;
	}
	return n;
}

static void delete_case6(rbtree t, node n) {
	sibling(n)->color = node_color(n->parent);
	n->parent->color = BLACK;
	if (n == n->parent->left) {
		assert (node_color(sibling(n)->right) == RED);
		sibling(n)->right->color = BLACK;
		rotate_left(t, n->parent);
	}
	else
	{
		assert (node_color(sibling(n)->left) == RED);
		sibling(n)->left->color = BLACK;
		rotate_right(t, n->parent);
	}
}

static void delete_case5(rbtree t, node n) {
	if (n == n->parent->left &&
		node_color(sibling(n)) == BLACK &&
		node_color(sibling(n)->left) == RED &&
		node_color(sibling(n)->right) == BLACK)
	{
		sibling(n)->color = RED;
		sibling(n)->left->color = BLACK;
		rotate_right(t, sibling(n));
	}
	else if (n == n->parent->right &&
			 node_color(sibling(n)) == BLACK &&
			 node_color(sibling(n)->right) == RED &&
			 node_color(sibling(n)->left) == BLACK)
	{
		sibling(n)->color = RED;
		sibling(n)->right->color = BLACK;
		rotate_left(t, sibling(n));
	}
	delete_case6(t, n);
}

static void delete_case4(rbtree t, node n) {
	if (node_color(n->parent) == RED &&
		node_color(sibling(n)) == BLACK &&
		node_color(sibling(n)->left) == BLACK &&
		node_color(sibling(n)->right) == BLACK)
	{
		sibling(n)->color = RED;
		n->parent->color = BLACK;
	}
	else
		delete_case5(t, n);
}

static void delete_case1(rbtree t, node n);
static void delete_case3(rbtree t, node n) {
	if (node_color(n->parent) == BLACK &&
		node_color(sibling(n)) == BLACK &&
		node_color(sibling(n)->left) == BLACK &&
		node_color(sibling(n)->right) == BLACK)
	{
		sibling(n)->color = RED;
		delete_case1(t, n->parent);
	}
	else
		delete_case4(t, n);
}

static void delete_case2(rbtree t, node n) {
	if (node_color(sibling(n)) == RED) {
		n->parent->color = RED;
		sibling(n)->color = BLACK;
		if (n == n->parent->left)
			rotate_left(t, n->parent);
		else
			rotate_right(t, n->parent);
	}
	delete_case3(t, n);
}

static void delete_case1(rbtree t, node n) {
	if (n->parent == NULL)
		return;
	else
		delete_case2(t, n);
}

rbtree rbtree_create() {
	rbtree t = mpool_alloc(rbtree_pool);
	t->root = NULL;
	return t;
}

void* rbtree_lookup(rbtree t, void* key, void *userdata) {
	node n = lookup_node(t, key, global_compare_func, userdata);
	return n == NULL ? NULL : n->value;
}

/* @return 0 - only replace existing key
 *         1 - successfully insert new node
 *        -1 - error occour
 */
int rbtree_insert(rbtree t, void* key, void* value) {
	node inserted_node = new_node(key, value, RED, NULL, NULL);
	if (t->root == NULL) {
		t->root = inserted_node;
	} else {
		node n = t->root;
		while (1) {
			int comp_result = global_compare_func(key, n->key);
			if (comp_result == 0) {
				int ret = -1;
				if (!global_insert_helper(n->key)) {
					global_vreplace_func(n->value, value);
					ret = 0;
				}
				
				global_vfree_func(key, value);
				global_kfree_func(key);
				mpool_free (inserted_node, node_pool);
				return ret;
			} else if (comp_result < 0) {
				if (n->left == NULL) {
					n->left = inserted_node;
					break;
				} else {
					n = n->left;
				}
			} else {
				assert (comp_result > 0);
				if (n->right == NULL) {
					n->right = inserted_node;
					break;
				} else {
					n = n->right;
				}
			}
		}
		inserted_node->parent = n;
	}
	insert_case1(t, inserted_node);
	return 1;
}

void rbtree_delete(rbtree t, void* key) {
	node child;
	node n = lookup_node(t, key, global_compare_func, NULL);
	if (n == NULL) return;  /* Key not found, do nothing */
	void *keytodelete = n->key;
	void *valuetodelete = n->value;
	
	if (n->left != NULL && n->right != NULL) {
		/* Copy key/value from predecessor and then delete it instead */
		node pred = maximum_node(n->left);
		
		n->key   = pred->key;
		n->value = pred->value;
		n = pred;
	}

	assert(n->left == NULL || n->right == NULL);
	child = n->right == NULL ? n->left  : n->right;
	if (node_color(n) == BLACK) {
		n->color = node_color(child);
		delete_case1(t, n);
	}
	replace_node(t, n, child);
	if (n->parent == NULL && child != NULL)
		child->color = BLACK;
	
	global_vfree_func(keytodelete, valuetodelete);
	global_kfree_func(keytodelete);
	mpool_free(n, node_pool);
}

int walk_tree_helper(rbtree_node n, void *userdata, rb_walk_callback callback) {
	if (n == NULL) return 0;
	
	if (n->right != NULL) walk_tree_helper(n->right, userdata, callback);
	if (callback(n->key, n->value, userdata)) return -1;
	if (n->left != NULL) walk_tree_helper(n->left, userdata, callback);
	
	return 0;
}

int rbtree_walk(rbtree t, void *userdata, rb_walk_callback callback) {
	return walk_tree_helper(t->root, userdata, callback);
}

void destroy_node(rbtree_node n)
{
	if (n == NULL) return;
	if (n->right != NULL) destroy_node(n->right);
	if (n->left != NULL) destroy_node(n->left);
	global_vfree_func(n->key, n->value);
	global_kfree_func(n->key);
	mpool_free(n, node_pool);
}

void rbtree_destroy(rbtree t)
{
	destroy_node(t->root);
	mpool_free(t, rbtree_pool);
}

void rbtree_module_init(rb_compare_func compare, 
						rb_kfree_func kfreefunc,
						rb_vfree_func vfreefunc,
						rb_vreplace_func vreplacefunc,
						rb_lookup_helper lookuphelper,
						rb_insert_helper inserthelper)
{
	rbtree_pool = mpool_create(sizeof(struct rbtree_t));
	node_pool = mpool_create(sizeof(struct rbtree_node_t));
	
	global_compare_func = compare;
	global_kfree_func = kfreefunc;
	global_vfree_func = vfreefunc;
	global_vreplace_func = vreplacefunc;
	global_lookup_helper = lookuphelper;
	global_insert_helper = inserthelper;
}
