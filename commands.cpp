
#include <Arduino.h>
#include "commands.h"
#include "cmd_proc.h"				// Command processor library (APS)
#include <SdFat.h>					// File system (in SPI Flash)
#include "XModem.h"					// From https://github.com/gilman88/xmodem-lib
#include "RTClib.h"					// Real Time Clock support
#include <TinyGPSPlus.h>			// GPS Module Support Library for GT-U7

extern CmdProcess cmd_proc; //((HardwareSerial*)&Serial);
extern FatVolume fatfs;
extern SdFat SD;
extern FsFile root;
extern XModem xmodem;
extern LoRa lora;
extern DateTime dateTime;
extern char	buffer[256];
extern RTC_Millis rtc;
extern TinyGPSPlus gps;

File32 fileToSend;


void send_file(void) {
	
	if (cmd_proc.num_tokens > 1) {
		Serial.printf("Sending file %s\n\r", cmd_proc.tokens[1].c_str());
		xmodem.begin((HardwareSerial &)Serial, XModem::ProtocolType::XMODEM);

		if (xmodem.send((char *)cmd_proc.tokens[1].c_str())) {
				Serial.println("Transfer Complete");
			} else {
				Serial.println("Error, transfer failed");
			}
	} else {
		Serial.println("Error, too few arguments.");
	}
	Serial.println("Done");
}

void send_small_file(void) {

	byte send_data[MAX_FILE_SIZE];
	
	if (cmd_proc.num_tokens > 1) {
		Serial.printf("Sending file %s\n\r", cmd_proc.tokens[1].c_str());

		fileToSend = fatfs.open(cmd_proc.tokens[1].c_str(), O_READ);

  		if (fileToSend) {
    		uint32_t fileSize = fileToSend.size(); // Gets file size

			if (fileSize > MAX_FILE_SIZE) {
				fileToSend.close();
				Serial.println(F("File too large to send."));
				return;
			}

			size_t bytesRead = fileToSend.read(send_data, fileSize);

			xmodem.begin((HardwareSerial &)Serial, XModem::ProtocolType::XMODEM);
			xmodem.send(send_data, bytesRead);

    		fileToSend.close();

		} else {
			Serial.println(F("Error, transfer failed"));
		}
	} else {
		Serial.println(F("Error, too few arguments."));
	}
	Serial.println(F("Done"));
}


void make_directory(void) {
	Serial.printf("Make directory %s: ", cmd_proc.tokens[1].c_str());
	if (cmd_proc.num_tokens > 1) {
		if (!fatfs.exists(cmd_proc.tokens[1].c_str())) {
			if (fatfs.mkdir(cmd_proc.tokens[1].c_str())) {
				Serial.println(F("Success!"));
			} else {
				Serial.println(F("Error, failed to create directory."));
			}
		} else {
			Serial.println(F("Error, directory already exists!"));
		}
	} else {
		Serial.println(F("Error, too few arguments."));
	}
}

void delete_directory(void) {
	File32 deleteDir = fatfs.open(cmd_proc.tokens[1].c_str());

	Serial.printf("Delete directory %s: ", cmd_proc.tokens[1].c_str());

	if (!deleteDir.rmRfStar()) {
		Serial.println(F("Error"));
	} else {
		Serial.println(F("Success"));
	}
}

// This does not seem to work
void change_directory(void) {
	//File32 changeDir = fatfs.open(cmd_proc.tokens[1].c_str());

	if (cmd_proc.num_tokens > 1) {
		if (SD.chdir(cmd_proc.tokens[1].c_str()) ) {
			Serial.println(F("Success!"));
		} else {
			Serial.println(F("Error, changing dir."));
		}
	} else {
		Serial.println(F("Error, too few arguments (path required)."));
	}
}


/** ---------------------------------------------------------------------------
 * @brief time()
 * 
 * 
 * 
 * @return nothing
 ** ------------------------------------------------------------------------ */
