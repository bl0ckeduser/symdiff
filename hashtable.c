/*
 * (adapted from wannabe C compiler project) 
 */

/*
 * Hash table
 */

#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

/* ======================================================== */

/*
 * Hash function. mostly just random nonsense for now,
 * I am no expert on hash functions.
 *
 * mode 0: hash a string
 * mode 1: (incrementally) hash a single character
 * (this mode is so the tokenizer can hash and match in parallel
 * to save time. the tokenizer uses a hash table to distinguish
 * keywords like "int" from identifiers)
 */
int hashtab_hash_mode(char *key, int nbuck, int mode, char prev, int prev_hash);

int hashtab_hash(char *key, int nbuck)
{
	return hashtab_hash_mode(key, nbuck, 0, 0, 0);
}

int hashtab_hash_char(char c, char prev, int nbuck, int prev_hash)
{
	return hashtab_hash_mode(&c, nbuck, 1, prev, prev_hash);
}

int hashtab_hash_mode(char *key, int nbuck, int single_char_mode, char prev, int prev_hash)
{
	int hash = prev_hash;
	int prev_key = *key;
	if (single_char_mode)
		prev_key = prev;
	while (*key) {
		hash += abs(*key - prev_key) * 100;
		hash += (prev_key = *key);
		hash %= nbuck;
		if (single_char_mode)
			break;
		else
			++key;
	}
	return hash;
}

/* ======================================================== */

hashtab_t* new_hashtab(void)
{
	hashtab_t* htab;
	int i;

	htab = malloc(sizeof(hashtab_t));
	if (!htab)
		fail("couldn't allocate a hash table");

	htab->nbuck = 241;	/* prime */
	htab->buck = malloc(htab->nbuck * sizeof(hashtab_entry_t *));
	if (!(htab->buck))
		fail("couldn't allocate hash table buckets");
	for (i = 0; i < htab->nbuck; ++i)
		htab->buck[i] = NULL;

	return htab;
}

/* ======================================================== */

void* hashtab_lookup(hashtab_t* htab, char* key)
{
	return hashtab_lookup_with_hash(
			htab, 
			key, 
			hashtab_hash(key, htab->nbuck));
}

void* hashtab_lookup_with_hash(hashtab_t* htab, char* key, int hash)
{
	hashtab_entry_t *ptr = htab->buck[hash];

	while (ptr)
		if (!strcmp(ptr->key, key))
			break;
		else
			ptr = ptr->next;

	if (ptr && strcmp(ptr->key, key))
		return NULL;
	else if (ptr)
		return ptr->val;
	else
		return NULL;
}

/* ======================================================== */

void hashtab_pseudo_delete(hashtab_t *htab, char* key)
{
	int hash = hashtab_hash(key, htab->nbuck);
        hashtab_entry_t *ptr = htab->buck[hash];

        while (ptr)
                if (!strcmp(ptr->key, key))
                        break;
                else
                        ptr = ptr->next;

	if (ptr && !strcmp(ptr->key, key))
		*(ptr->key) = 0;
}

/* ======================================================== */

void hashtab_insert(hashtab_t* htab, char* key, void* val)
{
	int hash = hashtab_hash(key, htab->nbuck);
	hashtab_entry_t **ptr = &(htab->buck[hash]);

	while (*ptr)
		ptr = &((*ptr)->next);

	*ptr = malloc(sizeof(hashtab_entry_t));
	if (!*ptr)
		fail("couldn't create a hash table bucket node");
	(*ptr)->key = malloc(strlen(key) + 1);
	if (!((*ptr)->key))
		fail("couldn't allocate a string");
	strcpy((*ptr)->key, key);
	(*ptr)->val = val;
	(*ptr)->next = NULL;
}

/* ======================================================== */

