/*
 * Asset Tracker 1 (AT-1)
 *
 * (c) 2026 Alan P. Summerlin
 *
 * Hardware Features:
 *   - Adafruit QT Py SAMD21 microcontroller board (with 2MB SPI-Flash installed)
 *   - GT-U7 GPS module for worldwide position detection
 *   - Reylax RYLR896 LoRa wireless Module
 *   - SSD1306 OLED Display
 *   - Li-Ion battery pack
 *
 * The device uses the GPS sensor to monitor its position, speed, etc.
 * This data can be logged locally and/or sent to a wireless receiver for direct, real-time
 * monitoring using the LoRa radio.
 *  
 * TODO:
 *    - Add serial port handler (similar to Robot1) so commands can entered using the USB 
 *      serial port (a.k.a. debug port). [Done]
 *    - Add file system  [Done]
 *    - Add command handler for debug port. 
 *    - Add commands for deleting, renaming, etc. files in the file system. [Done]
 *    - Add configuration file and default configuration for initial power up.
 *    - Allow LoRa channel, tx power, network id, address (and possibly other items) to be
 *       configured over the debug port. [Done]
 *    - Allow password to be configured to LoRa module.
 *    - Use xmodem to download files to a PC / etc. Add command to debug port for this. [Done - small files < 10K only]
 *    - A command to suspend debug output may be needed.
 *    - Create a "geo fence" object that defines a geographic area. And support an alarm that can
 *      be triggered when the fence is crossed.
 * 
*/

/** ---------------------------------------------------------------------------
 * @file      main.cpp
 * 
 * @details   Contains the main function and others.
 * 
 * @copyright (c)2026, Alan P. Summerlin
 * 
 * @author    Alan P. Summerlin
 * 
 * @brief     The main entry point for the application.
 * 
 ** ------------------------------------------------------------------------ */


#include <Adafruit_GFX.h>			// Graphics support for OLED display
#include <Adafruit_SSD1306.h>		// OLED Display driver
#include <Adafruit_SPIFlash.h>		// SPI Flash (16 Mbit - 2 mbyte)
#include <Arduino.h>
#include "cmd_proc.h"				// Command processor library (APS)
#include "commands.h"				//
#include "config.h"					// Program configuration structure, etc. 
#include "file_util.h"
#include "flash_config.h"			// SPI Flash configuration
#include "lora.h"					// Private LoRa protocol (APS)
#include "RTClib.h"					// Real Time Clock support
#include <SdFat.h>					// File system (in SPI Flash)
#include "sercom.h"					// SAMD21 SERCOM support
#include <SPI.h>					// SPI bus support
#include <string.h>					// String support
#include <TinyGPSPlus.h>			// GPS Module Support Library for GT-U7
#include <Wire.h>
#include "wiring_private.h"
#include "variant.h"
#include "XModem.h"					// From https://github.com/gilman88/xmodem-lib
#include "usbserial.h"				// USB serial port message handler 

#define APP_NAME			"AssetTracker v1.0"
#define OLED_RESET 			-1 		// Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 		128 	// OLED display width, in pixels
#define SCREEN_HEIGHT 		64 		// OLED display height, in pixels
#define	BUF_SiZE			256

const 	String 				log_filename = "data.csv";
const	String				settings_filename = "settings.txt";
int 	pinLoRaReset = 8;
char	buffer[256];
unsigned long lastmillis;
unsigned long millsnow;

bool	g_gps_data_available = false;

/* Function Prototypes */
void location(void);
void display_gps_data(void);
void gps_log(void);

