
//
// Class to handle serial port connection.
//
#ifndef usbserial_h
#define usbserial_h

#include <Arduino.h>
#include <vector>
#include <string>

#define RX_BUFFER_SZ	128U
#define FF				0x0CU				// FF (Form Feed)
#define CR				0x0DU				// CR (Carriage Return)
#define BS				0x08U				// BS (Back Space)
#define LF				0x0AU				// LF (Line Feed)
#define VT				0x0BU				// VT (Virtical Tab)
#define ESC				0x1BU				// ESC (Escape)

class USBSerialport
{
	private:
		char 	rxBuffer[RX_BUFFER_SZ];
		int		rxIdx;
		std::vector<std::string> history_buffer;
		int		history_buffer_idx;
		bool    processMessage;
		HardwareSerial *sp;

	public:
		USBSerialport(HardwareSerial *serial_port);
		bool got_message();
		std::string get_message();
		void prompt();
		int read();

};

#endif