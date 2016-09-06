#ifndef NETWORK_RESET_H
#define NETWORK_RESET_H

//GPIO Pin declarations
#define RELAY1_PIN					2		//Relay is active LOW!
#define GLED_PIN					3		//LED is active HIGH!
#define RLED_PIN					4		//LED is active HIGH!
#define BTN_PIN						17		//Button is active LOW!

//Default period declarations. All in seconds
#define NET_CHECK_PERIOD_STD		30		//How often to check network connectivitiy under normal circumstances
#define NET_CHECK_PERIOD_ALT		90		//When a network failure occurs, how often to check until network is back? This value must be greater than the time it takes for your router/modem to reboot.
#define POWER_CYCLE_TIME			10	 	//When power cycling, how long to toggle the relay for
#define LED_OK_PULSE				0.25	//When ping suceeded, how long to pulse the green LED
#define MAIN_LOOP_DELAY				0.1		//Delay added to each iteration of the main control loop (to save CPU)

//Misc default values
#define LOGGING_ENABLED				1		//Enable logging by default
#define MAX_PING_FAILS				3		//How many consecutive failed pings to count as network failures

//System command used to ping Google DNS to test internet connectivity
#define PING_CMD			"ping -c 1 8.8.8.8 > /dev/null 2>&1"

#endif
