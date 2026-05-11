// ============================================================================
// config.c - (c) 2025 Alan Summerlin
// ============================================================================
//
// Configuration management for the EzPz Tracker.
//

#include <Adafruit_SPIFlash.h>		// SPI Flash (16 Mbit - 2 mbyte)
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include "cmd_proc.h"				// Command processor library (APS)
#include "config.h"
#include "RTClib.h"					// Real Time Clock support


extern const String settings_filename = "settings.txt";
extern FatVolume fatfs;
extern ProgramConfiguration_t m_configuration;
extern RTC_Millis rtc;
extern String m_states[];
extern String m_modes[];
extern CmdProcess cmd_proc;

// Tags found in "settings.txt"
String settings_tags[] = {
	"LOGGING_ENABLED",
	"LOGGING_MODE",
	"LOGGING_INTERVAL",
	"GMT_OFFSET",
	"MOVEMENT_TRIGGER_THRESHOLD",
	"LORA_ADDRESS",
	"LORA_NETWORKID",
	"LORA_PASSWORD",
	"LORA_POWER",
	"LORA_HOSTADDRESS",
	"LOG_DATE",
	"LOG_TIME",
	"LOG_POSITION",
	"LOG_ALTITUDE",
	"LOG_SPEED",
	"LOG_COURSE"
};

void config_load_configuration(void) {
	Serial.printf("Read configuration file into RAM data structure.\n\r");
	File32 configFile = fatfs.open(settings_filename.c_str(), FILE_READ);

	if (configFile) {

		char fname[20];		
		configFile.getName(fname, 20);
		Serial.printf("Loading configuration from file %s\n\r", fname);

		String line;
		do {			
			line = configFile.readStringUntil('\n');
			//Serial.println(line);
			int i = line.indexOf('=');
			if(i) {
				// Parse each key / value pair from the configuration file.
				//
				String key = line.substring(0, i);
				key.trim();				
				String value = line.substring(i + 1);
				value.trim();

				if(key == "LOGGING_ENABLED") {
					//
					// false (0) logging disabled, true (1) logging enabled
					//
					m_configuration.enabled = value.toInt();
					Serial.printf("m_configuration.enabled = %s\n\r", m_states[m_configuration.enabled].c_str());

				} else if(key == "LOGGING_MODE") {
					//
					// 0 = periodic logging, 1 = log when moving
					//
					m_configuration.mode = value.toInt();
					Serial.printf("m_configuration.mode = %s\n\r", m_modes[m_configuration.mode].c_str());

				} else if (key == "LOGGING_INTERVAL") {
					//
					// int 
					//
					m_configuration.logging_interval = value.toInt();
					Serial.printf("m_configuration.logging_interval = %d\n\r", m_configuration.logging_interval);

				} else if (key == "GMT_OFFSET") {
					m_configuration.gmt_offset = value.toInt();
					Serial.printf("m_configuration.gmt_offset = %d\n\r", m_configuration.gmt_offset);
					
				} else if (key == "LORA_ADDRESS") {
					m_configuration.lora_address = value.toInt();
					Serial.printf("m_configuration.lora_address = %d\n\r", m_configuration.lora_address);

				} else if (key == "LORA_NETWORKID") {
					m_configuration.lora_networkID = value.toInt();
					Serial.printf("m_configuration.lora_address = %d\n\r", m_configuration.lora_networkID);

				} else if (key == "LORA_PASSWORD") {
					//
					// TODO: Not sure how this is going to work.... test it!!!
					//
					value.toCharArray(m_configuration.lora_password, 32);
					Serial.printf("m_configuration.lora_password = %S\n\r", String(m_configuration.lora_password));

				} else if (key == "LORA_POWER") {
					m_configuration.lora_power = value.toInt();
					Serial.printf("m_configuration.lora_power = %d\n\r", m_configuration.lora_power);

				} else if (key == "LORA_HOSTADDRESS") {
					m_configuration.lora_hostAddress = value.toInt();
					Serial.printf("m_configuration.lora_hostAddress = %d\n\r", m_configuration.lora_hostAddress);

				} else if (key == "MOVEMENT_TRIGGER_THRESHOLD") {
					//
					//
					m_configuration.motion_trigger_thresold = value.toDouble();
					Serial.printf("m_configuration.motion_trigger_thresold = %f\n\r", m_configuration.motion_trigger_thresold);

				} else if (key == "LOG_POSITION") {
					//
					// Bool
					m_configuration.log_position = value.toInt();
					Serial.printf("m_configuration.log_position = %d\n\r", m_configuration.log_position);

				} else if (key == "LOG_ALTITUDE") {
					//
					// Bool
					m_configuration.log_altitude = value.toInt();
					Serial.printf("m_configuration.log_atitude = %d\n\r", m_configuration.log_altitude);

				} else if (key == "LOG_SPEED") {
					//
					// bool
					m_configuration.log_speed = value.toInt();
					Serial.printf("m_configuration.log_speed = %d\n\r", m_configuration.log_speed);

				} else if (key == "LOG_COURSE") {
					//
					// bool
					m_configuration.log_course = value.toInt();
					Serial.printf("m_configuration.log_course = %d\n\r", m_configuration.log_course);
				}
			}
		} while(line.length() > 0);
		configFile.close();

	} else {
		// Create a default file.
		config_build_config_file(0);
	}
}


