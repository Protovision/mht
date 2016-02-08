#include "mht.h"

struct mht *mht_new( size_t initial_capacity, mht_free_fn *free_fn, mht_hash_fn *hash_fn, mht_equals_fn *equals_fn )
{
	struct mht *t;

	t = (struct mht*)malloc( sizeof(struct mht) );
	if ( !t ) return 0;
	t->table = (struct mht_ent**)calloc( initial_capacity, sizeof(struct mht_ent*) );
	if ( !t->table ) {
		free( t );
		return 0;
	}
	t->load_factor = 0.66;
	t->capacity = initial_capacity;
	t->size = 0;
	t->free = free_fn;
	t->hash = hash_fn;
	t->equals = equals_fn;
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
static unsigned long int mht_strk_hash( const void *k )
{
	const char *rkey = (char*) k;
	unsigned long int hashval = 1;

	while (*rkey)
		hashval = hashval * 33 + *rkey++;
	
	return hashval;
}

static unsigned long int mht_ptrk_hash( const void *k )
{
	/* CAW: refactored to be 64bit nice */
	return (unsigned long)((((ptrdiff_t)k * 0x9e370001UL) >> 4) & ULONG_MAX);
}
/*****************************************************************************/

static int mht_strk_equals( const void *k1, const void *k2 )
{
	return ( strcmp(k1, k2) == 0 );
}

static int mht_ptrk_equals( const void *k1, const void *k2 )
{
	return ( k1 == k2 );
}

struct mht *mht_strk_new( size_t initial_capacity, mht_free_fn *free_fn )
{
	return mht_new( initial_capacity, free_fn, mht_strk_hash, mht_strk_equals );
}

struct mht *mht_ptrk_new( size_t initial_capacity, mht_free_fn *free_fn )
{
	return mht_new( initial_capacity, free_fn, mht_ptrk_hash, mht_ptrk_equals );
}

static struct mht_ent *mht_search_bucket( struct mht *t, size_t idx, void *k )
{
	struct mht_ent *e;

	for ( e = t->table[idx]; e; e = e->next ) {
		if ( t->equals(e->k, k) ) return e;
	}
	return 0;
}

int mht_get( struct mht *t, void *k, void **v )
{
	size_t idx;
	struct mht_ent *e;

	idx = t->hash(k) % t->capacity;
	e = mht_search_bucket( t, idx, k );
	if ( !e ) return -1;
	*v = e->v;
	return 0;
}

int mht_set( struct mht *t, void *k, void *v, int overwrite )
{
	size_t idx;
	struct mht_ent *e;

	if ( t->size >= t->capacity * t->load_factor ) {
		if ( mht_rehash(t, t->capacity * 2) ) return -1;
	}
	idx = t->hash(k) % t->capacity;
	e = mht_search_bucket( t, idx, k );
	if ( e ) {
		if ( !overwrite ) return 0;
		if ( t->free ) t->free( e->k, e->v );
		e->k = k;
		e->v = v;
		return 0;
	}
	e = (struct mht_ent*)malloc( sizeof(struct mht_ent) );
	if ( !e ) return -1;
	e->k = k;
	e->v = v;
	e->prev = 0;
	e->next = t->table[idx];
	if ( e->next ) {
		e->next->prev = e;
	}
	t->table[idx] = e;
	++t->size;
	return 0;
}

void mht_delete( struct mht *t, void *k )
{
	size_t idx;
	struct mht_ent *e;

	idx = t->hash(k) % t->capacity;
	e = mht_search_bucket( t, idx, k );
	if ( !e ) return;
	if ( t->free ) t->free( e->k, e->v );
	if ( e->next ) e->next->prev = e->prev;
	if ( e->prev ) e->prev->next = e->next;
	else t->table[idx] = e->next;
	--t->size;
}

int mht_rehash( struct mht *t, size_t new_capacity )
{
	size_t i;
	struct mht *nt;
	struct mht_ent *e;

	nt = (struct mht*)malloc( sizeof(struct mht) );
	if ( !nt ) return -1;
	nt->table = (struct mht_ent**)calloc( new_capacity, sizeof(struct mht_ent*) );
	if ( !nt->table ) {
		free( nt );
		return -1;
	}
	nt->load_factor = 0.66;
	nt->capacity = new_capacity;
	nt->size = 0;
	nt->free = t->free;
	nt->hash = t->hash;
	nt->equals = t->equals;
	for ( i = 0; i < t->capacity; ++i ) {
		if ( !t->table[i] ) continue;
		for ( e = t->table[i]; e; e = e->next ) {
			if ( mht_set(nt, e->k, e->v, 1) < 0 ) {
				mht_free( nt );
				return -1;
			}
		}
	}
	free( t );
	return 0;
}

void mht_free( struct mht *t )
{
	size_t i;
	struct mht_ent *e, *next_e;

	for ( i = 0; i < t->capacity; ++i ) {
		if ( !t->table[i] ) continue;
		for ( e = t->table[i]; e; e = next_e ) {
			next_e = e->next;
			if ( t->free ) t->free( e->k, e->v );
			free( e );
		}
	}
	free( t->table );
	free( t );
}