// Real Time Clock support.
//
DateTime dateTime;
RTC_Millis rtc;
//
// Create the TinyGPSPlus object (for GT-U7 GPS module)
//
TinyGPSPlus gps;
//
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
//
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
//
// Asign SERCOM2 to be a UART (Serial2) to be used with the LoRa radio module.
//
Uart Serial2(&sercom2, 9, 10, SERCOM_RX_PAD_1, UART_TX_PAD_2);
//
// Setup the LoRa driver to control the radio module.
// Debugging is available over the USB Serial port.
//
LoRa lora((HardwareSerial*)&Serial, &Serial2, 8);
//
// Create the command processor for LoRa messages.
// Debugging is available over the USB Serial port.
//
CmdProcess cmd_proc((HardwareSerial*)&Serial);
//
// Create flash object to use with spi flash (for data logging)
//
Adafruit_SPIFlash flash(&flashTransport);
//
// file system objects from SdFat
//
FatVolume fatfs;
SdFat SD;
FsFile root;
//
// Create the debug port on Serial (USB serial interface)
//
USBSerialport usbserial((HardwareSerial*)&Serial);

XModem xmodem;

ProgramConfiguration_t m_configuration;

String m_states[] = {"disabled", "enabled"};
String m_modes[] = {"periodic", "motion triggered"};

 /**
  * Configure the IRQ Handler for UART Serial2. 
  */
void SERCOM2_Handler() {
    Serial2.IrqHandler();
}

/**
 * Call back for file timestamps.  Only called for file create and sync().
 */
void dateTimeCallback(uint16_t* date, uint16_t* time, uint8_t* ms10) {
  DateTime now = rtc.now();

  // Return date using FS_DATE macro to format fields.
  *date = FS_DATE(now.year(), now.month(), now.day());

  // Return time using FS_TIME macro to format fields.
  *time = FS_TIME(now.hour(), now.minute(), now.second());

  // Return low time bits in units of 10 ms, 0 <= ms10 <= 199.
  *ms10 = now.second() & 1 ? 100 : 0;
}

/**
 * This is the setup() function for the application.
 * It takes no parameters and returns nothing.
 */
void setup() {
	//
	// Setup code - runs once at startup
	//
  	// Configure serial ports:
	//  - Serial is the debug port (USB).
	//  - Serial1 is the GT-U7 GPS Module interface.
	//	- Serial2 is the LoRa radio Moiodule interface.
	//
	Serial.begin(115200);
	delay(2000);
	Serial1.begin(9600);
    pinPeripheral(9, PIO_SERCOM); 		// MISO -> TX
    pinPeripheral(10, PIO_SERCOM); 		// MOSI -> RX
	Serial2.begin(115200);
	//
	// Start real time clock.
	//
	rtc.begin(DateTime(F(__DATE__), F(__TIME__)));
	//
	// Configure OLED display module
	//
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
		// Address 0x3D for 128x64
		Serial.println("SSD1306 allocation failed");
		while(true);
	}
	// 
	// Configure the command processor by adding commands here:
	//
	cmd_proc.add("CLR_LOG",		"CLR_LOG - Clear the contents of the location log.",			clear_gps_log);
	cmd_proc.add("DEL", 		"DEL <filename> - Delete the specified file.",					delete_file);
	cmd_proc.add("DIR",		 	"DIR - Show files in the root directory.",						directory);
	cmd_proc.add("GPS_LOG",		"GPS_LOG - Log the current GPS data",							gps_log);
	cmd_proc.add("GPS_STAT",	"GPS_STAT - Show the current GPS status",						gps_status);
	cmd_proc.add("LOCATION",	"LOCATION - Get current location",								location);
	cmd_proc.add("TIME", 		"TIME <hh:mm:ss dd-mm-yyyy> - Set or get current RTC time.",	time);
	cmd_proc.add("PRINT",		"PRINT <filename> - Print text file.",							print_file);
	cmd_proc.add("RENAME",		"RENAME <old_file_name> <new_file_name> - Rename a file.",		rename_file);
	cmd_proc.add("XMODEM",		"XMODEM <file_name> - Send file to the host.",					send_small_file);
	cmd_proc.add("SETTINGS",	"SETTINGS <name value> - Review or update a setting.",			settings);


