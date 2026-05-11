//
// Copyright (c) 2025, Alan P. Summerlin, All rights reserved
//
// cmd_proc.cpp - command processor class functions.
//  
// Description: A general purpose command parser and processor. 
//

#include <Arduino.h>
#include "cmd_proc.h"
#include <string.h>

using namespace std;

CmdProcess::CmdProcess(HardwareSerial *debugSer)
{
	spDebug = debugSer;
	iCommands = 0;
	iCommandIndex = 0;
}

// Add a command
//
//void CmdProcess::add(char *name_, char *help_, void (*FuncPtr_)(String** tokens))
void CmdProcess::add(const char *name_, const char *help_, void (*FuncPtr_)(void))
{
	char buffer[100] = {0};

	if(iCommands < MAX_COMMANDS)
	{
		Command_t temp;
		temp.FuncPtr = FuncPtr_;
		temp.name = name_;
		temp.help = help_;
		temp.id = iCommands++;
		cmds[temp.id] = temp;	
	}
	else
	{
		sprintf(buffer, "Error: Command not added. Exceeded MAX_COMMANDS (defined as %d).", MAX_COMMANDS);
		spDebug->println(buffer);
	}
}

// Add a command using the Command_t structure.
//
void CmdProcess::add(Command_t cmd)
{
	char buffer[100] = {0};

	if(iCommands < MAX_COMMANDS)
	{
		cmd.id = iCommands++;
		cmds[cmd.id] = cmd;
	}
	else
	{
		sprintf(buffer, "Error: Command not added. Exceeded MAX_COMMANDS (defined as %d).", MAX_COMMANDS);
		spDebug->println(buffer);
	}
}

int  CmdProcess::num_commands()
{
	return iCommands;
}

//
// Process command - Start by parsing the command into tokens
// then pass the tokens to the command.
//
void CmdProcess::process(String command)
{	int tokIdx = 0;
	bool foundCommand = false;
	char buffer[100];
	//String tokens[100];				// Create an array to hold the tokens
	String temp_str;				// Holds a temporary string
	
	//
	// Parse command into tokens.	
	//
	int pos1 = 0;
	int pos2;
	bool done = false;
	while (done == false)
	{
		pos2 = command.indexOf(' ', pos1);

		if(pos2 == -1) 
		{
			pos2 = command.length();
			done = true;
		} 

		if((pos1 != pos2) && (tokIdx < MAX_TOKENS))
		{
			temp_str = command.substring(pos1, pos2);
			spDebug->println(temp_str.c_str());
			tokens[tokIdx++] = temp_str;
		}
		pos1 = ++pos2;
	}

	// Set the number of tokens found.
	num_tokens = tokIdx;

	// Look through the list of commands to see if any match.
	// Execute the function associated with the command.
	//
	for (auto & element : cmds) {
		if(strcmp(tokens[0].c_str(), "help") == 0) {
			// Intercept 'help' and process it now.
			//
			foundCommand = true;

			// Print all the command help strings.
			//
			for (auto & element : cmds) {
				spDebug->println(element.help);
			}
			break;
		} else if(strcmp(element.name, tokens[0].c_str()) == 0)	{
			element.FuncPtr();
			//element.FuncPtr();

			foundCommand = true;
			break;
		}
	}

	if(foundCommand == false) {
		sprintf(buffer, "Error: command %s is unknown.", tokens[0].c_str());
		spDebug->println(buffer);
	}
}
