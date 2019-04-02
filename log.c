#include "log.h"

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

struct timeval start_time;
FILE * myLog = NULL;

int initialize_log(){
	/* Get log_file name defined in the env variables */
	char *log_file = getenv("LOGFILENAME");

	if(log_file == NULL){
		fprintf(stderr, "LOGFILENAME not defined in env variables\n");
		return -1;
	}

	/* Open the log file */
	if((myLog = fopen(log_file, "a")) == NULL){
		fprintf(stderr, "Error opening logfile\n");
		return -2;
	}

	/* Initialize start time */
	gettimeofday(&start_time, NULL);

	return 0;
}

void write_in_log(char * act){
	struct timeval time;
	/* Get current time */
	gettimeofday(&time, NULL);

	double real_time = (double) (time.tv_usec - start_time.tv_usec) / 1000000 + (double) (time.tv_sec - start_time.tv_sec);

	/* Print to log if it was initilized */
	if(myLog != NULL){
		// Anchor Left
		fprintf(myLog, "%-8.2f - %-8d - %s\n", real_time * 1000, getpid(), act);
		// Anchor Right
		// fprintf(myLog, "%*.2f - %*d - %s\n", 8, real_time * 1000, 8, getpid(), act);
		fflush(myLog);
	}
}

int close_log(){
	/* Close log file */
	if(fclose(myLog) != 0){
		perror("close_log");
		return -1;
	}
	return 0;
}
