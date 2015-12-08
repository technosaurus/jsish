/* An implementation of Red-Black Trees with invariant node pointers. 
 * Nodes are allocated using single malloc that includes the key. 
 * This means that string keys (which are of varying length) can not be copied between nodes.
 * So instead of swapping node key/values, positions are swapped when balancing the tree. */

#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif

enum {RED=0, BLACK=1};

#define GETKEY(t,n) ((t->keyType == JSI_KEYS_ONEWORD || t->keyType == JSI_KEYS_STRINGPTR) ? n->key.oneWordValue:n->key.string)

/********************** RED/BLACK HELPERS **************************/

static Jsi_TreeEntry* grandparent(Jsi_TreeEntry* n) {
    Assert (n != NULL);
    Assert (n->parent != NULL);
    Assert (n->parent->parent != NULL);
    return n->parent->parent;
}

static Jsi_TreeEntry* sibling(Jsi_TreeEntry* n) {
    Assert (n != NULL);
    Assert (n->parent != NULL);
    return (n == n->parent->left ? n->parent->right : n->parent->left);
}

static Jsi_TreeEntry* uncle(Jsi_TreeEntry* n) {
    Assert (n != NULL);
    Assert (n->parent != NULL);
    Assert (n->parent->parent != NULL);
    return sibling(n->parent);
}

static int node_color(Jsi_TreeEntry* n) {
    return n == NULL ? BLACK : n->f.bits.color;
}

static void set_color(Jsi_TreeEntry* n, int color) {
    if (color == BLACK && n == NULL) return;
    n->f.bits.color = color;
}

