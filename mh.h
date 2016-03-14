/*****************************************************************************
 * mh.h 
 * Version 1.0
 * Mark's traversable hashtable using separate chaining
 * Mark Swoope (markswoope@outlook.com)
 * March 12, 2016
 *
 * Copyright (c) 2016, Mark Swoope
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *****************************************************************************/
#ifndef MARKS_HASHTABLE_H 
#define MARKS_HASHTABLE_H 

#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

struct mh_bucket;

struct mh_entry {
	struct mh_bucket *bucket;	// Each entry knows it's bucket owner
					// therefore entry pointers remain
					// valid even after a table rehash.
	void *k;		// Key.
	void *v;		// Value.
	struct mh_entry *prev;	// Previous entry.
	struct mh_entry *next;	// Next entry.
};

struct mh_bucket {
	struct mh_entry *entries;	// Linked-List of entries.
	struct mh_bucket *prev;		// Prev non-empty bucket.
	struct mh_bucket *next;		// Next non-empty bucket.
};

// Called every time an entry is being deleted
// This function should only touch the k and v fields
typedef void (mh_free_fn)(struct mh_entry *entry);

// Hash function returns unique value for k
typedef unsigned long int (mh_hash_fn)(const void *k);

// Returns non-zero if k1 and k2 are equal
typedef int (mh_equals_fn)(const void *k1, const void *k2);

struct mh_hooks {
	mh_free_fn *free;	// Free key and value within an entry.
	mh_hash_fn *hash;	// Hash function.
	mh_equals_fn *equals;	// Key equality function.
};

struct mh {
	double load_factor;		// Table is reallocated and rehashed
					// when size>=capacity*load_factor.
	unsigned int capacity;		// Number of buckets.
	unsigned int size;		// Number of entries.
	struct mh_bucket *table;	// The hashtable (array of buckets).
	struct mh_bucket *buckets;	// Linked-List of non-empty buckets.
	struct mh_hooks hooks;		// Hook functions.
};

#define mh_size(T)	((T)->size)
#define mh_capacity(T)	((T)->capacity)

// Create new hashtable
struct mh *mh_new(unsigned int initial_capacity, const struct mh_hooks *hooks);

// Create new hashtable with string keys
struct mh *mh_strk_new(unsigned int initial_capacity, mh_free_fn *free_fn);

// Create new hashtable with pointer keys
struct mh *mh_ptrk_new(unsigned int initial_capacity, mh_free_fn *free_fn);

// Clear hashtable
void mh_clear(struct mh *t);

// Free hashtable
void mh_free(struct mh *t);

// Find hashtable entry from key
struct mh_entry *mh_get(struct mh *t, void *k);

// Insert/Overwrite hashtable entry with key and value
int mh_put(struct mh *t, void *k, void *v);

// Delete hashtable entry
void mh_delete(struct mh *t, struct mh_entry *entry);

// Reallocate and rehash hashtable
int mh_realloc(struct mh *t, unsigned int new_capacity);

// Hashtable traversal callback.
// Returns non-zero to stop traversing early
typedef int (mh_traverse_fn)(struct mh *t, struct mh_entry *entry,
	void *udata);

// Traverse all entries in hashtable
int mh_traverse(struct mh *t, mh_traverse_fn *callback, void *udata);

// Walk through hashtable entries incrementally
struct mh_entry *mh_first(struct mh *t);
struct mh_entry *mh_next(struct mh_entry *e);

#endif

