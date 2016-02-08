#ifndef MINIMAL_HASHTABLE_H 
#define MINIMAL_HASHTABLE_H 

#include <stddef.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

struct mht_ent {
	void		*k;
	void		*v;
	struct mht_ent	*next;
	struct mht_ent	*prev;
};

typedef void			(mht_free_fn)( void *k, void *v );
typedef unsigned long int	(mht_hash_fn)( const void *k );
typedef int			(mht_equals_fn)( const void *k1, const void *k2 );

struct mht {
	double		load_factor;
	size_t		capacity;
	size_t		size;
	struct mht_ent	**table;
	mht_free_fn	*free_fn;
	mht_hash_fn	*hash_fn;
	mht_equals_fn	*equals_fn;
};

#define mht_size(T)	((T)->size)
#define mht_capacity(T)	((T)->capacity)

struct mht	*mht_new( size_t initial_capacity, mht_free_fn *free_fn, mht_hash_fn *hash_fn, mht_equals_fn *equals_fn );
struct mht	*mht_strk_new( size_t initial_capacity, mht_free_fn *free_fn );
struct mht	*mht_ptrk_new( size_t initial_capacity, mht_free_fn *free_fn );
void		mht_free( struct mht *t );
int		mht_set( struct mht *t, void *k, void *v, int overwrite );
int		mht_get( struct mht *t, void *k, void **v );
void		mht_delete( struct mht *t, void *k );
int		mht_rehash( struct mht *t, size_t new_capacity );

#endif

