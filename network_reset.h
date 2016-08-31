#ifndef NETWORK_RESET_H
#define NETWORK_RESET_H

//Pin declarations
#define RELAY1_PIN			2		//Relay is active LOW!
#define GLED_PIN			3		//LED is active HIGH!
#define RLED_PIN			4		//LED is active HIGH!
#define BTN_PIN				17		//Button is active LOW!


//Default period declarations. All in seconds
#define NET_CHECK_PERIOD_STD		30		//How often to check network connectivitiy under normal circumstances
#define NET_CHECK_PERIOD_ALT		90		//When a network failure occurs, how often to check until network is back? This value must be greater than the time it takes for your router/modem to reboot.
#define POWER_CYCLE_TIME			10	 	//When power cycling, how long to toggle the relay for
#define LED_OK_PULSE				0.25	//When ping suceeded, how long to pulse the green LED
#define MAIN_LOOP_DELAY			0.1		//Delay added to each iteration of the main control loop (to save CPU)


//System command used to ping Google DNS to test internet connectivity
#define PING_CMD			"ping -c 1 8.8.8.8 > /dev/null 2>&1"


//Logging related declarations
#define LOG_FILENAME 			"events.log"			//Filename of the log
#define LOGGING_ENABLED		1					//Disable logging by default
//#define LOG_MAX_EVENTS		1024				//Maximum number of lines of events in the log file. UNIMPLEMENTED.


//Configuration file  related declarations
#define CFG_FILENAME			"settings.cfg"			//Name of config file
#define CFG_HEADER				"[NETWORK_RESET_CFG]"		//First line of the config file
#define CFG_COMMENT_CHAR	'#'							//Lines that start with this character are treated as comments
#define CFG_MAX_LINE_CHAR		100							//Maximum number of characters to be read from each line


//String tokens in the CFG file that represents an override for one of the above values
#define NET_CHECK_PERIOD_STD_STR	"NET_CHECK_PERIOD_STD"
#define NET_CHECK_PERIOD_ALT_STR	"NET_CHECK_PERIOD_ALT"
#define POWER_CYCLE_TIME_STR		"POWER_CYCLE_TIME"
#define LOGGING_ENABLED_STR		"LOGGING_ENABLED"
//#define LOG_MAX_EVENTS_STR		"LOG_MAX_EVENTS"	//Unimplemented


#endif
