/* avl.h - avl tree definitions */
/*
	Copyright (c) 1986, by David Michael Betz
	All rights reserved
*/

typedef struct tree {
    struct tnode *tr_root;	/* root node */
    int tr_cnt;			/* count of entries */
} TREE;

typedef struct tnode {
    int tn_b;			/* balance flag */
    struct tnode *tn_llink;	/* left subtree */
    struct tnode *tn_rlink;	/* right subtree */
    char *tn_key;		/* word */
    int tn_word;		/* word number */
} TNODE;

#define LLINK(n)	((n)->tn_llink)
#define RLINK(n)	((n)->tn_rlink)
#define KEY(n)		((n)->tn_key)
#define WORD(n)		((n)->tn_word)
#define B(n)		((n)->tn_b)
#define tentries(t)	((t)->tr_cnt)
