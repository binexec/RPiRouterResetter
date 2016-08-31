#ifndef NETWORK_RESET_LOGGING_H
#define NETWORK_RESET_LOGGING_H

#define LOG_FILENAME			"events.log"		//Filename of the log
//#define LOG_MAX_EVENTS	1024				//Maximum number of lines of events in the log file. UNIMPLEMENTED.

void printTimestamp(FILE *stream);
void writeEventToLog(char *msg);

#endif
