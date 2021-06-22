
// JULIO 2020. CÉSAR BORAO MORATINOS: uniqfiles.c

#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

enum {
	Max = 100,
	Max_list = 500,
};

struct Cell {
	int pid;
	char compword[Max];
	int status;
};

typedef struct Cell Cell;

int
addtolist(int pid, char *word, Cell *list) {

	int i = 0;
	while (strcmp(list[i].compword,"\0") != 0) {
		i++;
	}

	if (i < Max_list) {
		strncpy(list[i].compword,word,strlen(word)+1);
		list[i].pid = pid;
		list[i+1].compword[0] = '\0';
		return 1;
	}
	return -1;
}

int
addpidstatus(int pid, int status, Cell *list) {
	int i = 0;
	while (strcmp(list[i].compword,"\0") != 0) {
		if (pid == list[i].pid) {
			list[i].status = WEXITSTATUS(status);
			return 1;
		}
		i++;
	}
	return -1;
}

// if file is unique return 1, if not return -1;
int
uniquefile(int w, int numofcomp, Cell *list){

	for (size_t j = w*numofcomp; j < numofcomp*(1+w); j++) {
		if (list[j].status == 0) {
			return -1;
		}
	}
	return 1;
}

// if an error in comparisons has ocurred, return -1. If not error, then return if
// all files are unique or not.

int
selectout(Cell *list, int argc){
	int i = 0;
	int unique = 1;

	while (strcmp(list[i].compword,"\0") != 0) {
		if (list[i].status == 2) {
			return -1;
		}
		i++;
	}

	int numofcomp = argc-1;
	for (size_t w = 0; w < argc; w++) {
		if (uniquefile(w,numofcomp,list) > 0) {
			fprintf(stdout,"%s\n",list[w*numofcomp].compword);
		} else {
			unique = 0;
		}
	}
	return unique;
}

void
runchild(char *argv[], int *pid, int i, int j) {

	char *input[] = {"/usr/bin/cmp","-s",argv[i],argv[j],NULL};
	switch (*pid = fork()) {
		case -1:
			errx(EXIT_FAILURE, "error: fork failed!");
		case 0:
			execv("/usr/bin/cmp",input);
			errx(EXIT_FAILURE, "error: execv failed!");
	}
}

void
runcomp(char *argv[], int argc, Cell *list) {

	int pid;
	int index = 0;
	for (size_t i = 0; i < argc; i++) {
		for (size_t j = 0; j < argc; j++) {
			if (j != i) {
				runchild(argv,&pid,i,j);
				addtolist(pid,argv[i],list);
				index++;
			}
		}
	}
}

void
loadstatus(Cell *list) {
	int status;
	int pid;
	while ((pid = wait(&status)) != -1) {
		if (!WIFEXITED(status))
			errx(EXIT_FAILURE, "child ends without exit() call");

		addpidstatus(pid,status,list);
	}
}

int
main(int argc, char *argv[]) {

	argv++;
	argc--;

	Cell list[Max_list];
	list[0].compword[0] = '\0';

	runcomp(argv,argc,list);
	loadstatus(list);

	switch (selectout(list,argc)) {
		case 0:
			exit(EXIT_FAILURE);
		case 1:
			exit(EXIT_SUCCESS);
		default:
			errx(EXIT_FAILURE,"usage: uniqfiles [files ...]");
	}
}
