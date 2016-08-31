/*
	Compile:	gcc -o network_reset network_reset.c -lpigpio -lrt -lpthread
	Run:		sudo ./network_reset
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <pigpio.h>
#include "network_reset.h"

//Default values for user definable variables loaded from external config file
int net_check_period_std = NET_CHECK_PERIOD_STD;
int net_check_period_alt = NET_CHECK_PERIOD_ALT;
int power_cycle_time = POWER_CYCLE_TIME;
int logging_enabled = LOGGING_ENABLED;

//Pointer to log file
FILE *logfile;

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

void createCfgFile()
{
	FILE *cfg = fopen(CFG_FILENAME, "w");

	fprintf(cfg, "%s\n\n", CFG_HEADER);
	
	fprintf(cfg, "%s %d\n", NET_CHECK_PERIOD_STD_STR,  NET_CHECK_PERIOD_STD);
	fprintf(cfg, "%s %d\n", NET_CHECK_PERIOD_ALT_STR, NET_CHECK_PERIOD_ALT);
	fprintf(cfg, "%s %d\n", POWER_CYCLE_TIME_STR, POWER_CYCLE_TIME);
	fprintf(cfg, "%s %d\n", LOGGING_ENABLED_STR, LOGGING_ENABLED);
	
	fprintf(cfg, "\n#end of configuration file\n");

	fclose(cfg);
}

void parseCfgFile()
{
	char buf[CFG_MAX_LINE_CHAR];
	char *config_str;		//The string represent a variable to set from the line
	int config_val;		//The value associated with the variable 

	FILE *cfg = fopen(CFG_FILENAME, "r");	

	//Check if the configuration file already exists
	if(cfg == NULL)
	{
		printf("Configuration file \"%s\" does not exist. Creating a new one...\n", CFG_FILENAME);
		createCfgFile();
		return;
	}

	//Stop reading the cfg file if it's empty, or missing the correct header
	if(fgets(buf, CFG_MAX_LINE_CHAR, cfg) == NULL || strncmp(buf, CFG_HEADER, strlen(CFG_HEADER)) != 0)
	{
		printf("Configuration file does not seem to be valid. Using default values instead.\n");
		fclose(cfg);
		return;
	}

	//Parse the settings in the config file line by line
	while(fgets(buf, CFG_MAX_LINE_CHAR, cfg) != NULL)
	{
		//Ignore empty lines and comment lines
		if(buf[0] == '\n' || buf[0] == CFG_COMMENT_CHAR ) 
			continue;

		//Extract the variable name and associated value from the line
		config_str = strtok(buf, " \n\t");		
		config_val = atoi(strtok(NULL, " \n\t"));
		//printf("PARSED: %s,%d\n", config_str, config_val);
		
		//Attempt to match the string with a known command, and set its corresponding variable with the provided value
		//The corresponding numerical value is checked to ensure it's within valid range. If not, default values are used.
		if(strcmp(config_str, NET_CHECK_PERIOD_STD_STR) == 0)
		{
			if(config_val <= 0)
			{
				printf("Value for %s must be greater than 0. Using default (%d)\n", NET_CHECK_PERIOD_STD_STR,  NET_CHECK_PERIOD_STD);
				continue;
			}
			net_check_period_std = config_val;
		}
		
		else if(strcmp(config_str, NET_CHECK_PERIOD_ALT_STR) == 0)
		{
			if(config_val <= 0)
			{
				printf("Value for %s must be greater than 0.  Using default  (%d)\n", NET_CHECK_PERIOD_ALT_STR, NET_CHECK_PERIOD_ALT);
				continue;
			}
			net_check_period_alt = config_val;
		}
		
		else if(strcmp(config_str, POWER_CYCLE_TIME_STR) == 0)
		{
			if(config_val <= 0)
			{
				printf("Value for %s must be greater than 0.  Using default  (%d)\n", POWER_CYCLE_TIME_STR, POWER_CYCLE_TIME);
				continue;
			}
			power_cycle_time = config_val;
		}
		
		else if(strcmp(config_str, LOGGING_ENABLED_STR) == 0)
		{
			if(config_val < 0)
			{
				printf("Value for %s must be 0 or 1.  Using default  (%d)\n", LOGGING_ENABLED_STR, LOGGING_ENABLED);
				continue;
			}
			logging_enabled = config_val;
		}
		
		else
			printf("Unrecognized command \"%s\"\n", config_str);
	}

	fclose(cfg);
}

void writeEventToLog(char *msg)
{
	//Open the log file
	logfile = fopen(LOG_FILENAME , "a");
	
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

void sig_handler(int signo)
{
	switch(signo)
	{
		case SIGINT:
			printf("\n***SIGINT CAPTURED*** \n");
			if(logging_enabled)
				writeEventToLog("Received SIGINT\n");
			break;
			
		case SIGTERM:
			printf("\n***SIGTERM CAPTURED*** \n");
			if(logging_enabled)
				writeEventToLog("ReceivedSIGTERM\n");
			break;
			
		case SIGABRT:
			printf("\n***SIGABRT CAPTURED*** \n");
			if(logging_enabled)
				writeEventToLog("Received SIGABRT\n");
			break;
			
		default:
			printf("\nCAPTURED UNEXPECTED SIGNAL %d. Ignoring...\n", signo);
			return;
	}

	//Set output GPIOs to reset values and exit the program
	gpioWrite(RELAY1_PIN, 1);	
	gpioWrite(RLED_PIN, 0);	
	gpioWrite(GLED_PIN, 0);	

	gpioTerminate();
	exit(0);			
}

void termination_routine()
{
	printTimestamp(stdout);
	printf("Exiting...\n");
	
	if(logging_enabled)
		writeEventToLog("Program terminated.\n\n");
}

void powerCycle()
{
	gpioWrite(RELAY1_PIN, 0);	
	time_sleep(power_cycle_time);
	gpioWrite(RELAY1_PIN, 1);
}

int main(int argc, char *argv[])
{
	double start;
	int current_checking_period;
	int was_offline = 0;
	
	//Run the termination routine if the program exits at some point, due to initialization failures or captured signals
	atexit(termination_routine);
		
	//Parse the external config file if exists
	parseCfgFile();
	current_checking_period = net_check_period_std;
	
	printTimestamp(stdout);
	printf("Program Started!\n");

	if(logging_enabled)
		writeEventToLog("Program Started\n");
 	
	//Initialize the piggpio library
	if (gpioInitialise() < 0)
  	{
		fprintf(stderr, "pigpio initialisation failed\n");
		if(logging_enabled)
			writeEventToLog("\nProgram Started\n");
		
		return -1;
	}

	//Initialize GPIO Modes
	gpioSetMode(RELAY1_PIN, PI_OUTPUT);
	gpioSetMode(RLED_PIN, PI_OUTPUT);
	gpioSetMode(GLED_PIN, PI_OUTPUT);
	gpioSetMode(BTN_PIN, PI_INPUT);

	//Setup Pull-up/down for Input pins
	gpioSetPullUpDown(BTN_PIN, PI_PUD_UP);

	//Initial values for output pins
	gpioWrite(RELAY1_PIN, 1);	
	gpioWrite(RLED_PIN, 0);	
	gpioWrite(GLED_PIN, 0);	

	//Setup signal handling for graceful terminations
	gpioSetSignalFunc(SIGINT, sig_handler);
	gpioSetSignalFunc(SIGTERM, sig_handler);
	gpioSetSignalFunc(SIGABRT, sig_handler);
	
	//Start timer
	start = time_time();

	//Main control loop
	for(;;)
	{
		//Run the loop at a slower reasonable pace to avoid hogging the CPU
		time_sleep(MAIN_LOOP_DELAY);

		//Handles manual reset button press
		if(gpioRead(BTN_PIN) == 0)
		{
			printTimestamp(stdout);
			printf("Manual Reset! Power Cycling for %d seconds. Next check in %d seconds\n", power_cycle_time, net_check_period_alt);
			
			//Write event to log
			if(logging_enabled)
			{
				writeEventToLog("Manual Reset\n");
				was_offline = 1;
			}
		
			//Turn on the RED LED, and turn off the GREEN LED (if on)
			gpioWrite(RLED_PIN, 1);	
			gpioWrite(GLED_PIN, 0);	
			
			//Wait until the button is released.
			while(gpioRead(BTN_PIN) == 0)
				time_sleep(MAIN_LOOP_DELAY);
	
			//Power cycles the relay
			powerCycle();

			//Reset timer into alt mode
			start = time_time();
			current_checking_period = net_check_period_alt;
			continue;
		}
		
		//Skip network checking if the wait period's not up yet
		if((time_time() - start) < current_checking_period)
			continue;

		//Check if the network is ok by pinging google's DNS server
		if(system(PING_CMD) == 0)
		{
			printTimestamp(stdout);
			printf("Ping ok! Next check in %d seconds\n", net_check_period_std);
			
			//If network was previously down, write log that network is back online
			if(was_offline && logging_enabled)
			{
				writeEventToLog("Network restored\n");
				was_offline = 0;
			}

			//Turn off the RED LED (if on)
			gpioWrite(RLED_PIN, 0);	

			//Pulse the green LED low
			gpioWrite(GLED_PIN, 0);	
			time_sleep(LED_OK_PULSE);
			gpioWrite(GLED_PIN, 1);			
			
			//Reset the timer in normal mode
			start = time_time();
			current_checking_period = net_check_period_std;
		}

		//Ping failed indicating network failure
		else
		{
			printTimestamp(stdout);
			printf("Ping failed! Next check in %d seconds\n", net_check_period_alt);
			
			//Log the event
			if(!was_offline && logging_enabled)
			{
				writeEventToLog("Lost network connectivity\n");
				was_offline = 1;
			}

			//Turn on the RED LED, and turn off the GREEN LED (if on)
			gpioWrite(RLED_PIN, 1);	
			gpioWrite(GLED_PIN, 0);	
			
			//Power cycles the relay
			powerCycle();

			//Reset the timer into alt mode
			start = time_time();
			current_checking_period = net_check_period_alt;
		}

	}
}
