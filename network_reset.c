#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pigpio.h>

#include "network_reset.h"
#include "logging.h"
#include "extern_cfg.h"

//Default values for user definable variables loaded from external config file
int net_check_period_std = NET_CHECK_PERIOD_STD;
int net_check_period_alt = NET_CHECK_PERIOD_ALT;
int power_cycle_time = POWER_CYCLE_TIME;
int logging_enabled = LOGGING_ENABLED;

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
	
	printTimestamp(stdout);
	printf("Modem/Router Watchdog initialized!\n");
	if(logging_enabled)
		writeEventToLog("Program initialized\n");
}

int main(int argc, char *argv[])
{
	double start;
	int current_checking_period;
	int was_offline = 0;

	initialize();
	
	start = time_time();
	current_checking_period = net_check_period_std;

	//Main control loop
	for(;;)
	{
		//Run the loop at a slower reasonable pace to avoid hogging the CPU
		time_sleep(MAIN_LOOP_DELAY);

		//Handles manual reset button press
		if(gpioRead(BTN_PIN) == 0)
		{
			printTimestamp(stdout);
			printf("Manual Reset! Power cycling for %d seconds. Next check in %d seconds\n", power_cycle_time, net_check_period_alt);
			
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
