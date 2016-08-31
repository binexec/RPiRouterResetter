#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "extern_cfg.h"
#include "network_reset.h"

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
