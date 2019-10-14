//
// Created by Matteo on 26/06/2019.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "lib/os_access.h"


int main(int argc, char* argv[]){

	if(argc != 3){
		return -1;
	}

	int tcase = atoi(argv[2]);
	int i, len, successful;
	int n = 20;
	char path[256];
	char name[8];
	FILE *fp;

	int res;

	switch(tcase){
		case 1:
			if(os_connect(argv[1]) != 1){
				return -1;
			}

			successful = 0;
			memset(path, 0, 256);
			memset(name, 0, 8);

			for(i=1 ; i<=n ; i++){
				sprintf(path, "os_client/testfile/file%d", i);
				sprintf(name, "file%d", i);
				fp = fopen(path, "r");
				if(fp == NULL){
					fprintf(stderr, "Durante apertura %s : %s", path, strerror(errno));
					break;
				}

				// Misura dimensione file
				fseek(fp, 0, SEEK_END);
				len = ftell(fp);
				rewind(fp);

				// Scrittura blocco su buffer
				char block[len+1];
				memset(block, 0, len+1);
				if(fread(block, len, 1, fp) != 1){
					perror("read file in case 1");
				}
				fclose(fp);
				if(os_store(name, block, len) == 1){
					successful++;
				}
			}

			printf("Client %s: issued %d os_store, %d successful\n", argv[1], n, successful);
			break;

		case 2:
			if(os_connect(argv[1]) != 1){
				return -1;
			}

			char* block;

			successful = 0;
			memset(path, 0, 256);
			memset(name, 0, 8);

			for(i=1 ; i<=n ; i++) {
				sprintf(path, "os_client/testfile/file%d", i);
				sprintf(name, "file%d", i);
				fp = fopen(path, "r");
				if(fp == NULL){
					fprintf(stderr, "Durante apertura %s : %s", path, strerror(errno));
					break;
				}

				// Misura dimensione file
				fseek(fp, 0, SEEK_END);
				len = ftell(fp);
				rewind(fp);

				char *tfileptr;
				tfileptr = calloc((len+1), sizeof(char));

				if(fread(tfileptr, len, 1, fp) != 1){
					free(tfileptr);
					fclose(fp);
					break;
				}
				fclose(fp);
				block = os_retrieve(name);
				if(block != NULL){
					//printf("blocco %d, lunghezza %d:\n", i, (int) strlen(block));
					//puts(block);
					res = strncmp(block, tfileptr, len);

					if(res == 0){
						successful++;
					}
					free(tfileptr);
					free(block);
				}
			}

			printf("Client %s: issued %d os_retrieve, %d successful\n", argv[1], n, successful);
			break;

		case 3:
			if(os_connect(argv[1]) != 1){
				return -1;
			}

			successful = 0;
			memset(name, 0, 8);

			for(i=1 ; i<=n ; i++){
				sprintf(name, "file%d", i);

				if(os_delete(name) == 1){
					successful++;
				}
			}

			printf("Client %s: issued %d os_delete, %d successful\n", argv[1], n, successful);
			break;

		default:
			return -1;
			break;
	}

	os_disconnect();

	return 0;
}