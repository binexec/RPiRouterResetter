/*
	Compile:	gcc -o network_reset network_reset.c -lpigpio -lrt -lpthread
	Run:		sudo ./network_reset
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <pigpio.h>


//Pin declarations
#define RELAY1_PIN	2		//Relay is active LOW!
#define GLED_PIN	3		//LED is active HIGH!
#define RLED_PIN	4		//LED is active HIGH!
#define BTN_PIN		17		//Button is active LOW!

//Times declarations. All in seconds
#define NET_CHECK_PERIOD_STD	60	//How often to check network connectivitiy under normal circumstances
#define NET_CHECK_PERIOD_ALT	180	//When a network failure occurs, how often to check until network is back? This value must be greater than the time it takes for your router/modem to reboot.
#define RELAY_TOGGLE_TIME	10 	//Relay Cycle time
#define LED_OK_PULSE		0.25	//When ping suceeded, how long to pulse the green LED

//System command used to ping Google DNS to test internet connectivity
#define PING_CMD "ping -c 1 8.8.8.8 > /dev/null 2>&1"


void sig_handler(int signo)
{
	switch(signo)
	{
		case SIGINT:
			printf("SIGINT CAPTURED. Exiting...\n");
			break;
		case SIGTERM:
			printf("SIGINT CAPTURED. Exiting...\n");
			break;
		case SIGABRT:
			printf("SIGINT CAPTURED. Exiting...\n");
			break;
		default:
			printf("CAPTURED UNEXPECTED SIGNAL %d. Ignoring...\n", signo);
			return;
	}

	//Set output GPIOs to initial values and exit the program
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
	time_sleep(RELAY_TOGGLE_TIME);
	gpioWrite(RELAY1_PIN, 1);
}

int main(int argc, char *argv[])
{
	double start;
	int net_check_period = NET_CHECK_PERIOD_STD;

	printTimestamp();
	printf("Program Started!\n");
 	
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

	/*End of initialization*/

	//Main control loop
	for(;;)
	{
		//Handles manual reset button press
		if(gpioRead(BTN_PIN) == 0)
		{
			printTimestamp();
			printf("Manual Reset! Power Cycling for %d seconds. Next check in %d seconds\n", RELAY_TOGGLE_TIME, NET_CHECK_PERIOD_ALT);
		
			//Turn the RED LED ON
			gpioWrite(RLED_PIN, 1);	
			
			//Wait until the button is released
			while(gpioRead(BTN_PIN) == 0);
	
			//Power cycles the relay
			cycleRelay();
	
			//Turn off the RED LED
			gpioWrite(RLED_PIN, 0);	

			//Reset timer into alt mode
			start = time_time();
			net_check_period = NET_CHECK_PERIOD_ALT;
			continue;
		}
		
		//Skip network checking if the wait period's not up yet
		if((time_time() - start) < net_check_period)
			continue;

		//Check if the network is ok by pinging google's DNS server
		if(system(PING_CMD) == 0)
		{
			printTimestamp();
			printf("Ping ok! Next check in %d seconds\n", NET_CHECK_PERIOD_STD);

			//Turn off the RED LED (if on)
			gpioWrite(RLED_PIN, 0);	

			//Pulse the green LED
			gpioWrite(GLED_PIN, 0);	
			time_sleep(LED_OK_PULSE);
			gpioWrite(GLED_PIN, 1);			
			
			//Reset the timer in normal mode
			start = time_time();
			net_check_period = NET_CHECK_PERIOD_STD;
		}

		//Ping failed indicating network failure
		else
		{
			printTimestamp();
			printf("Ping failed! Next check in %d seconds\n", NET_CHECK_PERIOD_ALT);

			//Turn off the GREEN LED (if on)
			gpioWrite(GLED_PIN, 0);	
			
			//Turn on the red LED
			gpioWrite(RLED_PIN, 1);	
			
			//Power cycles the relay
			cycleRelay();

			//Reset the timer into alt mode
			start = time_time();
			net_check_period = NET_CHECK_PERIOD_ALT;
		}

	}
}