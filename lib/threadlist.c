//
// Created by Matteo on 06/07/2019.
//

#include "threadlist.h"
#include <stdlib.h>
#include <pthread.h>

void addThread(thread_list** list, pthread_t new_thread_id){
	thread_list* new = (thread_list*) malloc(sizeof(thread_list));
	new->thread_id = new_thread_id;
	new->next = *list;
	*list = new;
}

pthread_t removeThread(thread_list** list){
	if(*list != NULL){
		if((*list)->next){
			pthread_t tid;
			thread_list* tmp = *list;

			*list = (*list)->next;
			tid = tmp->thread_id;
			free(tmp);

			return tid;
		} else{
			*list = NULL;
		}
	}
	return 0;
}