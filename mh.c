/*****************************************************************************
 * mh.c
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
 ****************************************************************************/
#include "mh.h"

struct mh *mh_new(unsigned int initial_capacity, const struct mh_hooks *hooks)
{
	struct mh *t;

	t = (struct mh*)malloc(sizeof(struct mh));
	if (!t) return 0;
	t->table = (struct mh_bucket*)calloc(initial_capacity,
		sizeof(struct mh_bucket));
	if (!t->table) {
		free(t);
		return 0;
	}
	t->load_factor = 0.66;
	t->capacity = initial_capacity;
	t->size = 0;
	t->hooks = *hooks;
	return t;
}

/******************************************************************************
 * mh_strk_hash and mh_ptrk_hash functions taken from
 * linkhash.c v 1.6 2006/01/26
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 * Copyright (c) 2009 Hewlett-Packard Development Company, L.P.
 */

 /* a simple hash function similiar to what perl does for strings.
  * for good results, the string should not be excessivly large.
  */
static unsigned long int mh_strk_hash(const void *k)
{
	const char *rkey = (char*)k;
	unsigned long int hashval = 1;

	while (*rkey)
		hashval = hashval * 33 + *rkey++;
	
	return hashval;
}

static unsigned long int mh_ptrk_hash(const void *k)
{
	/* CAW: refactored to be 64bit nice */
	return (unsigned long)(
		(((ptrdiff_t)k * 0x9e370001UL) >> 4) & ULONG_MAX);
}
/*****************************************************************************/

static int mh_strk_equals(const void *k1, const void *k2)
{
	return (strcmp(k1, k2) == 0);
}

static int mh_ptrk_equals(const void *k1, const void *k2)
{
	return (k1 == k2);
}

struct mh *mh_strk_new(unsigned int initial_capacity, mh_free_fn *free_fn)
{
	struct mh_hooks hooks;

	hooks.free = free_fn;
	hooks.hash = mh_strk_hash;
	hooks.equals = mh_strk_equals;
	return mh_new(initial_capacity, &hooks);
}

struct mh *mh_ptrk_new(unsigned int initial_capacity, mh_free_fn *free_fn)
{
	struct mh_hooks hooks;

	hooks.free = free_fn;
	hooks.hash = mh_ptrk_hash;
	hooks.equals = mh_ptrk_equals;
	return mh_new(initial_capacity, &hooks);
}

static struct mh_entry *mh_search_bucket(struct mh *t,
	struct mh_bucket *bucket, void *k)
{
	struct mh_entry *e;

	for (e = bucket->entries; e; e = e->next) {
		if (t->hooks.equals(e->k, k)) return e;
	}
	return 0;
}

struct mh_entry *mh_get(struct mh *t, void *k)
{
	unsigned int idx;

	idx = t->hooks.hash(k) % t->capacity;
	return mh_search_bucket(t, &t->table[idx], k);
}

static void mh_put_entry(struct mh *t, struct mh_entry *e)
{
	struct mh_bucket *b;

	b = e->bucket;
	e->prev = 0;
	e->next = b->entries;
	b->entries = e;
	if (e->next) {
		e->next->prev = e;
	} else {
		b->next = t->buckets;
		t->buckets = b;
		if (b->next) {
			b->next->prev = b;
		}
	}
	++t->size;
}

int mh_put(struct mh *t, void *k, void *v)
{
	unsigned int idx;
	struct mh_entry *e;
	struct mh_bucket *b;

	if (t->size >= t->capacity * t->load_factor) {
		if (mh_rehash(t, t->capacity * 2)) return -1;
	}
	idx = t->hooks.hash(k) % t->capacity;
	b = &t->table[idx];
	e = mh_search_bucket(t, b, k);
	if (e) {
		if (t->hooks.free) t->hooks.free(e);
		e->k = k;
		e->v = v;
		return 0;
	}
	e = (struct mh_entry*)malloc(sizeof(struct mh_entry));
	if (!e) return -1;
	e->bucket = b;
	e->k = k;
	e->v = v;
	mh_put_entry(t, e);
	return 0;
}

void mh_delete(struct mh *t, struct mh_entry *e)
{
	struct mh_bucket *b;
	
	if (t->hooks.free) t->hooks.free(e);
	if (e->next) e->next->prev = e->prev;
	if (e->prev) e->prev->next = e->next;
	else {
		b = e->bucket;
		b->entries = e->next;
		if (!b->entries) {
			if (b->next) b->next->prev = b->prev;
			if (b->prev) b->prev->next = b->next;
			else {
				t->buckets = b->next;
			}
		}
	}
	free(e);
	--t->size;
}

int mh_rehash(struct mh *t, unsigned int new_capacity)
{
	struct mh_bucket *b;
	struct mh_entry *e, *next_e;
	struct mh_bucket *new_table, *old_table, *old_buckets;
	unsigned int idx;

	new_table = (struct mh_bucket*)calloc(new_capacity,
		sizeof(struct mh_bucket));
	if (!new_table) return -1;
	old_table = t->table;
	old_buckets = t->buckets;
	t->table = new_table;
	t->buckets = 0;
	t->size = 0;
	t->capacity = new_capacity;
	for (b = old_buckets; b; b = b->next) {
		for (e = b->entries; e; e = next_e) {
			next_e = e->next;
			idx = t->hooks.hash(e->k) % t->capacity;
			e->bucket = t->table + idx;
			mh_put_entry(t, e);
		}
	}
	free(old_table);
	return 0;
}

void mh_free(struct mh *t)
{
	struct mh_bucket *b;
	struct mh_entry *e, *next_e;

	for (b = t->buckets; b; b = b->next) {
		for (e = b->entries; e; e = next_e) {
			next_e = e->next;
			if (t->hooks.free) t->hooks.free(e);
			free(e);
		}
	}
	free(t->table);
	free(t);
}

int mh_traverse(struct mh *t, mh_traverse_fn *callback, void *udata)
{
	struct mh_bucket *b, *next_b;
	struct mh_entry *e, *next_e;
	int ret;

	for (b = t->buckets; b; b = next_b) {
		next_b = b->next;
		for (e = b->entries; e; e = next_e) {
			next_e = e->next;
			ret = callback(t, e, udata);
			if (ret) return ret;
		}
	}
	return 0;
}

struct mh_entry *mh_first(struct mh *t)
{
	if (!t->buckets) return 0;
	return t->buckets->entries;
}

struct mh_entry *mh_next(struct mh_entry *e)
{
	if (e->next) return e->next;
	if (e->bucket->next) 
		return e->bucket->next->entries;
	return 0;
}

struct mh_entry *mh_prev(struct mh_entry *e)
{
	struct mh_entry *ent, *ent_next;

	if (e->prev) return e->prev;
	if (e->bucket->prev) {
		for (ent = e->bucket->prev->entries; ent->next;
			ent = ent->next);
		return ent;
	}
	return 0;
}