void config_build_config_file(int mode) {
	//
	// Build a default configuration file and store in the root directory.
	//
	// mode = 0 - Build the default file (setup default values in m_configuration)
	// mode = 1 - Update the file from m_configuration
	//
	char buffer[80];

	//Serial.printf("Delete %s file.\n\r", settings_filename.c_str());
	Serial.printf("Delete %s file.\n\r", settings_filename.c_str());
	if (!fatfs.remove(settings_filename.c_str())) {
		Serial.println(F("Error, could not delete old configuration file."));
		// Overwrite mode to create a default file.
		mode = 0;
	}
	// https://forum.arduino.cc/t/sdfat-do-not-overwrite/373174/4
	//
	File32 configFile = fatfs.open(settings_filename.c_str(), (O_WRITE | O_CREAT | O_TRUNC));

	if(mode == 0) {
		Serial.println(F("Create a default configuration file."));
		
		char defaultpw[LORA_PW_SZ] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
			0, 1};

		m_configuration.enabled = 0;
		m_configuration.mode = LOG_MODE;
		m_configuration.logging_interval = 10;
		m_configuration.motion_trigger_thresold = 0.0005;
		m_configuration.gmt_offset = -4;

		m_configuration.lora_address = 100;
		m_configuration.lora_networkID = 1;
		memcpy(m_configuration.lora_password, defaultpw, LORA_PW_SZ);
		m_configuration.lora_power = 15;
		m_configuration.lora_hostAddress = 1998;

		m_configuration.log_date = LOG_DATE;						// Always log date and time for now.
		m_configuration.log_time = LOG_TIME;
		m_configuration.log_altitude = LOG_ALTITUDE;
		m_configuration.log_course = LOG_COURSE;
		m_configuration.log_date = LOG_DATE;
		m_configuration.log_position = LOG_POSITION;
		m_configuration.log_speed = LOG_SPEED;
		m_configuration.log_time = LOG_TIME;
	} else {
		Serial.println(F("Update configuration file."));
	}

	//
	//
	//
	if (configFile) {

		configFile.timestamp(
			T_WRITE, 
			rtc.now().year(), 
			rtc.now().month(), 
			rtc.now().day(), 
			rtc.now().hour(), 
			rtc.now().minute(), 
			rtc.now().second());

		Serial.printf("Year = %d\n\r", rtc.now().year()); // debug

		// Logging enabled: 0 = Disabled, 1 = enabled
		//
		sprintf(buffer, "LOGGING_ENABLED = %d", m_configuration.enabled);
		configFile.println(buffer);
		// 
		// Logging Mode: 0 = Manual (Default), 1 = Periodic, 2 = movement
		//
		sprintf(buffer, "LOGGING_MODE = %d", m_configuration.mode);
		configFile.println(buffer);
		//
		// Logging interval (in seconds - for periodic mode) - int 
		//
		sprintf(buffer, "LOGGING_INTERVAL = %ld", m_configuration.logging_interval);
		configFile.println(buffer);

		sprintf(buffer, "GMT_OFFSET = %d", m_configuration.gmt_offset);
		configFile.println(buffer);
		//
		// Movement in degrees (for movement, LOGGING_MODE = 2)
		//
		sprintf(buffer, "MOVEMENT_TRIGGER_THRESHOLD = %0.04f", m_configuration.motion_trigger_thresold);
		configFile.println(buffer);
		
		sprintf(buffer, "LORA_ADDRESS = %d", m_configuration.lora_address);
		configFile.println(buffer);

		sprintf(buffer, "LORA_NETWORKID = %d", m_configuration.lora_networkID);
		configFile.println(buffer);

		sprintf(buffer, "LORA_PASSWORD = %s", m_configuration.lora_password);
		configFile.println(buffer);

		sprintf(buffer, "LORA_POWER = %d", m_configuration.lora_power);
		configFile.println(buffer);

		sprintf(buffer, "LORA_HOSTADDRESS = %d", m_configuration.lora_hostAddress);
		configFile.println(buffer);

		sprintf(buffer, "LOG_DATE = %d", m_configuration.log_date);
		configFile.println(buffer);

		sprintf(buffer, "LOG_TIME = %d", m_configuration.log_time);
		configFile.println(buffer);

		//
		// Log GPS position (0 = no, 1 = yes)
		//
		sprintf(buffer, "LOG_POSITION = %d", m_configuration.log_position);
		configFile.println(buffer);
		//
		// Log GPS altitude (0 = no, 1 = yes)
		//
		sprintf(buffer, "LOG_ALTITUDE = %d", m_configuration.log_altitude);
		configFile.println(buffer);
		//
		// Log GPS speed (0 = no, 1 = yes)
		//
		sprintf(buffer, "LOG_SPEED = %d", m_configuration.log_speed);
		configFile.println(buffer);
		//
		// Log GPS course (0 = no, 1 = yes)
		//
		sprintf(buffer, "LOG_COURSE = %d", m_configuration.log_course);
		configFile.println(buffer);

		configFile.close();
	} else {
		Serial.println(F("Error, could not create the configuration file."));
	}
}

