#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <pigpio.h>

#include "logging.h"
#include "extern_cfg.h"
#include "network_reset.h"

//Global variable for this program only
static double timer;
static uint32_t current_checking_period;
static uint32_t failed_pings = 0;

//Handles OS signals that were captured to either ignore, or terminate the program gracefully
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
				writeEventToLog("Received SIGTERM\n");
			break;	
			
		case SIGABRT:
			printf("\n***SIGABRT CAPTURED*** \n");
			if(logging_enabled)
				writeEventToLog("Received SIGABRT\n");
			break;	
			
		case SIGTSTP:
			printf("\nIgnoring SIGTSTP... \n");
			return;	
			
		default:
			printf("\nCAPTURED UNEXPECTED SIGNAL %d. Ignoring...\n", signo);
			return;
	}
	exit(0);			
}

//Called whenever the program exits to reset the GPIO values. 
void termination_routine()
{
	//Set output GPIOs to reset values and exit the program
	gpioWrite(RELAY1_PIN, 1);	
	gpioWrite(RLED_PIN, 0);	
	gpioWrite(GLED_PIN, 0);	
	gpioTerminate();
	
	printTimestamp(stdout);
	printf("Exiting...\n");
	if(logging_enabled)
		writeEventToLog("Program terminated.\n\n");
}

//Initialize the program and GPIOs
void initialize()
{
	//Initialize the piggpio library and check if the program is running as root
	if (gpioInitialise() < 0)
  	{
		fprintf(stderr, "pigpio initialisation failed\n");
		exit(-1);
	}
	
	//Run the termination routine if the program exits at some point, due to initialization failures or captured signals
	atexit(termination_routine);
		
	//Parse the external config file if exists
	parseCfgFile();
	
	//Initialize GPIO input pins
	gpioSetMode(BTN_PIN, PI_INPUT);
	gpioSetPullUpDown(BTN_PIN, PI_PUD_UP);
	
	//Initialize GPIO output pins
	gpioSetMode(RELAY1_PIN, PI_OUTPUT);
	gpioSetMode(RLED_PIN, PI_OUTPUT);
	gpioSetMode(GLED_PIN, PI_OUTPUT);

	//Set initial values for output pins
	gpioWrite(RELAY1_PIN, 1);	
	gpioWrite(RLED_PIN, 0);	
	gpioWrite(GLED_PIN, 0);	

	//Setup signal handling for graceful terminations
	gpioSetSignalFunc(SIGINT, sig_handler);
	gpioSetSignalFunc(SIGTERM, sig_handler);
	gpioSetSignalFunc(SIGABRT, sig_handler);
	gpioSetSignalFunc(SIGTSTP, sig_handler);
	
	printTimestamp(stdout);
	printf("Modem/Router Watchdog initialized!\n");
	if(logging_enabled)
		writeEventToLog("Program initialized\n");
}

//Toggle the relay for a fixed amount of time to power cycle the router/modem
void powerCycle()
{
	gpioWrite(RELAY1_PIN, 0);	
	time_sleep(power_cycle_time);
	gpioWrite(RELAY1_PIN, 1);
}

//Handles when the manual reset button is pressed
int manual_reset_handler()
{
	printTimestamp(stdout);
	printf("Manual Reset! Power cycling for %d seconds. Next check in %d seconds\n", power_cycle_time, net_check_period_alt);
			
	//Write event to log 
	if(logging_enabled)
		writeEventToLog("Manual Reset\n");
	
	//mark network as down
	failed_pings = MAX_PING_FAILS;
		
	//Turn on the RED LED, and turn off the GREEN LED (if on)
	gpioWrite(RLED_PIN, 1);	
	gpioWrite(GLED_PIN, 0);	
		
	//Wait until the button is released.
	while(gpioRead(BTN_PIN) == 0)
		time_sleep(MAIN_LOOP_DELAY);
	
	//Power cycles the relay
	powerCycle();

	//Reset timer into alt mode
	timer = time_time();
	current_checking_period = net_check_period_alt;
}

//Handles when a ping has been sucessfully made
int ping_ok_handler()
{
	printTimestamp(stdout);
	printf("Ping ok! Next check in %d seconds\n", net_check_period_std);

	//If network was previously down, write log that network is back online
	if(failed_pings >= MAX_PING_FAILS && logging_enabled)
		writeEventToLog("Network restored\n");
	
	//Clear the failed pings counter
	failed_pings = 0;

	//Turn off the RED LED (if on)
	gpioWrite(RLED_PIN, 0);	

	//Pulse the green LED low
	gpioWrite(GLED_PIN, 0);	
	time_sleep(LED_OK_PULSE);
	gpioWrite(GLED_PIN, 1);			
			
	//Reset the timer in normal mode
	timer = time_time();
	current_checking_period = net_check_period_std;
}

//Handles when a ping has failed
int ping_failed_handler()
{
	char *logstr;
	
	//Don't reset the router/modem if the number of failed pings hasn't exceeded the threshold yet
	if(failed_pings < max_ping_fails - 1)
	{
		printTimestamp(stdout);
		printf("Ping failed (%d/%d). Next check in %d seconds\n", failed_pings + 1, max_ping_fails, net_check_period_alt);
		failed_pings++;
		
		//Reset the timer as normal mode and return
		timer = time_time();
		current_checking_period = net_check_period_std;
		return;
	}
			
	//Log the event when the network has been determined as failed
	if(failed_pings == max_ping_fails - 1 && logging_enabled)
	{
		printTimestamp(stdout);
		printf("Ping failed (%d/%d). \n", failed_pings + 1, max_ping_fails);
		
		//Prevent any subsequent failed pings from being logged
		failed_pings++;		
		
		//Write the event to log
		logstr = (char*) malloc(100*sizeof(char));
		sprintf(logstr, "Lost network connectivity after failing %d consecutive pings\n", failed_pings + 1);
		writeEventToLog(logstr);
		free(logstr);
	}
	
	printTimestamp(stdout);
	printf("Network is down! Next check in %d seconds\n", failed_pings + 1, max_ping_fails, net_check_period_alt);

	//Turn on the RED LED, and turn off the GREEN LED (if on)
	gpioWrite(RLED_PIN, 1);	
	gpioWrite(GLED_PIN, 0);	
	
	//Power cycles the relay
	powerCycle();

	//Reset the timer into alt mode
	timer = time_time();
	current_checking_period = net_check_period_alt;
}

int main(int argc, char *argv[])
{
	initialize();
	timer = time_time();
	current_checking_period = net_check_period_alt;

	//Main control loop
	for(;;)
	{
		//Run the loop at a slower reasonable pace to avoid hogging the CPU
		time_sleep(MAIN_LOOP_DELAY);

		//Check if the manual reset button has been pressed
		if(gpioRead(BTN_PIN) == 0)
		{
			manual_reset_handler();
			continue;
		}
		
		//Skip network checking if the wait period's not up yet
		if((time_time() - timer) < current_checking_period)
			continue;
		
		//Attempt to ping Google's DNS server to determine if our network is still up
		if(system(PING_CMD) == 0)
			ping_ok_handler();
		else
			ping_failed_handler();

	}
}
