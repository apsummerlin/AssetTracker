// ============================================================================
// config.h - (c) 2025 Alan Summerlin
// ============================================================================
//
// Configuration management for the EzPz Tracker.


//
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Constants
//
//#define CONFIG_FILE_NAME 	"settings.txt"
#define LOG_DATE			1
#define LOG_TIME			1
#define LOG_MODE			1		// 0 = manual, 1 = periodic, 2 = movement
#define	LOG_POSITION		1		// Log this position (long and lat)
#define LOG_SPEED			1		// Log speed
#define LOG_ALTITUDE		1		// Log altitude
#define	LOG_COURSE			0		// Log course
#define LOG_TIME			1
#define LOG_DATE			1
#define LORA_PW_SZ			32

// Program configuratoion data
//
typedef struct {
	bool		enabled;					// false - no logging, true - logging enabled.
	uint8_t		mode;						// 0 = manual, 1 = time interval, 2 = motion + time interval
	uint32_t	logging_interval;			// Logging interval in seconds
	int8_t		gmt_offset;					// +/- 12
	double		motion_trigger_thresold;	// For logging on motion - how many degrees in any direction to trigger the log entry

	// LoRa parameters
	uint16_t	lora_address;				// LoRa Address:	0 - 65535 (default 0) 
	uint8_t		lora_networkID;				// LoRa Network ID:	0 - 16 (default 0)
	char		lora_password[32];			// LoRa Password:	32 char  (00000000000000000000000000000001 to FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF)
	uint8_t		lora_power;					// LoRa RF power:	0 - 15 (0 = 0Bm ... 15 = 15Bm)
	uint16_t	lora_hostAddress;			// Host address: 	0 - 65535 (default 0) 

	// TODO: Make a bit field
	bool		log_date;					// 0 = no date logging, 1 = date logging enabled (default).
	bool		log_time;					// 0 = no time logging, 1 = time logging enabled (default).
	bool		log_position;				// 0 = no position logging, 1 = position logging enabled (default).
	bool		log_altitude;				// 0 = no altitude logging (default), 1 = altitude logging enabled.
	bool		log_speed;					// 0 = no speed logging (default), 1 = speed logging enabled.
	bool		log_course;	
} ProgramConfiguration_t;

// Function Prototypes
//
void config_load_configuration(void);
void config_build_config_file(int mode);
void print_text_file(const char *name);
void settings(void);

#endif
