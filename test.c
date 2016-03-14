#include "mh.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

void free_table_entry(struct mh_entry *e)
{
	free(e->k);
	free(e->v);
}

char *strclone(const char *str)
{
	char *p;

	p = (char*)malloc(strlen(str)+1);
	if (!p) return 0;
	strcpy(p, str);
	return p;
}

void populate_table(struct mh *table)
{
	FILE *f;
	static char line[512];
	char *rank, *state, *postal, *pop;

	f = fopen("2014_usa_states.csv", "rb");
	fgets(line, 512, f);
	for (;;) {
		if (!fgets(line, 512, f)) break;
		rank = strtok(line, ",\r\n");
		state = strtok(0, ",\r\n");
		postal = strtok(0, ",\r\n");
		pop = strtok(0, ",\r\n");
		if (state == 0 || pop == 0) break;
		mh_put(table, strclone(state), strclone(pop));
	}
	fclose(f);
}

void test_size(struct mh *t)
{
	assert(mh_size(t) == 52);
}

void test_capacity(struct mh *t)
{
	assert(mh_capacity(t) >= t->size * t->load_factor);
}

void test_get(struct mh *t)
{
	struct mh_entry *e;

	e = mh_get(t, "Florida");
	assert(e != 0);
	assert(strcmp((char*)e->k, "Florida") == 0);
	assert(strcmp((char*)e->v, "19893297.0") == 0);

	e = mh_get(t, "California");
	assert(e != 0);
	assert(strcmp((char*)e->k, "California") == 0);
	assert(strcmp((char*)e->v, "38802500.0") == 0);

	e = mh_get(t, "Hawaii");
	assert(e != 0);
	assert(strcmp((char*)e->k, "Hawaii") == 0);
	assert(strcmp((char*)e->v, "1419561.0") == 0);
}

void test_put(struct mh *t)
{
	struct mh_entry *e;

	assert(mh_put(t, strclone("foo"), strclone("bar")) == 0);
	assert(mh_put(t, strclone("horse"), strclone("shoe")) == 0);
	assert(mh_put(t, strclone("male"), strclone("female")) == 0);
	e = mh_get(t, "horse");
	assert(e != 0);
	assert(strcmp((char*)e->k, "horse") == 0);
	assert(strcmp((char*)e->v, "shoe") == 0);
}

void test_delete(struct mh *t)
{
	struct mh_entry *e;

	e = mh_get(t, "Michigan");
	assert(e != 0);
	mh_delete(t, e);
	e = mh_get(t, "Michigan");
	assert(e == 0);
	e = mh_get(t, "California");
	assert(e != 0);
	mh_delete(t, e);
	e = mh_get(t, "California");
	assert(e == 0);
}

void test_clear(struct mh *t)
{
	struct mh_entry *e;

	mh_clear(t);
	assert(mh_size(t) == 0);
	e = mh_get(t, "New York");
	assert(e == 0);
}

int main()
{
	struct mh *t;
	struct mh_entry *e;

	t = mh_strk_new(8, free_table_entry);
	populate_table(t);

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