void time(void) {
	// Get or Set time for RTC
	//
	// Send time command in this format to set the time.
	// time <hh:mm:ss> <dd-MM-yyyy>

	if (cmd_proc.num_tokens == 1) {
		dateTime = rtc.now();

		sprintf(
			buffer, 
			"%02d:%02d:%02d %02d-%02d-%04d", 
			dateTime.hour(), 
			dateTime.minute(), 
			dateTime.second(),
			dateTime.day(),
			dateTime.month(),
			dateTime.year());

		lora.write(buffer, 1998);
		
		Serial.println(buffer);		// Debug print

	} else if(cmd_proc.num_tokens == 3) {
		
		Serial.println("Setting time now");

		int idx[2], h=0, m=0, s=0, y=0, M=0, d=0, num_colons = 0, num_dashes = 0, p = 0;

		do {
			p = cmd_proc.tokens[1].indexOf(':', p + 1);
	  		if (p != -1) idx[num_colons++] = p;
		} while (p != -1);

		if(num_colons == 2) {
			h = (int)atoi(cmd_proc.tokens[1].substring(0, idx[0]).c_str());				// Hour
			m = (int)atoi(cmd_proc.tokens[1].substring(idx[0] + 1, idx[1]).c_str());	// Minute
			s = (int)atoi(cmd_proc.tokens[1].substring(idx[1] + 1).c_str());			// Second

			//sprintf(buffer, "%d %d %d", h, m, s);
			//Serial.println(buffer);
		}

		do {
			p = cmd_proc.tokens[2].indexOf('-', p + 1);
	  		if (p != -1) idx[num_dashes++] = p;
		} while (p != -1);

		if (num_dashes == 2) {
			d = (int)atoi(cmd_proc.tokens[2].substring(0, idx[0]).c_str());				// Day
			M = (int)atoi(cmd_proc.tokens[2].substring(idx[0] + 1, idx[1]).c_str());	// Month
			y = (int)atoi(cmd_proc.tokens[2].substring(idx[1] + 1).c_str());			// Year

			//sprintf(buffer, "%d %d %d", d, M, y);
			//Serial.println(buffer);

		}

		if (num_colons== 2 && num_dashes == 2) {
			// Set time and date
			Serial.printf("Setting time to %02d:%02d:%02d %02d-%02d-%04d\n", h, m, s, M, d, y);
			rtc.adjust(DateTime(y, M, d, h, m, s));
		} else {
			sprintf(buffer, "Error %d %d", num_colons, num_dashes);
			Serial.println(buffer);
		}
	} else {
		Serial.println("Error, invalid arguments. See help.");
	}
}

void gps_status(void) {
	Serial.println("satalites:");
	Serial.printf("\tisValid .....: %d \n\r", gps.satellites.isValid());
	Serial.printf("\tisUpdated ...: %d \n\r", gps.satellites.isUpdated());
	Serial.printf("\tage .........: %d \n\r", gps.satellites.age());
	Serial.printf("\tvalue .......: %d \n\r", gps.satellites.value());
	
	Serial.println("time:");
	Serial.printf("\tisValid .....: %d \n\r", gps.time.isValid());
	Serial.printf("\tisUpdated ...: %d \n\r", gps.time.isUpdated());
	Serial.printf("\tage .........: %d \n\r", gps.time.age());
	Serial.printf("\thour ........: %d \n\r", gps.time.hour());
	Serial.printf("\tminute ......: %d \n\r", gps.time.minute());
	Serial.printf("\tsecond ......: %d \n\r", gps.time.second());
//	Serial.printf("\tvalue .......: %d \n\r", gps.time.value());
	
	Serial.println("date:");
	Serial.printf("\tisValid .....: %d \n\r", gps.date.isValid());
	Serial.printf("\tisUpdated ...: %d \n\r", gps.date.isUpdated());
	Serial.printf("\tage .........: %d \n\r", gps.date.age());
	Serial.printf("\tday .........: %d \n\r", gps.date.day());
	Serial.printf("\tmonth .......: %d \n\r", gps.date.month());
	Serial.printf("\tyear ........: %d \n\r", gps.date.year());
//	Serial.printf("\tvalue .......: %d \n\r", gps.date.value());
	
	Serial.println("location:");
	Serial.printf("\tisValid .....: %d \n\r", gps.location.isValid());
	Serial.printf("\tisUpdated ...: %d \n\r", gps.location.isUpdated());
	Serial.printf("\tage .........: %d \n\r", gps.location.age());
	Serial.printf("\tlat .........: %f \n\r", gps.location.lat());
	Serial.printf("\tlng .........: %f \n\r", gps.location.lng());
	
	Serial.println("altitude:");
	Serial.printf("\tisValid .....: %d \n\r", gps.altitude.isValid());
	Serial.printf("\tisUpdated ...: %d \n\r", gps.altitude.isUpdated());
	Serial.printf("\tage .........: %d \n\r", gps.altitude.age());
	Serial.printf("\tfeet ........: %f \n\r", gps.altitude.feet());
//	Serial.printf("\tvalue .......: %d \n\r", gps.altitude.value());
	
	Serial.println("speed:");
	Serial.printf("\tisValid .....: %d \n\r", gps.speed.isValid());
	Serial.printf("\tisUpdated ...: %d \n\r", gps.speed.isUpdated());
	Serial.printf("\tage .........: %d \n\r", gps.speed.age());
	Serial.printf("\tmph .........: %f \n\r", gps.speed.mph());
//	Serial.printf("\tvalue .......: %d \n\r", gps.speed.value());

	Serial.println("course:");
	Serial.printf("\tisValid .....: %d \n\r", gps.course.isValid());
	Serial.printf("\tisUpdated ...: %d \n\r", gps.course.isUpdated());
	Serial.printf("\tage .........: %d \n\r", gps.course.age());
	Serial.printf("\tdeg .........: %f \n\r", gps.course.deg());
//	Serial.printf("\tvalue .......: %d \n\r", gps.course.value());
}