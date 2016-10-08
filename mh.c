#include "mh.h"

MH *mh_new(unsigned int keysz, unsigned int valsz,
	unsigned int initcap, const struct mh_hooks *hooks)
{
	struct mh *t;

	t = (struct mh*)malloc(sizeof(struct mh));
	if (!t) return 0;
	t->table = (struct mh_entry**)malloc(initcap * sizeof(struct mh*));
	if (!t->table) return 0;
	memset(t->table, 0, initcap * sizeof(struct mh*));
	t->load = 0.66;
	t->cap = initcap;
	t->len = 0;
	t->keysz = keysz;
	t->valsz = valsz;
	t->hooks = *hooks;
	t->first = 0;
	return t;
}

/* mh_strkhash and mh_ptrk_hash taken from
 * linkhash.c v1.6 2006/01/26
 * Copright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 * Copyright (c) 2009 Hewlett-Packard Developement Companry, L.P.
 */

static unsigned long int mh_strk_hash(const void *k)
{
	const char *rkey = *(const char**)k;
	unsigned long int hashval = 1;

	while (*rkey)
		hashval = hashval * 33 + *rkey++;

	return hashval;
}

static unsigned long int mh_ptrk_hash(const void *k)
{
	return (unsigned long)(
		((*(const ptrdiff_t*)k * 0x9e370001UL) >> 4) & ULONG_MAX);
}

static int mh_strk_equals(const void *k1, const void *k2)
{
	return (strcmp(*(const char**)k1, *(const char**)k2) == 0);
}

static int mh_ptrk_equals(const void *k1, const void *k2)
{
	return (*(const void**)k1 == *(const void**)k2);
}

MH *mh_strk_new(unsigned int valsz, unsigned int initcap, mh_del_fn *del)
{
	struct mh_hooks hooks;
	hooks.del = del;
	hooks.hash = mh_strk_hash;
	hooks.equals = mh_strk_equals;
	return mh_new(sizeof(char*), valsz, initcap, &hooks);
}

MH *mh_ptrk_new(unsigned int valsz, unsigned int initcap, mh_del_fn *del)
{
	struct mh_hooks hooks;
	hooks.del = del;
	hooks.hash = mh_ptrk_hash;
	hooks.equals = mh_ptrk_equals;
	return mh_new(sizeof(void*), valsz, initcap, &hooks);
}

MH_ENT *mh_get(MH *t, const void *k)
{
	struct mh_entry **ep;

	ep = t->table + (t->hooks.hash(k) % t->cap);
	while (ep != t->table + t->cap) {
		if (*ep != 0 && t->hooks.equals(mh_key(t, *ep), k))
			return *ep;
		++ep;
	}
	return 0;
}

MH_ENT *mh_put(MH *t, const void *k, const void *v)
{
	struct mh_entry **ep, *e;

	if (t->len >= t->cap * t->load) {
		if (mh_realloc(t, t->cap * 2)) return 0;
	}
	ep = t->table + (t->hooks.hash(k) % t->cap);
	for (;;) {
		if (*ep == 0) break;
		else if (t->hooks.equals(mh_key(t, *ep), k)) {
			if (v) memcpy(mh_val(t, *ep), v, t->valsz);
			return *ep;
		}
		++ep;
	}
	e = (struct mh_entry*)malloc(sizeof(struct mh_entry) + t->keysz + t->valsz);
	if (!e) return 0;
	*ep = e;
	memcpy(mh_key(t, e), k, t->keysz);
	if (v) memcpy(mh_val(t, e), v, t->valsz);
	if (t->first) {
		e->prev = 0;
		e->next = t->first;
		t->first->prev = e;
		t->first = e;
	} else {
		e->prev = 0;
		e->next = 0;
		t->first = e;
	}
	++t->len;
	return e;
}

void mh_del(MH *t, MH_ENT *e)
{
	struct mh_entry **ep;

	ep = t->table + (t->hooks.hash(mh_key(t, e)) % t->cap);
	while (*ep != e) ++ep;
	*ep = 0;
	if (t->hooks.del) t->hooks.del(mh_key(t, e), mh_val(t, e));
	if (e->prev) e->prev->next = e->next;
	if (e->next) e->next->prev = e->prev;
	if (e == t->first) t->first = e->next;
	free(e);
	--t->len;
}

void mh_clear(MH *t)
{
	struct mh_entry *e, *next_e;
	for (e = t->first; e; e = next_e) {
		next_e = e->next;
		if (t->hooks.del) t->hooks.del(mh_key(t, e), mh_val(t, e));
		free(e);
	}
	t->first = 0;
	t->len = 0;
}

void mh_free(MH *t)
{
	mh_clear(t);
	free(t);
}

int mh_realloc(MH *t, unsigned int cap)
{
	struct mh_entry **table, **ep, *e;

	table = (struct mh_entry**)malloc(cap * sizeof(struct mh_entry*));
	if (!table) return -1;
	free(t->table);
	t->table = table;
	t->cap = cap;
	memset(table, 0, cap * sizeof(struct mh_entry*));
	for (e = t->first; e; e = e->next) {
		ep = table + (t->hooks.hash(mh_key(t, e)) % cap);
		while (*ep != 0) ++ep;
		*ep = e;
	}
	return 0;
}
