/*
Sean Bjornsson

hash table consists of an array of bucket structures, which in turn contain a linked list and a mutex.
all bucket operations are locked with their own mutex.

hash function pulled from google and modified.

*/

#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

//initialize a bucket in the hashtable
void bucket_init(bucket *list){
	list->head=NULL;
	list->mutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(list->mutex,NULL);
}

//add string to a given bucket
void bucket_insert(const char *string, bucket *list) {
	pthread_mutex_lock(list->mutex);
    struct node *newnode = malloc(sizeof(struct node));
    newnode->string = strdup(string);
    newnode->next = list->head;
    list->head = newnode;
	pthread_mutex_unlock(list->mutex);
}

//remove first bucket entry containing string, if such a bucket exists.
void bucket_remove(const char *string, bucket *list){
	pthread_mutex_lock(list->mutex);
	if(list->head!=NULL && strcmp(list->head->string,string)==0){//if the first node is a match, remove and return
		struct node * tmp = list->head;
		list->head = list->head->next;
		free(tmp->string);
		free(tmp);
		pthread_mutex_unlock(list->mutex);
		return;
	}
	struct node *n = list->head;
	if(n->next == NULL){										//if there is no second node, return
		pthread_mutex_unlock(list->mutex);
		return;
	}
	while (n->next!=NULL){
		if(strcmp(n->next->string, string)==0){					//walk through the list looking for string. remove and return if found.
			struct node * tmp =n->next;
			n->next=n->next->next;
			free(tmp->string);
			free(tmp);
			pthread_mutex_unlock(list->mutex);
			return;
		}
		else
			n=n->next;
	}	
	pthread_mutex_unlock(list->mutex);
	return;
}

//print all contents of given bucket    
void bucket_print(bucket *list) {
	pthread_mutex_lock(list->mutex);    
	struct node *temp = list->head;
    while (temp != NULL) {
        printf("%s\n", temp->string);
        temp = temp->next;
    }
	pthread_mutex_unlock(list->mutex);
}

//clear all contents of given bucket    
void bucket_clear(bucket *list) {
	pthread_mutex_lock(list->mutex);
	struct node *n = list->head;
    while (n != NULL) {
    	struct node *tmp = n;
    	n = n->next;
		free(tmp->string);
    	free(tmp);
    }
	pthread_mutex_unlock(list->mutex);
	pthread_mutex_destroy(list->mutex);
	free(list->mutex);
	free(list);
}

// return bucket index based on string key
//written by Justin Sobel with some modifications
unsigned int gethash(const char* str, unsigned int len){
	unsigned int hash = 1315423911;
   	int i= 0;
	int temp =0;
	for(;i<strlen(str);i++){
		temp+=str[i];
	}
   	for(i = 0; i < len; str++, i++)
   	{
   	   hash ^= ((hash << 5) + (temp) + (hash >> 2));
   	}
   	return hash%len;
}

// create a new hashtable; parameter is a size hint
hashtable_t *hashtable_new(int sizehint) {
	hashtable_t *newhashtable = malloc(sizeof(hashtable_t));
	newhashtable->table = malloc(sizeof(bucket*)*sizehint);
	newhashtable->length = sizehint;
	int i = 0;
	for(;i<sizehint;i++){
		(newhashtable->table)[i] = NULL;
	}	
    return newhashtable;
}

// free anything allocated by the hashtable library
void hashtable_free(hashtable_t *hashtable) {
	int i = 0;
	for(;i<hashtable->length;i++)
		if((hashtable->table)[i]!=NULL) 
			bucket_clear((hashtable->table)[i]);
}

// add a new string to the hashtable
void hashtable_add(hashtable_t *hashtable, const char *s) {
	int hash = gethash(s,hashtable->length);
	if((hashtable->table)[hash] ==NULL){
		(hashtable->table)[hash] = malloc(sizeof(bucket));
		bucket_init((hashtable->table)[hash]);
	}
	bucket_insert(s,(hashtable->table)[hash]);
}

// remove a string from the hashtable; if the string
// doesn't exist in the hashtable, do nothing
void hashtable_remove(hashtable_t *hashtable, const char *s) {
	int hash = gethash(s,hashtable->length);
	if((hashtable->table)[hash]==NULL) return;
	else{
		bucket_remove(s,(hashtable->table)[hash]);
	}
}

// print the contents of the hashtable
void hashtable_print(hashtable_t *hashtable){
	int i = 0;
	for(;i<hashtable->length;i++)
		if((hashtable->table)[i]!=NULL) 
			bucket_print((hashtable->table)[i]);
}

