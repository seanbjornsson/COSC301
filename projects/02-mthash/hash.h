#ifndef __HASH_H__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

//contains a linked list and a mutex
typedef struct{
    struct node *head;
	pthread_mutex_t *mutex;
} bucket;

typedef struct {
	bucket** table;
	int length;
} hashtable_t;

struct node {
    char *string;
    struct node *next; 
};



//initialize a bucket in the hashtable
void bucket_init(bucket *);

//add string to a given bucket
void bucket_insert(const char *, bucket *);

//remove first bucket entry containing string, if such a bucket exists.
void bucket_remove(const char *, bucket *);

//print all contents of given bucket    
void bucket_print(bucket *);

//clear all contents of given bucket    
void bucket_clear(bucket *);

// create a new hashtable; parameter is a size hint
hashtable_t *hashtable_new(int);

// free anything allocated by the hashtable library
void hashtable_free(hashtable_t *);

// add a new string to the hashtable
void hashtable_add(hashtable_t *, const char *);

// remove a string from the hashtable; if the string
// doesn't exist in the hashtable, do nothing
void hashtable_remove(hashtable_t *, const char *);

// print the contents of the hashtable
void hashtable_print(hashtable_t *);

// return bucket index based on string key
int hash(const char *, int);

#endif

