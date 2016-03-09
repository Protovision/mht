/*****************************************************************************
 * mht.c
 * Version 1.0
 * Mark's hashtable implementation using separate chaining
 * Mark Swoope (markswoope@outlook.com)
 * February 8, 2016
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
#include "mht.h"

struct mht *mht_new(size_t initial_capacity, const struct mht_hooks *hooks)
{
	struct mht *t;

	t = (struct mht*)malloc(sizeof(struct mht));
	if (!t) return 0;
	t->table = (struct mht_ent**)calloc(initial_capacity,
		sizeof(struct mht_ent*));
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
 * mht_strk_hash and mht_ptrk_hash functions taken from
 * linkhash.c v 1.6 2006/01/26
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 * Copyright (c) 2009 Hewlett-Packard Development Company, L.P.
 */

 /* a simple hash function similiar to what perl does for strings.
  * for good results, the string should not be excessivly large.
  */
static unsigned long int mht_strk_hash(const void *k)
{
	const char *rkey = (char*)k;
	unsigned long int hashval = 1;

	while (*rkey)
		hashval = hashval * 33 + *rkey++;
	
	return hashval;
}

static unsigned long int mht_ptrk_hash(const void *k)
{
	/* CAW: refactored to be 64bit nice */
	return (unsigned long)(
		(((ptrdiff_t)k * 0x9e370001UL) >> 4) & ULONG_MAX);
}
/*****************************************************************************/

static int mht_strk_equals(const void *k1, const void *k2)
{
	return (strcmp(k1, k2) == 0);
}

static int mht_ptrk_equals(const void *k1, const void *k2)
{
	return (k1 == k2);
}

struct mht *mht_strk_new(size_t initial_capacity, mht_free_fn *free_fn)
{
	struct mht_hooks hooks;

	hooks.free = free_fn;
	hooks.hash = mht_strk_hash;
	hooks.equals = mht_strk_equals;
	return mht_new(initial_capacity, &hooks);
}

struct mht *mht_ptrk_new(size_t initial_capacity, mht_free_fn *free_fn)
{
	struct mht_hooks hooks;

	hooks.free = free_fn;
	hooks.hash = mht_ptrk_hash;
	hooks.equals = mht_ptrk_equals;
	return mht_new(initial_capacity, &hooks);
}

static struct mht_ent *mht_search_bucket(struct mht *t, size_t idx, void *k)
{
	struct mht_ent *e;

	for (e = t->table[idx]; e; e = e->next) {
		if (t->hooks.equals(e->k, k)) return e;
	}
	return 0;
}

int mht_get(struct mht *t, void *k, void **v)
{
	size_t idx;
	struct mht_ent *e;

	idx = t->hooks.hash(k) % t->capacity;
	e = mht_search_bucket(t, idx, k);
	if (!e) return -1;
	*v = e->v;
	return 0;
}

int mht_set(struct mht *t, void *k, void *v, int overwrite)
{
	size_t idx;
	struct mht_ent *e;

	if (t->size >= t->capacity * t->load_factor) {
		if (mht_rehash(t, t->capacity * 2)) return -1;
	}
	idx = t->hooks.hash(k) % t->capacity;
	e = mht_search_bucket(t, idx, k);
	if (e) {
		if (!overwrite) return 0;
		if (t->hooks.free) t->hooks.free(e->k, e->v);
		e->k = k;
		e->v = v;
		return 0;
	}
	e = (struct mht_ent*)malloc(sizeof(struct mht_ent));
	if (!e) return -1;
	e->k = k;
	e->v = v;
	e->prev = 0;
	e->next = t->table[idx];
	if (e->next) {
		e->next->prev = e;
	}
	t->table[idx] = e;
	++t->size;
	return 0;
}

void mht_delete(struct mht *t, void *k)
{
	size_t idx;
	struct mht_ent *e;

	idx = t->hooks.hash(k) % t->capacity;
	e = mht_search_bucket(t, idx, k);
	if (!e) return;
	if (t->hooks.free) t->hooks.free(e->k, e->v);
	if (e->next) e->next->prev = e->prev;
	if (e->prev) e->prev->next = e->next;
	else t->table[idx] = e->next;
	--t->size;
}

int mht_rehash(struct mht *t, size_t new_capacity)
{
	size_t i, old_capacity;
	struct mht_ent *ent, *next_ent;
	struct mht_ent **old_table, **new_table;

	old_table = t->table;
	new_table = (struct mht_ent**)calloc(new_capacity,
		sizeof(struct mht_ent*));
	if (!new_table) return -1;
	t->table = new_table;
	old_capacity = t->capacity;
	t->capacity = new_capacity;
	for (i = 0; i < old_capacity; ++i) {
		if (!old_table[i]) continue;
		for (ent = old_table[i]; ent; ent = next_ent) {
			next_ent = ent->next;
			if (mht_set(t, ent->k, ent->v, 1) < 0) return -1;
			free(ent);
		}
	}
	free(old_table);
	return 0;
}

void mht_free(struct mht *t)
{
	size_t i;
	struct mht_ent *e, *next_e;

	for (i = 0; i < t->capacity; ++i) {
		if (!t->table[i]) continue;
		for (e = t->table[i]; e; e = next_e) {
			next_e = e->next;
			if (t->hooks.free) t->hooks.free(e->k, e->v);
			free(e);
		}
	}
	free(t->table);
	free(t);
}
