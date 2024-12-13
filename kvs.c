#include "kvs.h"
#include "string.h"

#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>

// Hash function based on key initial.
// @param key Lowercase alphabetical string.
// @return hash.
// NOTE: This is not an ideal hash function, but is useful for test purposes of the project
int hash(const char *key) {
    int firstLetter = tolower(key[0]);
    if (firstLetter >= 'a' && firstLetter <= 'z') {
        return firstLetter - 'a';
    } else if (firstLetter >= '0' && firstLetter <= '9') {
        return firstLetter - '0';
    }
    return -1; 
}

// Added a read-write lock for each keynode on the hashTable
struct HashTable* create_hash_table() {
    HashTable *ht = malloc(sizeof(HashTable));
    if (!ht) return NULL;

    for (int i = 0; i < TABLE_SIZE; i++) {
        ht->table[i] = NULL;
        pthread_rwlock_init(&ht->locks[i], NULL); 
    }

    return ht;
}

// Read-write Locks and Unlocks added to critical zones
int write_pair(HashTable *ht, const char *key, const char *value) {
    int index = hash(key);
    pthread_rwlock_wrlock(&ht->locks[index]); 

    KeyNode *keyNode = ht->table[index];

    
    while (keyNode != NULL) {
        if (strcmp(keyNode->key, key) == 0) {
            free(keyNode->value);
            keyNode->value = strdup(value);
            pthread_rwlock_unlock(&ht->locks[index]);
            return 0;
        }
        keyNode = keyNode->next;
    }

    
    keyNode = malloc(sizeof(KeyNode));
    keyNode->key = strdup(key); 
    keyNode->value = strdup(value); 
    keyNode->next = ht->table[index];
    ht->table[index] = keyNode; 

    pthread_rwlock_unlock(&ht->locks[index]);
    return 0;
}

char* read_pair(HashTable *ht, const char *key) {
    int index = hash(key);
    pthread_rwlock_rdlock(&ht->locks[index]);

    KeyNode *keyNode = ht->table[index];
    char* value;

    while (keyNode != NULL) {
        if (strcmp(keyNode->key, key) == 0) {
            value = strdup(keyNode->value);
            pthread_rwlock_unlock(&ht->locks[index]); 
            return value; 
        }
        keyNode = keyNode->next; 
    }

    pthread_rwlock_unlock(&ht->locks[index]); 
    return NULL; 
}

int delete_pair(HashTable *ht, const char *key) {
    int index = hash(key);
    pthread_rwlock_wrlock(&ht->locks[index]); 

    KeyNode *keyNode = ht->table[index];
    KeyNode *prevNode = NULL;

    
    while (keyNode != NULL) {
        if (strcmp(keyNode->key, key) == 0) {
            
            if (prevNode == NULL) {
                
                ht->table[index] = keyNode->next; 
            } else {
                
                prevNode->next = keyNode->next; 
            }
            
            free(keyNode->key);
            free(keyNode->value);
            free(keyNode); 

            pthread_rwlock_unlock(&ht->locks[index]); 
            return 0;
        }
        prevNode = keyNode; 
        keyNode = keyNode->next; 
    }

    pthread_rwlock_unlock(&ht->locks[index]); 
    return 1;
}

void free_table(HashTable *ht) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        pthread_rwlock_wrlock(&ht->locks[i]); 

        KeyNode *keyNode = ht->table[i];
        while (keyNode != NULL) {
            KeyNode *temp = keyNode;
            keyNode = keyNode->next;
            free(temp->key);
            free(temp->value);
            free(temp);
        }

        pthread_rwlock_unlock(&ht->locks[i]); 
        pthread_rwlock_destroy(&ht->locks[i]); 
    }

    free(ht);
}