#if 0
	// Not supporting directories now
	cmd_proc.add((char *)"MKDIR",		(char *)"MKDIR <path> - Create a directory.",						make_directory);
	cmd_proc.add((char *)"RMDIR",		(char *)"RMDIR <path> - Delete a directory.",						delete_directory);
	cmd_proc.add((char *)"CD",			(char *)"CD <path> - Change directory.",							change_directory);
#endif	
	

	//
	// Display header text on OLED
	//
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0, 0);
	display.println(APP_NAME);
	display.display(); 

	//
	// Initialize the SPI flash, which is used for SdFat.
	//
	if (!flash.begin()) {
		Serial.println("Error, flash chip initialization failed");
		while(1);
	} 
	//
	// First call begin to mount the filesystem.  Check that it returns true
	// to make sure the filesystem was mounted.
	//
	if (!fatfs.begin(&flash)) {
		Serial.println("Error, failed to mount newly formatted filesystem!");		
	    while(1);
	}

	FsDateTime::setCallback(dateTimeCallback);

  	Serial.println("File system Mounted successfully!");

	// Load program configuration
	//
	config_load_configuration();
	//
	// Setup LoRa radio
	//
	pinMode(pinLoRaReset, OUTPUT);
	lora.hw_reset();
	if(!lora.connect(m_configuration.lora_address, m_configuration.lora_networkID)) {
		Serial.println("LoRa Radio setup failed.");
	}


	usbserial.prompt();
	millsnow = millis();
	millsnow = lastmillis;

}

 /**
  * The loop() function runs "forever"
  * - Poll Serial (the debug port) and process commands received.
  * - Poll Serial1 (the GPS port) - only receiving data from here.
  * - Poll the LoRa port and process any commands received.
  */
void loop() {

	millsnow = millis();

	while (usbserial.read()) {
		String message = usbserial.get_message().c_str();
	
		Serial.printf("rs232: %s\r\n", message.c_str());	// TODO: Need 'usbserial.print()' function
		cmd_proc.process(message);
		usbserial.prompt();
	}


	while (Serial1.available() > 0) {
		/* Read data from the GPM module, encode it, and update the display. */
		if(gps.encode(Serial1.read())) {
			display_gps_data();
		}
	}

	while (lora.read() == true) {
		LoRaMessage message = lora.parse_message();
		if(message.msg_size > 0) {
			//sprintf(buffer, "LoRa: %s\r\n", message.message.c_str());
			Serial.printf("LoRa: %s\r\n", message.message.c_str());
			cmd_proc.process(message.message);
		}
		lora.reset_buffer();
	}

	// Log if needed.
	//
	if (millsnow - lastmillis > (m_configuration.logging_interval * 1000) && 
		m_configuration.mode != 0 &&
		m_configuration.enabled) {
		
		if (m_configuration.mode == 2) {
			if (gps.speed.mph() > m_configuration.motion_trigger_thresold) {
				gps_log();	
			}
		} else {
			gps_log();
		}
		lastmillis = millsnow;
	}
}

/**
 * This function is uesd to log GPS data.
 * 
 */
void gps_log(void) {
	
	static bool dataAvailable = true;

	if (g_gps_data_available) {
		dataAvailable = true;
		File32 logFile = fatfs.open(log_filename, FILE_WRITE);

		if (logFile) {
			//
			// Display "L" on the top line to indicate a log entry is happening.
			// 
			display.setTextSize(1);
			display.setCursor(120, 0);
			display.print("L");
			display.display();
			

			// Format string for log entry capible of being uploading to https://www.gpsvisualizer.com/
			// TODO: On a new, file add the header
			//
			sprintf(
					buffer, 
					"T,%02d:%02d:%02d,%s,%s,%s,%s", 
					gps.time.hour() + m_configuration.gmt_offset, 
					gps.time.minute(), 
					gps.time.second(), 
					String(gps.location.lat(), 7).c_str(),
					String(gps.location.lng(), 7).c_str(),
					m_configuration.log_altitude ? String(gps.altitude.feet()).c_str() : "",
					m_configuration.log_speed ? String(gps.speed.mph(), 1).c_str() : "");

			logFile.println(buffer);
			logFile.close();

			// TODO: Make LoRa transmission optional.
			lora.write(buffer, m_configuration.lora_hostAddress);

			//
			// Clear the "L" from the display.
			//
			display.setCursor(120, 0);
			display.fillRect(120, 0, 8, 8, BLACK);
			display.display();

		} else {
			Serial.println("Error, could not open log file.");
		}
	} else {
		if (dataAvailable) {
			Serial.println("GPS data not available.");
			dataAvailable = false;
		}
	}
}


