/* Mark's hashtable
 * Uses open-addressing scheme. This implementation can store entries
 * containing any fixed-size key and value. Inserting or retrieving a 
 * key/value pair returns a reference to a hashtable entry. References to
 * hashtable entries remain valid even after subsequent insertions or
 * deletions, this is because hashtable entries are each allocated
 * dynamically. The hashtable only stores pointers to these entries.
 * This implementation removes the need to copy all key/value pairs when the
 * hashtable needs to be resized. All entries are doubly-linked, thus
 * iterating from one hashtable entry to the next takes contant time.
 */
#ifndef MARKS_HASHTABLE_H
#define MARKS_HASHTABLE_H

#include <stdlib.h>
#include <string.h>
#include <limits.h>

struct mh_entry {
	struct mh_entry *prev;
	struct mh_entry *next;
	char data[0];
};

typedef struct mh_entry MH_ENT;

typedef void (mh_del_fn)(void * k, void *v);
typedef unsigned long int (mh_hash_fn)(const void *k);
typedef int (mh_equals_fn)(const void *k1, const void *k2);

struct mh_hooks {
	mh_del_fn *del;
	mh_hash_fn *hash;
	mh_equals_fn *equals;
};

struct mh {
	double load;
	unsigned int cap;
	unsigned int len;
	unsigned int keysz;
	unsigned int valsz;
	struct mh_hooks hooks;
	struct mh_entry **table;
	struct mh_entry *first;
};

typedef struct mh MH;

#define mh_cap(T) ((T)->cap)
#define mh_len(T) ((T)->len)
#define mh_key(T, E) ((void*)((E)->data))
#define mh_val(T, E) ((void*)((E)->data + (T)->valsz))
#define mh_first(T)	((T)->first)
#define mh_next(E)	((E)->next)

MH *mh_new(unsigned int keysz, unsigned int valsz,
		unsigned int initcap, const struct mh_hooks *hooks);
MH *mh_strk_new(unsigned int valsz, unsigned int initcap, mh_del_fn *del);
MH *mh_ptrk_new(unsigned int valsz, unsigned int initcap, mh_del_fn *del);
void mh_free(MH *t);
void mh_clear(MH *t);
MH_ENT *mh_get(MH *t, const void *k);
MH_ENT *mh_put(MH *t, const void *k, const void *v);
void mh_del(MH *t, MH_ENT *ent);
int mh_realloc(MH *t, unsigned int cap);

#endif
