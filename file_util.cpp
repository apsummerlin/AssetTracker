/** ---------------------------------------------------------------------------
 * @file      file_util.cpp
 * 
 * @details   Contains file utility functions.
 * 
 * @copyright (c)2026, Alan P. Summerlin
 * 
 * @author    Alan P. Summerlin
 * 
 * @brief     Provides basic file management functions.
 * 
 ** ------------------------------------------------------------------------ */

#include "cmd_proc.h"
#include <SdFat.h>					// File system (in SPI Flash)
#include "config.h"					// Program configuration structure, etc. 
#include <Adafruit_SPIFlash.h>		// SPI Flash (16 Mbit - 2 mbyte)

extern CmdProcess cmd_proc;
extern FatVolume fatfs;
extern SdFat SD;
extern FsFile root;
extern Adafruit_SPIFlash flash;
extern const String log_filename;

/** 
 * Private Function Prototypes 
 */
void ShowFreeSpace(void);

/**
 * Show the contents of the root directory.
 */
void directory(void) {	
	//
	// Display a directory of files in root.
	//
	Serial.printf("\n\rDir of root:\n\r");
	fatfs.ls(LS_A | LS_DATE | LS_SIZE | LS_R);
	ShowFreeSpace();		
	Serial.println();
}

/* ----------------------------------------------------------------------------
 * Private Functions 
 * ------------------------------------------------------------------------- */

/**
 * Show the amount of free space in flash memory.
 */
void ShowFreeSpace(void) {
	//
	// Show total spi flash memory
	// Calculate and show free spi flash memory
	//
	uint32_t freeClusters = fatfs.freeClusterCount();
    uint32_t bytesPerCluster = fatfs.sectorsPerCluster() * 512;
    uint32_t freeBytes = freeClusters * bytesPerCluster;
    
    Serial.printf("Total flash size %d bytes.\n\rSpace available: %d bytes\n\r", flash.size(), freeBytes);
}

/* ----------------------------------------------------------------------------
 * Public Functions 
 * ------------------------------------------------------------------------- */

/**
 * Print the file specified by the command argument.
 */
void print_file(void) {

	if (cmd_proc.num_tokens > 1) {
		// Get file File name 
		print_text_file(cmd_proc.tokens[1].c_str());
	} else {
		Serial.println("Error, no file specified");
	}
}

/**
 * Delete the file specified by the command argument.
 */
void delete_file(void) {

	Serial.print("Delete file: ");

	if (cmd_proc.num_tokens > 1) {
		// Get file File name 
		//
		// TODO: Verify with the use the file to be deleted.
		//
		Serial.printf("%s ", cmd_proc.tokens[1].c_str());

		File32 deleteFile = fatfs.open(cmd_proc.tokens[1].c_str(), FILE_WRITE);
		deleteFile.close();

		if(fatfs.remove(cmd_proc.tokens[1].c_str())) {
			Serial.println("Success");
		} else {
			Serial.println("Error, unable to delete file");
		}
	} else {
		Serial.println("Error, no file specified");
	}
}

/**
 * Rename the file specified by the command argument.
 */
void rename_file(void) {
	if (cmd_proc.num_tokens > 2) {
		//
		// TODO: Verify with the user the file to be renamed.
		//
		Serial.printf("Renaming %s to %s\n\r", cmd_proc.tokens[1].c_str(), cmd_proc.tokens[1].c_str());

		if(fatfs.rename(cmd_proc.tokens[1].c_str(), cmd_proc.tokens[2].c_str())) {
			Serial.print("Success!\n");
		} else {
			Serial.print("Error, unable to rename file.\n");
		}
	} else {
		Serial.print("Error, too few arguments.\n");
	}
}

/**
 * Clear the log file data. Contains just the header.
 */
void clear_gps_log(void) {

	File32 logFile = fatfs.open(log_filename, O_RDWR | O_CREAT | O_TRUNC);

	if (logFile) {
		Serial.println("Clearing the log file.");
		logFile.println("type,time,latitude,longitude,alt,speed");
		logFile.close();
	} else {
		Serial.println("Error, could not clear the log file.");
	}
}

/* --- End of file ---*/