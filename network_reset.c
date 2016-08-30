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


void createCfgFile()
{
	FILE *cfg = fopen(CFG_FILENAME, "w");

	fprintf(cfg, "%s\n\n", CFG_HEADER);
	fprintf(cfg, "NET_CHECK_PERIOD_STD %d\n", NET_CHECK_PERIOD_STD);
	fprintf(cfg, "NET_CHECK_PERIOD_ALT %d\n", NET_CHECK_PERIOD_ALT);
	fprintf(cfg, "POWER_CYCLE_TIME %d\n", POWER_CYCLE_TIME);

	fclose(cfg);
}

void parseCfgFile()
{
	char buf[CFG_MAX_LINE_CHAR];
	char *config_str;		//The string represent a variable to set from the line
	int config_val;			//The value associated with the variable 

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
		//Ignore empty lines
		if(buf[0] == '\n') 
			continue;

		//Extract the variable name and associated value from the line
		config_str = strtok(buf, " \n\t");		
		config_val = atoi(strtok(NULL, " \n\t"));
		//printf("PARSED: %s,%d\n", config_str, config_val);
		
		//Process the command
		if(strcmp(config_str, NET_CHECK_PERIOD_STD_STR) == 0)
			net_check_period_std = config_val;

		else if(strcmp(config_str, NET_CHECK_PERIOD_ALT_STR) == 0)
			net_check_period_alt = config_val;

		else if(strcmp(config_str, POWER_CYCLE_TIME_STR) == 0)
			power_cycle_time = config_val;

		else
			printf("Unrecognized command \"%s\"\n", config_str);
	}

	fclose(cfg);
}

void sig_handler(int signo)
{
	switch(signo)
	{
		case SIGINT:
			printf("\n***SIGINT CAPTURED*** \nExiting...\n");
			break;
		case SIGTERM:
			printf("\n***SIGTERM CAPTURED*** \nExiting...\n");
			break;
		case SIGABRT:
			printf("\n***SIGABRT CAPTURED*** \nExiting...\n");
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

void printTimestamp()
{
	time_t tm;
	struct tm* tm_info;
	char tm_str[20];

	time(&tm);
	tm_info = localtime(&tm);
	strftime(tm_str, 20, "%m-%d-%Y %H:%M:%S", tm_info);
	
	printf("%s ", tm_str);
}

void cycleRelay()
{
	gpioWrite(RELAY1_PIN, 0);	
	time_sleep(power_cycle_time);
	gpioWrite(RELAY1_PIN, 1);
}

int main(int argc, char *argv[])
{
	double start;
	int current_checking_period;

	printTimestamp();
	printf("Program Started!\n");
	
	//Parse the external config file if exists
	parseCfgFile();
	current_checking_period = net_check_period_std;
 	
	//Initialize the piggpio library
	if (gpioInitialise() < 0)
  	{
		fprintf(stderr, "pigpio initialisation failed\n");
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
			printTimestamp();
			printf("Manual Reset! Power Cycling for %d seconds. Next check in %d seconds\n", POWER_CYCLE_TIME, NET_CHECK_PERIOD_ALT);
		
			//Turn on the RED LED, and turn off the GREEN LED (if on)
			gpioWrite(RLED_PIN, 1);	
			gpioWrite(GLED_PIN, 0);	
			
			//Wait until the button is released
			while(gpioRead(BTN_PIN) == 0);
	
			//Power cycles the relay
			cycleRelay();

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
			printTimestamp();
			printf("Ping ok! Next check in %d seconds\n", net_check_period_std);

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
			printTimestamp();
			printf("Ping failed! Next check in %d seconds\n", net_check_period_alt);

			//Turn on the RED LED, and turn off the GREEN LED (if on)
			gpioWrite(RLED_PIN, 1);	
			gpioWrite(GLED_PIN, 0);	
			
			//Power cycles the relay
			cycleRelay();

			//Reset the timer into alt mode
			start = time_time();
			current_checking_period = net_check_period_alt;
		}

	}
}