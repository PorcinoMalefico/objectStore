//
// Created by Matteo on 06/07/2019.
//

#include <pthread.h>

typedef struct node{
	pthread_t thread_id;
	struct node* next;
} thread_list;

void addThread(thread_list** list, pthread_t new_thread_id);

pthread_t removeThread(thread_list** list);