static void replace_node(Jsi_TreeEntry* oldn, Jsi_TreeEntry* newn) {
    Assert(oldn);
    Jsi_Tree* t = oldn->treePtr;
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

static void rotate_left(Jsi_TreeEntry* n) {
    Jsi_TreeEntry* r;
    Assert(n);
    r = n->right;
    replace_node(n, r);
    n->right = r->left;
    if (r->left != NULL) {
        r->left->parent = n;
    }
    r->left = n;
    n->parent = r;
}

static void rotate_right(Jsi_TreeEntry* n) {
    Jsi_TreeEntry* l;
    Assert(n);
    l = n->left;
    replace_node(n, l);
    n->left = l->right;
    if (l->right != NULL) {
        l->right->parent = n;
    }
    l->right = n;
    n->parent = l;
}

static void insert_case5(Jsi_TreeEntry* n) {
    Jsi_TreeEntry* g = grandparent(n);
    set_color(n->parent, BLACK);
    set_color(g, RED);
    if (n == n->parent->left) {
        rotate_right(g);
    } else {
        Assert (n == n->parent->right);
        rotate_left(g);
    }
}

static void insert_case4(Jsi_TreeEntry* n) {
    Jsi_TreeEntry* g = grandparent(n);
    if (n == n->parent->right && n->parent == g->left) {
        rotate_left(n->parent);
        n = n->left;
    } else if (n == n->parent->left && n->parent == g->right) {
        rotate_right(n->parent);
        n = n->right;
    }
    insert_case5(n);
}

static void insert_case1(Jsi_TreeEntry* n);

static void insert_case3(Jsi_TreeEntry* n) {
    Jsi_TreeEntry *g, *u = uncle(n);
    if (node_color(u) == RED) {
        set_color(n->parent, BLACK);
        set_color(u, BLACK);
        g = grandparent(n);
        set_color(g, RED);
        insert_case1(g);
    } else {
        insert_case4(n);
    }
}

static void insert_case2(Jsi_TreeEntry* n) {
    if (node_color(n->parent) == BLACK)
        return;
    insert_case3(n);
}

static void insert_case1(Jsi_TreeEntry* n) {
    if (n->parent == NULL)
        set_color(n, BLACK);
    else
        insert_case2(n);
}

static Jsi_TreeEntry* maximum_node(Jsi_TreeEntry* n) {
    Assert (n != NULL);
    while (n->right != NULL) {
        n = n->right;
    }
    return n;
}

static void delete_case6(Jsi_TreeEntry* n) {
    Jsi_TreeEntry* s = sibling(n);
    set_color(s, node_color(n->parent));
    set_color(n->parent, BLACK);
    if (n == n->parent->left) {
        Assert (node_color(s->right) == RED);
        set_color(s->right, BLACK);
        rotate_left(n->parent);
    }
    else
    {
        //Assert (node_color(s->left) == RED);
        set_color(s->left, BLACK);
        rotate_right(n->parent);
    }
}

static void delete_case5(Jsi_TreeEntry* n) {
    Jsi_TreeEntry* s = sibling(n);
    if (node_color(s) == BLACK ) {
        if (n == n->parent->left &&
                node_color(s->right) == BLACK &&
                node_color(s->left) == RED)
        {
            set_color(s, RED);
            set_color(s->left, BLACK);
            rotate_right(s);
        }
        else if (n == n->parent->right &&
                 node_color(s->right) == RED &&
                 node_color(s->left) == BLACK)
        {
            set_color(s, RED);
            set_color(s->right, BLACK);
            rotate_left(s);
        }
    }
    delete_case6(n);
}

static void delete_case4(Jsi_TreeEntry* n) {
    Jsi_TreeEntry* s = sibling(n);
    if (node_color(n->parent) == RED &&
            node_color(s) == BLACK &&
            node_color(s->left) == BLACK &&
            node_color(s->right) == BLACK)
    {
        set_color(s, RED);
        set_color(n->parent, BLACK);
    }
    else
        delete_case5(n);
}

static void delete_case1(Jsi_TreeEntry* n);

static void delete_case3(Jsi_TreeEntry* n) {
    Jsi_TreeEntry* s  = sibling(n);
    if (node_color(n->parent) == BLACK &&
        node_color(s) == BLACK &&
        node_color(s->left) == BLACK &&
        node_color(s->right) == BLACK)
    {
        set_color(s, RED);
        delete_case1(n->parent);
    } else
        delete_case4(n);
}

static void delete_case2(Jsi_TreeEntry* n) {
    Jsi_TreeEntry* s = sibling(n);
    if (node_color(s) == RED) {
        set_color(n->parent, RED);
        set_color(s, BLACK);
        if (n == n->parent->left)
            rotate_left(n->parent);
        else
            rotate_right(n->parent);
    }
    delete_case3(n);
}

static void delete_case1(Jsi_TreeEntry* n) {
    if (n->parent != NULL)
        delete_case2(n);
}

/***********************************************************/

int jsi_treeHeight(Jsi_TreeEntry* hPtr, int n)
{
    int l = -1, r = -1;
    if (hPtr->right == NULL && hPtr->right == NULL )
        return n;
    if (hPtr->left)
        l = jsi_treeHeight(hPtr->left, n+1);
    if (hPtr->right)
        r = jsi_treeHeight(hPtr->right, n+1);
    return (r > l ? r : l);
}

int jsi_nodeDepth(Jsi_TreeEntry* hPtr) {
    int d = 0;
    while (hPtr->parent != NULL) {
        d++;
        hPtr = hPtr->parent;
    }
    return d;
}


static int StringPtrCompare(Jsi_Tree *treePtr, const void *key1, const void *key2)
{
    //return (key1 - key2);
    if (key1 == key2) return 0;
    //return Jsi_DictionaryCompare((char*)key1, (char*)key2);
    return Jsi_Strcmp((char*)key1, (char*)key2);
}


static int StringCompare(Jsi_Tree *treePtr, const void *key1, const void *key2)
{
    return Jsi_DictionaryCompare((char*)key1, (char*)key2);
    //return Jsi_Strcmp((char*)key1, (char*)key2);
}

static int OneWordCompare(Jsi_Tree *treePtr, const void *key1, const void *key2)
{
    return (key1 - key2);
}

static int TreeArrayCompare(Jsi_Tree *treePtr, const void *key1, const void *key2)
{
    return memcmp(key1, key2, treePtr->keyType);
}


static Jsi_TreeEntry *TreeStringCreate( Jsi_Tree *treePtr, const void *key, int *newPtr)
{
    Jsi_TreeEntry *hPtr;
    size_t size;

    if ((hPtr = Jsi_TreeEntryFind(treePtr, key))) {
        if (newPtr)
            *newPtr = 0;
        return hPtr;
    }
    if (newPtr)
        *newPtr = 1;
    size = sizeof(Jsi_TreeEntry) + Jsi_Strlen(key) /*- sizeof(jsi_TreeKey)*/ + 1;
    hPtr = Jsi_Calloc(1,size);
    SIGINIT(hPtr,TREEENTRY);
    hPtr->treePtr = treePtr;
    hPtr->value = 0;
    Jsi_Strcpy(hPtr->key.string, key);
    treePtr->numEntries++;
    return hPtr;
}

static Jsi_TreeEntry *TreeArrayCreate(Jsi_Tree *treePtr, const void *key, int *newPtr)
{
    Jsi_TreeEntry *hPtr;
    size_t size;

    if ((hPtr = Jsi_TreeEntryFind(treePtr, key))) {
        if (newPtr)
            *newPtr = 0;
        return hPtr;
    }
    if (newPtr)
        *newPtr = 1;
    size = sizeof(Jsi_TreeEntry) + treePtr->keyType; /*- sizeof(jsi_TreeKey);*/
    hPtr = Jsi_Calloc(1,size);
    SIGINIT(hPtr,TREEENTRY);
    hPtr->treePtr = treePtr;
    hPtr->value = 0;
    memcpy(hPtr->key.string, key, treePtr->keyType);
    treePtr->numEntries++;
    return hPtr;
}

static Jsi_TreeEntry *OneWordCreate( Jsi_Tree *treePtr, const void *key, int *newPtr)
{
    Jsi_TreeEntry *hPtr;
    size_t size;
    if ((hPtr = Jsi_TreeEntryFind(treePtr, key))) {
        if (newPtr)
            *newPtr = 0;
        return hPtr;
    }
    if (newPtr)
        *newPtr = 1;
    size = sizeof(Jsi_TreeEntry);
    hPtr = Jsi_Calloc(1,size);
    SIGINIT(hPtr,TREEENTRY);
    hPtr->treePtr = treePtr;
    hPtr->value = 0;
    hPtr->key.oneWordValue = (void *)key;
    treePtr->numEntries++;
    return hPtr;
}


static Jsi_TreeEntry *StringPtrCreate( Jsi_Tree *treePtr, const void *key, int *newPtr)
{
    return OneWordCreate(treePtr, key, newPtr);
}

void *Jsi_TreeValueGet(Jsi_TreeEntry *hPtr)
{
    return hPtr->value;
}

void *Jsi_TreeKeyGet(Jsi_TreeEntry *hPtr)
{
    Jsi_Tree *treePtr = hPtr->treePtr;
    switch (treePtr->keyType) {
        case JSI_KEYS_STRINGPTR:
        case JSI_KEYS_ONEWORD: return hPtr->key.oneWordValue;
    }
    return hPtr->key.string;
}


Jsi_TreeEntry *Jsi_TreeEntryFind (Jsi_Tree *treePtr, const void *key)
{
    Jsi_TreeEntry* hPtr = treePtr->root;
    int rc;
    if (treePtr->flags.destroyed)
        return NULL;
    while (hPtr != NULL) {
        rc = treePtr->compareProc(treePtr, GETKEY(treePtr, hPtr), key);
        if (rc == 0) {
            return hPtr;
        }
        hPtr = (rc < 0 ? hPtr->left : hPtr->right);
    }
    return hPtr;
}

Jsi_TreeEntry *Jsi_TreeEntryCreate(Jsi_Tree *treePtr, const void *key, int *isNew)
{
    Jsi_TreeEntry* hPtr;
    int isn;
    if (treePtr->flags.destroyed)
        return NULL;
    treePtr->flags.inserting=1;
    if (treePtr->flags.internstr) {
        Assert(treePtr->keyType == JSI_KEYS_STRINGPTR);
        if (!treePtr->strHash)
            treePtr->strHash = Jsi_HashNew(treePtr->interp, JSI_KEYS_STRING, NULL);
        key = Jsi_HashEntryCreate(treePtr->strHash, key, NULL);
    }
    hPtr = treePtr->createProc(treePtr, key, &isn);
    if (isNew)
        *isNew = isn;
    if (isn == 0 || treePtr->flags.nonredblack == 1 || !hPtr) {
        treePtr->flags.inserting=0;
        return hPtr;
    }
    treePtr->epoch++;
    hPtr->f.bits.color = RED;
    if (treePtr->root == NULL) {
        treePtr->root = hPtr;
    } else {
        Jsi_TreeEntry* n = treePtr->root;
        while (1) {
            int rc = treePtr->compareProc(treePtr, GETKEY(treePtr,n) , key);
            if (rc == 0) {
                Assert(0);
            } else if (rc < 0) {
                if (n->left == NULL) {
                    n->left = hPtr;
                    break;
                } else {
                    n = n->left;
                }
            } else {
                if (n->right == NULL) {
                    n->right = hPtr;
                    break;
                } else {
                    n = n->right;
                }
            }
        }
        hPtr->parent = n;
    }
    insert_case1(hPtr);
    treePtr->flags.inserting = 0;
    return hPtr;
}

Jsi_Tree *Jsi_TreeNew(Jsi_Interp *interp, unsigned int keyType)
{
    Jsi_Tree* treePtr = Jsi_Calloc(1,sizeof(Jsi_Tree));
    SIGINIT(treePtr,TREE);
    treePtr->root = NULL;
    treePtr->interp = interp;
    treePtr->numEntries = 0;
    treePtr->epoch = 0;
    treePtr->keyType = keyType;

    switch (keyType) {
    case JSI_KEYS_STRING:   /* NULL terminated string keys. */
        treePtr->compareProc = StringCompare;
        treePtr->createProc = TreeStringCreate;
        break;

    case JSI_KEYS_STRINGPTR: /*  */
        treePtr->compareProc = StringPtrCompare;
        treePtr->createProc = StringPtrCreate;
        break;
        
    case JSI_KEYS_ONEWORD: /* 32 or 64 bit atomic keys. */
        treePtr->compareProc = OneWordCompare;
        treePtr->createProc = OneWordCreate;
        break;


    default:            /* Struct. */
        if (keyType < JSI_KEYS_STRUCT_MINSIZE) {
            Jsi_LogError("Jsi_TreeNew: Key size can't be %d, must be > %d\n", keyType, JSI_KEYS_STRUCT_MINSIZE);
            Jsi_Free(treePtr);
            return NULL;
        }
        treePtr->compareProc = TreeArrayCompare;
        treePtr->createProc = TreeArrayCreate;
        break;
    }
    return treePtr;
}

static void destroy_node(Jsi_Interp *interp, Jsi_TreeEntry* n)
{
    if (n == NULL) return;
    if (n->right != NULL) destroy_node(interp, n->right);
    if (n->left != NULL) destroy_node(interp, n->left);
    n->left = n->right = NULL;
    Jsi_TreeEntryDelete(n);
}

void Jsi_TreeDelete (Jsi_Tree *treePtr)
{
    SIGASSERT(treePtr, TREE);
    if (treePtr->flags.destroyed)
        return;
    treePtr->flags.destroyed = 1;
    /*if (treePtr->freeProc)
        treePtr->freeProc(treePtr);*/
    destroy_node(treePtr->interp, treePtr->root);
    MEMCLEAR(treePtr);
    Jsi_Free(treePtr);
}

/* Swap positions of nodes in tree.  This avoids moving the value, which we can't do for strings. */
static void SwapNodes(Jsi_TreeEntry* n, Jsi_TreeEntry* m)
{
    Jsi_Tree* t = n->treePtr;
    Jsi_TreeEntry *np, *nl, *nr, *mp, *ml, *mr;
    int mpc = 0, npc = 0, col = n->f.bits.color;
    n->f.bits.color = m->f.bits.color;  m->f.bits.color = col;
    np = n->parent; nl = n->left; nr = n->right;
    mp = m->parent; ml = m->left; mr = m->right;
    if (mp) mpc = (mp->left == m ?1 : 2);
    if (np) npc = (np->left == n ?1 : 2);

    n->parent = mp; n->left = ml; n->right = mr;
    m->parent = np; m->left = nl; m->right = nr;
    
    if (np == m) {
        m->parent = n;
        if (mr == n) n->right = m; else n->left = m;
    } else if (mp == n) {
        n->parent = m;
        if (nr == m) m->right = n; else m->left = n;
    }
    /* Fixup back pointers. */
    if (m->left)  m->left->parent  = m;
    if (m->right) m->right->parent = m;
    if (n->left)  n->left->parent  = n;
    if (n->right) n->right->parent = n;
    if (mpc) { if (mpc==1) n->parent->left = n; else  n->parent->right = n;}
    if (npc) { if (npc==1) m->parent->left = m; else  m->parent->right = m; }
    if (n->parent == NULL) {
        t->root = n;
    } else if (m->parent == NULL) {
        t->root = m;
    }
}

static void delete_one_child(Jsi_TreeEntry*n)
{
    Jsi_TreeEntry *child;
    Assert(n->left == NULL || n->right == NULL);
    child = n->right == NULL ? n->left  : n->right;
#if 1
    if (node_color(n) == BLACK) {
        set_color(n, node_color(child));
        delete_case1(n);
    }
    replace_node(n, child);
    if (n->parent == NULL && child != NULL)
        set_color(child, BLACK);
    
#else
    replace_node(n, child);
    if (node_color(n) == BLACK) {
        if (node_color(child) == RED)
            child->f.bits.color = BLACK;
        else
            delete_case1(n);
    }
#endif
}

void Jsi_TreeEntryDelete (Jsi_TreeEntry *entryPtr)
{
    Jsi_TreeEntry* n = entryPtr;
    Jsi_Tree* treePtr = n->treePtr;

    if (treePtr->flags.destroyed  || treePtr->flags.nonredblack == 1 /* || entryPtr->f.bits.deletesub */) {
        goto dodel;
    }
    /*printf("DEL(tree=%p,root=%p): (%p)%s\n", treePtr, treePtr->root, entryPtr,(char*)entryPtr->key.string);*/
    /*dumpTree(treePtr);*/
    entryPtr->treePtr->epoch++;
    if (n->left != NULL && n->right != NULL) {
        /* swap key/values delete pred instead */
        Jsi_TreeEntry* pred = maximum_node(n->left);
        switch (treePtr->keyType) {
        case JSI_KEYS_STRING:
            SwapNodes(n,pred);
            break;
        case JSI_KEYS_STRINGPTR:
        case JSI_KEYS_ONEWORD: {
            void *nv = n->value;
            n->value = pred->value;
            pred->value = nv;
            nv = n->key.oneWordValue;
            n->key.oneWordValue = pred->key.oneWordValue;
            pred->key.oneWordValue = nv;
            n = pred;
            break;
        }
        default: {
            int i;
            void *nv = n->value;
            n->value = pred->value;
            pred->value = nv;
            char ct, *cs = (char*)(n->key.string), *cd = (char*)(pred->key.string);
            for (i=0; i<treePtr->keyType; i++, cs++, cd++) {
                ct = *cd;
                *cd = *cs;
                *cs = ct;
            }
        }
                
        }
    }
    delete_one_child(n);
    /*dumpTree(treePtr);*/
dodel:
    treePtr->numEntries--;
    n->treePtr = NULL;
    if (treePtr->freeProc)
        treePtr->freeProc(treePtr->interp, n);
    Jsi_Free(n);
}

static void searchSpace(Jsi_TreeSearch *searchPtr, int n)
{
    if ((searchPtr->top+n) >= searchPtr->max) {
        int i, cnt = (searchPtr->max *= 2);
        if (searchPtr->Ptrs == searchPtr->staticPtrs)
            searchPtr->Ptrs = Jsi_Calloc(cnt, sizeof(Jsi_TreeEntry*));
        else
            searchPtr->Ptrs = Jsi_Realloc(searchPtr->Ptrs, cnt* sizeof(Jsi_TreeEntry*));
        for (i=0; i<cnt; i++)
            SIGINIT((searchPtr->Ptrs[i]),TREEENTRY);

    }
}

static Jsi_TreeEntry *searchAdd(Jsi_TreeSearch *searchPtr,  Jsi_TreeEntry *hPtr)
{
    int order = (searchPtr->flags & JSI_TREE_ORDER_MASK);
    searchSpace(searchPtr, 2);
    switch (order) {
        case JSI_TREE_LEVELORDER:
            if (hPtr) {
                if (hPtr->right)
                    searchPtr->Ptrs[searchPtr->top++] = hPtr->right;
                if (hPtr->left)
                    searchPtr->Ptrs[searchPtr->top++] = hPtr->left;
                return hPtr;
            }
            if (searchPtr->top<=0)
                return NULL;
            hPtr = searchPtr->Ptrs[0];
            searchPtr->top--;
            if (searchPtr->top > 0) {
                /* Not very efficient way to implement a queue, but works for now. */
                memmove(searchPtr->Ptrs, searchPtr->Ptrs+1, sizeof(Jsi_TreeEntry*)*searchPtr->top);
            }
            if (hPtr->right)
                searchPtr->Ptrs[searchPtr->top++] = hPtr->right;
            if (hPtr->left)
                searchPtr->Ptrs[searchPtr->top++] = hPtr->left;
            return hPtr;
            break;
            
        case JSI_TREE_POSTORDER:
            if (hPtr)
                searchPtr->Ptrs[searchPtr->top++] = searchPtr->current = hPtr;
            while (searchPtr->top>0) {
                hPtr = searchPtr->Ptrs[searchPtr->top-1];
                if (hPtr->right == searchPtr->current || hPtr->left == searchPtr->current ||
                    (hPtr->left == NULL && hPtr->right == NULL)) {
                    searchPtr->top--;
                    searchPtr->current = hPtr;
                    return hPtr;
                } else {
                    searchSpace(searchPtr, 2);
                    if (hPtr->left)
                        searchPtr->Ptrs[searchPtr->top++] = hPtr->left;
                    if (hPtr->right)
                        searchPtr->Ptrs[searchPtr->top++] = hPtr->right;
                }
            }
            return NULL;
            break;
            
        case JSI_TREE_PREORDER:
            if (!hPtr) {
                if (searchPtr->top<=0) return NULL;
                hPtr = searchPtr->Ptrs[--searchPtr->top];
            }
            searchPtr->Ptrs[searchPtr->top++] = hPtr;
            if (hPtr->left) searchPtr->Ptrs[searchPtr->top++] = hPtr->left;
            if (hPtr->right) searchPtr->Ptrs[searchPtr->top++] = hPtr->right;
            break;
            
        case JSI_TREE_INORDER:
            while (1) {
                searchSpace(searchPtr, 2);
                if (searchPtr->current) {
                    searchPtr->Ptrs[searchPtr->top++] = searchPtr->current;
                    searchPtr->current = searchPtr->current->right;
                } else {
                    if (searchPtr->top<=0)
                        return NULL;
                    hPtr = searchPtr->Ptrs[--searchPtr->top] ;
                    searchPtr->current = hPtr->left;
                    return hPtr;
                }
            }
            break;
            
        default:
            if (hPtr) {
                Jsi_Interp *interp = hPtr->treePtr->interp;
                Jsi_LogError("Invalid order: %d", order);    
            }    
    }
    return searchPtr->Ptrs[--searchPtr->top];
}

Jsi_TreeEntry *Jsi_TreeSearchFirst (Jsi_Tree *treePtr, Jsi_TreeSearch *searchPtr, int flags)
{
    Jsi_TreeEntry *hPtr;
    if (!treePtr) return NULL;
    memset(searchPtr, 0, sizeof(Jsi_TreeSearch));
    searchPtr->treePtr = treePtr;
    searchPtr->flags = flags;
    searchPtr->Ptrs = searchPtr->staticPtrs;
    searchPtr->max = sizeof(searchPtr->staticPtrs)/sizeof(searchPtr->staticPtrs[0]);
    searchPtr->epoch = treePtr->epoch;
    searchPtr->current = hPtr = treePtr->root;
    return searchAdd(searchPtr, hPtr);
}

void Jsi_TreeValueSet(Jsi_TreeEntry *hPtr, void *value)
{
    Jsi_Value *v = value;
#ifdef JSI_DEBUG_MEMORY
    SIGASSERT(v,VALUE);
#endif
    hPtr->value = value;
}

#ifndef JSI_LITE_ONLY

Jsi_Tree *Jsi_TreeFromValue(Jsi_Interp *interp, Jsi_Value *v)
{
    if (!Jsi_ValueIsObjType(interp, v, JSI_OT_OBJECT))
        return NULL;
    return v->d.obj->tree;
}

#endif 

Jsi_TreeEntry *Jsi_TreeSearchNext(Jsi_TreeSearch *searchPtr)
{
    Jsi_TreeEntry *hPtr = NULL;
    if (searchPtr->epoch == searchPtr->treePtr->epoch)
        hPtr = searchAdd(searchPtr, NULL);
    if (!hPtr)
        Jsi_TreeSearchDone(searchPtr);
    return hPtr;
}

void Jsi_TreeSearchDone(Jsi_TreeSearch *searchPtr)
{
    if (searchPtr->Ptrs != searchPtr->staticPtrs)
        Jsi_Free(searchPtr->Ptrs);
    searchPtr->Ptrs = searchPtr->staticPtrs;
    searchPtr->top = 0;
}

Jsi_TreeEntry *Jsi_TreeSet(Jsi_Tree *treePtr, const void *key, void *value)
{
    Jsi_TreeEntry *hPtr;
    int isNew;
    hPtr = Jsi_TreeEntryCreate(treePtr, key, &isNew);
    if (!hPtr) return hPtr;
    Jsi_TreeValueSet(hPtr, value);
    return hPtr;
}

void *Jsi_TreeGet(Jsi_Tree *treePtr, void *key)
{
    Jsi_TreeEntry *hPtr = Jsi_TreeEntryFind(treePtr, key);
    if (!hPtr)
        return NULL;
    return Jsi_TreeValueGet(hPtr);
}

static int tree_inorder(Jsi_Tree *treePtr, Jsi_TreeEntry *hPtr, Jsi_TreeWalkProc *callback, void *data) {
    int epoch = treePtr->epoch;
    if (hPtr == NULL) return JSI_OK;
    if (hPtr->right != NULL) {
        if (tree_inorder(treePtr, hPtr->right, callback, data) != JSI_OK || epoch != treePtr->epoch)
            return JSI_ERROR;
    }
    if (callback(treePtr, hPtr, data) != JSI_OK || epoch != treePtr->epoch)
        return JSI_ERROR;
    Assert(hPtr->treePtr);
    if (hPtr->left != NULL) {
        if (tree_inorder(treePtr, hPtr->left, callback, data) != JSI_OK || epoch != treePtr->epoch)
            return JSI_ERROR;
    }
    return JSI_OK;
}


static int tree_preorder(Jsi_Tree *treePtr, Jsi_TreeEntry *hPtr, Jsi_TreeWalkProc *callback, void *data) {
    int epoch = treePtr->epoch;
    if (hPtr == NULL) return JSI_OK;
    if (callback(treePtr, hPtr, data) != JSI_OK || epoch != treePtr->epoch)
        return JSI_ERROR;
    if (hPtr->right != NULL) {
        if (tree_preorder(treePtr, hPtr->right, callback, data) != JSI_OK || epoch != treePtr->epoch)
            return JSI_ERROR;
    }
    if (hPtr->left != NULL) {
        if (tree_preorder(treePtr, hPtr->left, callback, data) != JSI_OK || epoch != treePtr->epoch)
            return JSI_ERROR;
    }
    return JSI_OK;
}


static int tree_postorder(Jsi_Tree *treePtr, Jsi_TreeEntry *hPtr, Jsi_TreeWalkProc *callback, void *data) {
    int epoch = treePtr->epoch;
    if (hPtr == NULL) return JSI_OK;
    if (hPtr->right != NULL) {
        if (tree_postorder(treePtr, hPtr->right, callback, data) != JSI_OK || epoch != treePtr->epoch)
            return JSI_ERROR;
    }
    if (hPtr->left != NULL) {
        if (tree_postorder(treePtr, hPtr->left, callback, data) != JSI_OK || epoch != treePtr->epoch)
            return JSI_ERROR;
    }
    if (callback(treePtr, hPtr, data) != JSI_OK || epoch != treePtr->epoch)
        return JSI_ERROR;
    return JSI_OK;
}


static int tree_levelorder(Jsi_Tree *treePtr, Jsi_TreeEntry *hPtr, Jsi_TreeWalkProc *callback,
    void *data, int curlev, int level, int *cnt) {
    int epoch = treePtr->epoch;
    if (hPtr == NULL) return JSI_OK;
    if (curlev > level) return JSI_OK;
    if (curlev == level) {
        if (callback(treePtr, hPtr, data) != JSI_OK || epoch != treePtr->epoch)
            return JSI_ERROR;
        (*cnt)++;
    }
    if (hPtr->right != NULL) {
        if (tree_levelorder(treePtr, hPtr->right, callback, data, curlev+1, level, cnt) != JSI_OK || epoch != treePtr->epoch)
            return JSI_ERROR;
    }
    if (hPtr->left != NULL) {
        if (tree_levelorder(treePtr, hPtr->left, callback, data, curlev+1, level, cnt) != JSI_OK || epoch != treePtr->epoch)
            return JSI_ERROR;
    }
    return JSI_OK;
}


int Jsi_TreeWalk(Jsi_Tree* treePtr, Jsi_TreeWalkProc* callback, void *data, int flags) {
    Jsi_Interp *interp = treePtr->interp;
    int n = 0, m, lastm, order;
    order = flags & JSI_TREE_ORDER_MASK;
    switch (order) {
    case JSI_TREE_PREORDER:
        return tree_preorder(treePtr, treePtr->root, callback, data);
    case JSI_TREE_POSTORDER:
        return tree_postorder(treePtr, treePtr->root, callback, data);
    case JSI_TREE_INORDER:
        return tree_inorder(treePtr, treePtr->root, callback, data);
    case JSI_TREE_LEVELORDER:
        while (1) {
            lastm = m;
            if (tree_levelorder(treePtr, treePtr->root, callback, data, 0, n, &m) != JSI_OK)
                return JSI_ERROR;
            if (lastm == m)
                return JSI_OK;
            n++;
        }
            
    default:
        Jsi_LogError("Invalid order: %d", order);
    }
    return JSI_ERROR;
}

#ifdef JSI_TEST_RBTREE

int mycall(Jsi_Tree* treePtr, Jsi_TreeEntry* hPtr, void *data)
{
    printf("CALL: %s(%d) : %d\n", (char*)Jsi_TreeKeyGet(hPtr), jsi_nodeDepth(hPtr), (int)Jsi_TreeValueGet(hPtr));
    return JSI_OK;
}
static void TreeTest(Jsi_Interp* interp) {
    Jsi_Tree *st, *wt, *mt;
    Jsi_TreeEntry *hPtr, *hPtr2;
    int isNew, i;
    Jsi_TreeSearch srch;
    struct tdata {
        int n;
        int m;
    } t1, t2;
    char nbuf[100];
    
    wt = Jsi_TreeNew(interp, JSI_KEYS_ONEWORD);
    mt = Jsi_TreeNew(interp, sizeof(struct tdata));

    Jsi_TreeSet(wt, wt,(void*)0x88);
    Jsi_TreeSet(wt, mt,(void*)0x99);
    printf("WT: %p\n", Jsi_TreeGet(wt, mt));
    printf("WT2: %p\n", Jsi_TreeGet(wt, wt));
    Jsi_TreeDelete(wt);

    t1.n = 0; t1.m = 1;
    t2.n = 1; t2.m = 2;
    Jsi_TreeSet(mt, &t1,(void*)0x88);
    Jsi_TreeSet(mt, &t2,(void*)0x99);
    Jsi_TreeSet(mt, &t2,(void*)0x98);
    printf("CT: %p\n", Jsi_TreeGet(mt, &t1));
    printf("CT2: %p\n", Jsi_TreeGet(mt, &t2));
    Jsi_TreeDelete(mt);

    st = Jsi_TreeNew(interp, JSI_KEYS_STRING);
    hPtr = Jsi_TreeEntryCreate(st, "bob", &isNew);
    Jsi_TreeValueSet(hPtr, (void*)99);
    Jsi_TreeSet(st, "zoe",(void*)77);
    hPtr2 = Jsi_TreeSet(st, "ted",(void*)55);
    Jsi_TreeSet(st, "philip",(void*)66);
    Jsi_TreeSet(st, "alice",(void*)77);
    puts("SRCH");
    for (hPtr=Jsi_TreeSearchFirst(st,&srch,  JSI_TREE_INORDER); hPtr; hPtr=Jsi_TreeSearchNext(&srch))
        mycall(st, hPtr, NULL);
    Jsi_TreeSearchDone(&srch);
    puts("IN");
    Jsi_TreeWalk(st, mycall, NULL, JSI_TREE_INORDER);
    puts("PRE");
    Jsi_TreeWalk(st, mycall, NULL, JSI_TREE_PREORDER);
    puts("POST");
    Jsi_TreeWalk(st, mycall, NULL, JSI_TREE_POSTORDER);
    puts("LEVEL");
    Jsi_TreeWalk(st, mycall, NULL, JSI_TREE_LEVELORDER);
    Jsi_TreeEntryDelete(hPtr2);
    puts("INDEL");
    Jsi_TreeWalk(st, mycall, NULL, 0);

    for (i=0; i<1000; i++) {
        sprintf(nbuf, "name%d", i);
        Jsi_TreeSet(st, nbuf,(void*)i);
    }
    Jsi_TreeWalk(st, mycall, NULL, 0);
    for (i=0; i<1000; i++) {
        Jsi_TreeEntryDelete(st->root);
    }
    puts("OK");
    Jsi_TreeWalk(st, mycall, NULL, 0);
    Jsi_TreeDelete(st);

}

int jsi_TreeInit(Jsi_Interp *interp)
{
    TreeTest(interp);
    return JSI_OK;
}

#else

int jsi_TreeInit(Jsi_Interp *interp)
{
    /* TODO: maintain hash table of trees created per interp? */
    return JSI_OK;
}
#endif
int Jsi_TreeSize(Jsi_Tree *treePtr) { return treePtr->numEntries; }
void* Jsi_TreeConf(Jsi_Tree *treePtr, int op, ...) { return NULL; }
