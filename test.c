#include "mh.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

void free_table_entry(void *k, void *v)
{
	free(*(char**)k);
	free(*(char**)v);
}

char *strclone(const char *str)
{
	char *p;

	p = (char*)malloc(strlen(str)+1);
	if (!p) return 0;
	strcpy(p, str);
	return p;
}

void populate_table(MH *table)
{
	FILE *f;
	static char line[512];
	char *rank, *state, *postal, *pop, *k, *v;

	printf("Populating table...\n");
	f = fopen("2014_usa_states.csv", "rb");
	fgets(line, 512, f);
	for (;;) {
		if (!fgets(line, 512, f)) break;
		rank = strtok(line, ",\r\n");
		state = strtok(0, ",\r\n");
		postal = strtok(0, ",\r\n");
		pop = strtok(0, ",\r\n");
		if (state == 0 || pop == 0) break;
		k = strclone(state);
		v = strclone(pop);
		mh_put(table, &k, &v);
	}
	fclose(f);
}

void test_traversal(MH *t)
{
	unsigned int n;
	MH_ENT *e;
	
	printf("Testing traversal...\n");
	n = 0;
	e = mh_first(t);
	assert(e != 0);
	while (e) {
		printf("%s -> %s\n", *(char**)mh_key(t, e), *(char**)mh_val(t, e));
		e = mh_next(e);
		++n;
		if (!e) break;
	}
	assert(n == 52);
}

void test_size(MH *t)
{
	printf("Testing len...\n");
	assert(mh_len(t) == 52);
}

void test_capacity(MH *t)
{
	printf("Testing capacity...\n");
	assert(mh_cap(t) >= t->len * t->load);
}

void test_get(MH *t)
{
	MH_ENT *e;
	const char *key;

	printf("Testing get...\n");
	key = "Florida";
	e = mh_get(t, &key);
	assert(e != 0);
	assert(strcmp(*(char**)mh_key(t, e), "Florida") == 0);
	assert(strcmp(*(char**)mh_val(t, e), "19893297.0") == 0);

	key = "California";
	e = mh_get(t, &key);
	assert(e != 0);
	assert(strcmp(*(char**)mh_key(t, e), "California") == 0);
	assert(strcmp(*(char**)mh_val(t, e), "38802500.0") == 0);

	key = "Hawaii";
	e = mh_get(t, &key);
	assert(e != 0);
	assert(strcmp(*(char**)mh_key(t, e), "Hawaii") == 0);
	assert(strcmp(*(char**)mh_val(t, e), "1419561.0") == 0);
}

void test_put(MH *t)
{
	MH_ENT *e;
	const char *k, *v;

	printf("Testing put...\n");
	k = strclone("foo");
	v = strclone("bar");
	assert(mh_put(t, &k, &v));
	k = strclone("horse");
	v = strclone("shoe");
	assert(mh_put(t, &k, &v));
	k = strclone("male");
	v = strclone("female");
	assert(mh_put(t, &k, &v));
	k = "horse";
	e = mh_get(t, &k); 
	assert(e != 0);
	assert(strcmp(*(char**)mh_key(t, e), "horse") == 0);
	assert(strcmp(*(char**)mh_val(t, e), "shoe") == 0);
}

void test_delete(MH *t)
{
	MH_ENT *e;
	const char *k;
	
	printf("Testing delete...\n");
	k = "Michigan";
	e = mh_get(t, &k);
	assert(e != 0);
	mh_del(t, e);
	e = mh_get(t, &k);
	assert(e == 0);
	k = "California";
	e = mh_get(t, &k);
	assert(e != 0);
	mh_del(t, e);
	e = mh_get(t, &k);
	assert(e == 0);
}

void test_clear(MH *t)
{
	MH_ENT *e;
	const char *k;

	printf("Testing clear...\n");
	mh_clear(t);
	assert(mh_len(t) == 0);
	k = "New York";
	e = mh_get(t, &k);
	assert(e == 0);
}

int main()
{
	MH *t;

	t = mh_strk_new(sizeof(char*), 256, free_table_entry);
	populate_table(t);

	test_traversal(t);
	test_size(t);
	test_capacity(t);
	test_get(t);
	test_put(t);
	test_delete(t);
	test_clear(t);

	mh_free(t);
	printf("All tests passed.\n");

	return 0;
}
