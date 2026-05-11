//
// Copyright (c) 2024, Alan P. Summerlin, All rights reserved
//
// cmd_proc.h - command processor class
//
#ifndef cmd_proc_h
#define cmd_proc_h

#include "lora.h"
#include <string.h>

using namespace std;

#define MAX_COMMANDS 15			// Maximum number of commands
#define MAX_TOKENS   10			// Maximum number of tokens per command.

struct Command_t {
	const char * name;
	const char * help;
	int    id;
	void (*FuncPtr)(void);		// Pointer to function for menu item
	//
	// see: https://stackoverflow.com/questions/36969471/c-how-to-make-function-pointer-to-class-method
	//void (this::*FuncPtr)();		// Pointer to function for menu item
};

class CmdProcess
{
	private:
		int					iCommands;				// Number of commands stored
		int 				iCommandIndex;         	// Command index (point to command)
		Command_t 			cmds[MAX_COMMANDS];		// Command array
		HardwareSerial 		*spDebug;				// Debug serial port

	public:
		CmdProcess(HardwareSerial *debugSer);
		//void add(char * name_, char * help_, void (*FuncPtr_)(String** tokens)); ---- tokens don't need to be passed
		void add(const char * name_, const char * help_, void (*FuncPtr_)(void));
		void add(Command_t cmd);
		void process(String command);
		void process(LoRaMessage command);
		int  num_commands();
		int  num_tokens;
		String tokens[MAX_TOKENS];
};

#endif