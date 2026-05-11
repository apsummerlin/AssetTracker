//
// Copyright (c) 2024, Alan P. Summerlin, All rights reserved
//
// lora.c
//
// This class is responsible for LoRa wireless communications
// using the RYLR896 LoRa Module.
//
#include "lora.h"

using namespace std;

LoRa::LoRa(HardwareSerial *debugSer, HardwareSerial *loraSer, int _pin) {
	spDebug = debugSer;
	spLoRa = loraSer;
	pinLoRaReset = _pin;
	pinMode(pinLoRaReset, OUTPUT);
	state = (LoRaState_t)reset_;
	reset_buffer();
}

void LoRa::check_for_message_received() {
	// check for CR/LF and at least one byte
	//
	// TODO: Also check for buffer overfow
	//
	if ((rx_buffer_idx > 3) && (rx_buffer[rx_buffer_idx - 1] == 10) && (rx_buffer[rx_buffer_idx - 2] == 13)) {
		rx_buffer[rx_buffer_idx - 1] = 0;
		rx_buffer[rx_buffer_idx - 2] = 0;  // Trim off CR/LF
		rx_buffer_idx -= 2;
		messageReceived = true;
	}
}

bool LoRa::got_message(void) {
	return messageReceived;
}

LoRaMessage LoRa::parse_message(void) {
	LoRaMessage response = { 0 };

	int commas[] = { 0, 0, 0, 0 };
	int num_commas = 0;
	String s1 = String(rx_buffer);
	int p = 0;
	String s;
	
	if (s1.length() > 5) {
		//
		// Find the commas and store their
		// position in commas[]. There should
		// be 4 at most.
		//
		do {
			p = s1.indexOf(',', p + 1);
	  		if (p != -1) commas[num_commas++] = p;
		} while (p != -1);

		if (num_commas == 4) {
	  		response.sender = (uint)atoi(s1.substring(5, commas[0]).c_str());
	  		response.msg_size = (uint)atoi(s1.substring(commas[0] + 1, commas[1]).c_str());
			response.message = s1.substring(commas[1] + 1, commas[2]).c_str();
	  		response.RSSI = atoi(s1.substring(commas[2] + 1, commas[3]).c_str());
	  		response.SNR = atoi(s1.substring(commas[3] + 1).c_str());

			if(response.msg_size != response.message.length()) {
				response.error = true;
			}
		}
  	} else {
		if(s1.startsWith("+OK") || s1.startsWith("+ERR")) {
			response.message = s1.c_str();
		}
	}

  	return response;
}

//
// Connect to the LoRa module for wireless control.
// Times out after 1 second if not connected.
//
//		int _address
//		int _network_id
//
// Returns	1	Timed out
//			0	Success
//
bool LoRa::connect(int _address, int _network_id) {
	int status = -1;
	char temp[300];

	spDebug->println("Setup LoRa radio.");
	mills_start = millis();

	do {
		while (spLoRa->available()) {
	  		inByte = spLoRa->read();
	  		rx_buffer[rx_buffer_idx++] = (char)(inByte & 0x00FFU);
#if 0
			spDebug->print(char(inByte & 0x00FFU));
#endif
	  		check_for_message_received();
		}

		if (messageReceived == true) {
	#if 1
			spDebug->print("rx_buffer : ");
			spDebug->println((const char *)rx_buffer);
	#endif
			//
			// Process incoming module setup message responses, then wait for incoming messages.
			//
			if (/*(strncmp((const char *)rx_buffer, "+READY", 6) == 0) && */ (state == reset_)) {
				spDebug->print("Set LoRa Address ..: ");
				state = (LoRaState_t)setupNode1;
				sprintf(temp, "AT+ADDRESS=%d\r\n", _address);
				spLoRa->write(temp);
				spDebug->println(temp);
			} else if ((strncmp(rx_buffer, "+OK", 3) == 0) && (state == setupNode1)) {
				spDebug->print("Set Network ID ....: ");
				state = (LoRaState_t)setupNode2;
				sprintf(temp, "AT+NETWORKID=%d\r\n", _network_id);
				spLoRa->write(temp);
				spDebug->println(temp);
			} else if ((strncmp(rx_buffer, "+OK", 3) == 0) && (state == setupNode2)) {
				spDebug->print("SET BAND ..........: ");
				state = (LoRaState_t)setupNode3;
				sprintf(temp, "AT+BAND=915000000\r\n");
				spLoRa->write(temp);
				spDebug->println(temp);
			} else if ((strncmp(rx_buffer, "+OK", 3) == 0) && (state == setupNode3)) {
				spDebug->print("Set Parameters ....: ");
				state = (LoRaState_t)waitForMsg;
				sprintf(temp, "AT+PARAMETER=10,7,1,7\r\n");
				spLoRa->write(temp);
				spDebug->println(temp);
			} else if (state == waitForMsg) {
				spDebug->println("LoRa Connected");
				status = 0;
			}

			reset_buffer();
		}

		if ((millis() - mills_start) > 1000) {
			// time out
			spDebug->println("Error, LoRa timed out.");
			status = 1;
			reset_buffer();
		}
  	} while (status == -1);

  	return (status == 0);
}

bool LoRa::read(void) {

	while (spLoRa->available()) {
		inByte = spLoRa->read();
		rx_buffer[rx_buffer_idx++] = char(inByte);
#if 0		
		spDebug->print(char(inByte));
#endif
		check_for_message_received();
  	}

	return messageReceived;
}

void LoRa::write(String message, uint address) {
	sprintf(tx_buffer, "AT+SEND=%d,%d,%s\r\n", address,  message.length(), message.c_str());
	spLoRa->write(tx_buffer);
}

void LoRa::reset_buffer(void) {
	//
  	//
  	memset(rx_buffer, 0, sizeof(rx_buffer));
  	messageReceived = false;
  	rx_buffer_idx = 0;
}

void LoRa::hw_reset(void) {
	digitalWrite(pinLoRaReset, 0);
  	delay(110);
  	digitalWrite(pinLoRaReset, 1);
  	delay(10);

  	state = (LoRaState_t)reset_;
}