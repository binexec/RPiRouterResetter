#include <stdio.h>
#include <time.h>
#include "logging.h"

void printTimestamp(FILE *stream)
{
	time_t tm;
	struct tm* tm_info;
	char tm_str[20];

	time(&tm);
	tm_info = localtime(&tm);
	strftime(tm_str, 20, "%m-%d-%Y %H:%M:%S", tm_info);
	
	fprintf(stream, "%s ", tm_str);
}

void writeEventToLog(char *msg)
{
	//Open the log file
	FILE *logfile = fopen(LOG_FILENAME , "a");
	
	if(logfile == NULL)
	{
		printf("Failed to log event \"%s\" to %s", msg, LOG_FILENAME);
		perror("");
	}
	
	//Write the event to log
	printTimestamp(logfile);
	fprintf(logfile, " %s", msg);
	
	//Close the log file
	fclose(logfile);
}