/** ---------------------------------------------------------------------------
 * @brief Print a text file to the Serial port [move to utils.cpp ???]
 * 
 * @param name The name of the file to print.
 * 
 * @return Nothing
 ** ------------------------------------------------------------------------ */
void print_text_file(const char *name) {
	File32 readFile = fatfs.open(name, FILE_READ);
	if (!readFile) {
		Serial.printf("Error, failed to open %s!", name);
		while (1) {
			yield();
		}
	}
	char fname[20];		
	readFile.getName(fname, 20);
	Serial.printf("\n\rReading file %s\n\r", fname);
	String line;
	do {			
		line = readFile.readStringUntil('\n');
		Serial.println(line);
	} while(line.length() > 0);
	Serial.printf("File size %ld\n\r", readFile.fileSize());
	readFile.close();
}

/** ---------------------------------------------------------------------------
 * @brief This is the SETTINGS Command handler function.
 * 
 * @return Nothing
 ** ------------------------------------------------------------------------ */
void settings(void) {
	if (cmd_proc.num_tokens == 1) {
		//
		// If only one token, show the current settings.
		//
		Serial.printf("Showing contents of % ssettings file:\n\r", settings_filename.c_str());
		print_text_file(settings_filename.c_str());
		Serial.println(F("To update a setting enter SETTINGS <SETTING_NAME> <value>"));
		Serial.println(F("To save changes enter SETTINGS update"));

	} else if (cmd_proc.num_tokens == 2) {
		//
		// If two tokens, then only the "update" option is valid.
		//
		if (cmd_proc.tokens[1].equals("update")) {
			// Update the settings file with new data.
			config_build_config_file(1);
		} else {
			Serial.println(F("Error, unknown command"));
		}

	} else if (cmd_proc.num_tokens == 3) {
		//
		// If three tokens, then a setting is being updated.
		// The settings_tags[] must contain a valid tag for each setting and the tags must align
		// with items (settings) included in the ProgramConfiguration_t data structure. 
		// This makes the index used here an easy way to know which item is being updated so that
		// tokens[2] can be converted to the proper type and stored in the correct structure member.
		//
		// TODO: some range checking here would be a good idea.
		//

		for(int i = 0; i < (int)settings_tags->length(); i++) {

			if (settings_tags[i].equals(cmd_proc.tokens[1])) {

				switch(i) {
					case 0:
						//bool newValue = cmd_proc.tokens[2].toLowerCase() 
						m_configuration.enabled = (bool) cmd_proc.tokens[2].toInt();
						break;

					case 1:
						m_configuration.mode = (uint8_t) cmd_proc.tokens[2].toInt();
						break;

					case 2:
						m_configuration.logging_interval = (uint32_t) cmd_proc.tokens[2].toInt();
						break;

					case 3:
						m_configuration.gmt_offset = (int8_t) cmd_proc.tokens[2].toInt();
						break;

					case 4:
						m_configuration.motion_trigger_thresold = (double) cmd_proc.tokens[2].toDouble();
						break;

					case 5:
						m_configuration.lora_address = (uint16_t) cmd_proc.tokens[2].toInt();
						break;

					case 6:
						m_configuration.lora_networkID = (uint8_t) cmd_proc.tokens[2].toInt();
						break;

					case 7:
						// TODO: Check length.
						strcpy(m_configuration.lora_password, cmd_proc.tokens[2].c_str());
						break;

					case 8:
						m_configuration.lora_power = (uint8_t) cmd_proc.tokens[2].toInt();
						break;
					
					case 9:
						m_configuration.lora_hostAddress = (uint16_t) cmd_proc.tokens[2].toInt();
						break;

					case 10:
						m_configuration.log_date = (bool) cmd_proc.tokens[2].toInt();
						break;

					case 11:
						m_configuration.log_time = (bool) cmd_proc.tokens[2].toInt();
						break;

					case 12:
						m_configuration.log_position = (bool) cmd_proc.tokens[2].toInt();
						break;

					case 13:
						m_configuration.log_altitude = (bool) cmd_proc.tokens[2].toInt();
						break;

					case 14:
						m_configuration.log_speed = (bool) cmd_proc.tokens[2].toInt();
						break;

					case 15:
						m_configuration.log_course = (bool) cmd_proc.tokens[2].toInt();
						break;
				}

				Serial.printf("Updating %s to %s\n]r", settings_tags[i].c_str(), cmd_proc.tokens[2].c_str());
				return;
			}		
		}
		Serial.println(F("Error, unknown setting"));

	} else {
		Serial.println(F("Error, too few arguments! See help."));
	}
}

/* --- End of File --- */