/**
 * TODO: Keep ot remove??
 */
void location(void) {
	sprintf(
		buffer, 
		"LOCATION: Lat %s, Long %s", 
		String(gps.location.lat(), 7).c_str(),
		String(gps.location.lng(), 7).c_str());

	lora.write(buffer, 1998);		// TODO: Make host address configurable in the settings.

	Serial.println(buffer);			// Debug print

}

/**
 * Display GPS data.
 */
void display_gps_data(void) {
	//static uint8_t last_second = 0;
	char buffer[50];

	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0, 0);
	display.println(APP_NAME);

	if (gps.location.isValid()) {
		//
		//
		display.fillRect(0, 16, display.width(), 8, BLACK);
		sprintf(buffer, "Lat  %04.7f", gps.location.lat());
		display.setCursor(0, 16);
		display.println(buffer);

		display.fillRect(0, 25, display.width(), 8, BLACK);		
		sprintf(buffer, "Long %04.7f", gps.location.lng());
		display.setCursor(0, 25);
		display.println(buffer);
		
		g_gps_data_available = true;
	}
	
	if (gps.date.isValid() && gps.time.isValid()) { //(gps.date.isUpdated() || gps.time.isUpdated()) && (gps.date.year() != 2000)) {
		//
		// Display GPS time
		//
		display.fillRect(0, 52, display.width(), 8, BLACK);
		display.setCursor(0,52);
		sprintf(buffer,
			"%02d:%02d:%02d %02d/%02d/%04d",
			gps.time.hour(),
			gps.time.minute(), 
			gps.time.second(),
			gps.date.month(),
			gps.date.day(),
			gps.date.year());
		display.println(buffer);

#if 0
		//
		// Sync the internal RTC to the GPS clock (so RTC is in GMT now).
		//

		// Note 4/16/26 - taking this out for now, not sure if its needed.
		//
		DateTime currentRTCDateTime = {
			.year = rtc.now().year(),  .month = rtc.now().month(), .day = rtc.now().day(),
			.hour = rtc.now().hour(), .min = rtc.now().minute(),  .sec = rtc.now().second()};
		DateTime gpsDateTime        = {
			.year = gps.date.year(),   .month = gps.date.month(),  .day = gps.date.day(),
			.hour = gps.time.hour(),  .min = gps.time.minute(),   .sec = gps.time.second()};

		if(currentRTCDateTime.secondstime() != gpsDateTime.secondstime()) {
			rtc.adjust(gpsDateTime);

			Serial.printf("Time adjustment (%ld seconds)\n\r", currentRTCDateTime.secondstime() - gpsDateTime.secondstime());
		}
#endif		
	}
	
	if (gps.speed.isValid()) {

#if 1
		display.fillRect(0, 43, display.width(), 8, BLACK);
		display.setCursor(0, 43);
		sprintf(buffer, "speed %s mph.", String(gps.speed.mph(), 1).c_str());
		display.println(buffer);
#endif		

	}
	if (gps.course.isValid()) {

	}
	
	if (gps.altitude.isValid()) {
		//
		//
		display.fillRect(0, 34, display.width(), 8, BLACK);
		display.setCursor(0, 34);
		sprintf(buffer, "Alt %f ft.", gps.altitude.feet());
		display.println(buffer);

	}
	
	if (gps.satellites.isValid()) {

	}
	
	if (gps.hdop.isValid()) {
		

	}

	display.display(); 	
}


/* ---- Fnd of File ---- */