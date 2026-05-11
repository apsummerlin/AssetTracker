//
// Copyright (c) 2024, Alan P. Summerlin, All rights reserved
//
// lora.h
//
// This class is responsible for LoRa wireless communications
// using the RYLR896 LoRa Module.
//

#ifndef LORA_H
#define LORA_H

#include <Arduino.h>
#include <string.h>

using namespace std;

#define BUF_SIZE 256

typedef unsigned int uint;

// LoRa connection states.
typedef enum {
	reset_,
	setupNode1,
	setupNode2,
	setupNode3,
	waitForMsg
} LoRaState_t;

struct LoRaMessage {
	uint		sender;		// Address of the sender
	uint    	msg_size;	// Payload field size
	String		message;	// Message (payload)
	int			RSSI;		// Signal strength
	int			SNR;		// Signal to noise ratio
	bool		error;
};

class LoRa
{
	private:
		char 			rx_buffer[BUF_SIZE];
		char 			tx_buffer[BUF_SIZE];
		uint			rx_buffer_idx;
		int				inByte;
		int				pinLoRaReset;
		bool			messageReceived;
		uint32_t		mills_start;
		uint32_t		mills_now;
		LoRaState_t		state;
		HardwareSerial *spDebug;
		HardwareSerial *spLoRa;

	public:
		LoRa(HardwareSerial *debugSer, HardwareSerial *loraSer, int _pin);
		bool 		connect(int _address, int _network_id);
		bool 		read(void);
		void		write(String message, uint address);
		void 		check_for_message_received(void);
		LoRaMessage parse_message(void);
		bool 		got_message(void);
		void 		reset_buffer(void);
		void 		hw_reset(void);
};

#endif
