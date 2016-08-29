/*
	Compile:	gcc -o network_reset network_reset.c -lpigpio -lrt -lpthread
	Run:		sudo ./network_reset
*/

#include <stdio.h>
#include <pigpio.h>
#include <time.h>

//Pin declarations
#define RELAY1_PIN 2		//Relay is active LOW!
#define GLED_PIN 3		//LED is active HIGH!
#define RLED_PIN 4		//LED is active HIGH!
#define BTN_PIN 17		//Button is active LOW!

//Times declarations. All in seconds
#define NET_CHECK_PERIOD_STD 3		//How often to check network connectivitiy under normal circumstances
#define NET_CHECK_PERIOD_ALT 1		//When a network failure occurs, how often to check until network is back
#define RELAY_TOGGLE_TIME 0.5 		//Relay Cycle time
#define LED_OK_PULSE 0.25		//When ping suceeded, how long to pulse the green LED

//Other constant declarations
#define PING_CMD "ping -c 1 8.8.8.8 > /dev/null 2>&1"

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

	/*End of initialization*/

	start = time_time();

	//Main control loop
	for(;;)
	{
		//Handle button press. TODO: Use interrupts!
		if(gpioRead(BTN_PIN) == 0)
		{
			printTimestamp();
			printf("Manual Reset!\n");
		
			//If button is pressed, turn the LED ON
			gpioWrite(RLED_PIN, 1);	
			
			//Wait until the button is released
			while(gpioRead(BTN_PIN) == 0);
	
			//Power cycles the relay
			cycleRelay();
	
			//Turn LED off
			gpioWrite(RLED_PIN, 0);	

			continue;
		}
		
		//Skip network checking if the wait period's not up yet
		if((time_time() - start) < net_check_period)
			continue;

		//Check if the network is ok by pinging google's DNS server
		if(system(PING_CMD) == 0)
		{
			printTimestamp();
			printf("Ping ok!\n");

			//Turn off the RED LED (if on)
			gpioWrite(RLED_PIN, 0);	

			//Pulse the green LED
			gpioWrite(GLED_PIN, 0);	
			time_sleep(LED_OK_PULSE);
			gpioWrite(GLED_PIN, 1);			
			
			//Reset the timer
			start = time_time();
			net_check_period = NET_CHECK_PERIOD_STD;
		}

		//Ping failed indicating network failure
		else
		{
			printTimestamp();
			printf("Ping failed!\n");

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