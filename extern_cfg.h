#ifndef NETWORK_RESET_EXTERN_CFG_H
#define NETWORK_RESET_EXTERN_CFG_H

//Configuration file  related declarations
#define CFG_FILENAME				"settings.cfg"				//Name of config file
#define CFG_HEADER					"[NETWORK_RESET_CFG]"		//First line of the config file
#define CFG_COMMENT_CHAR			'#'							//Lines that start with this character are treated as comments
#define CFG_MAX_LINE_CHAR			100							//Maximum number of characters to be read from each line


//String tokens in the CFG file that represents a valid parameter
#define NET_CHECK_PERIOD_STD_STR	"NET_CHECK_PERIOD_STD"
#define NET_CHECK_PERIOD_ALT_STR	"NET_CHECK_PERIOD_ALT"
#define POWER_CYCLE_TIME_STR		"POWER_CYCLE_TIME"
#define MAX_PING_FAILS_STR			"MAX_PING_FAILS"
#define LOGGING_ENABLED_STR			"LOGGING_ENABLED"
//#define LOG_MAX_EVENTS_STR		"LOG_MAX_EVENTS"	//Unimplemented


//User definable variables to be loaded from external config file
uint32_t net_check_period_std;
uint32_t net_check_period_alt;
uint32_t power_cycle_time;
uint32_t logging_enabled;
uint32_t max_ping_fails;


//Function Prototypes
void createCfgFile();
void parseCfgFile();

#endif