
#include "usbserial.h"
#include <string>

USBSerialport::USBSerialport(HardwareSerial *serial_port)
{
	sp = serial_port;
	rxIdx = 0;
	processMessage = false;
	history_buffer_idx = 0;
}

// Read commands from the debug port.
//
int USBSerialport::read()
{
	while(sp->available())
	{
		char charIn = (char)(sp->read() & 0x00FFU);
		
		if(charIn == ESC)
		{
			// Handle arrow keys here:
			//
		//	lcd.setCursor(6, 3);
		//	lcd.print("ESC");

			int count = 0;
			int special = 0;
			do {
				special = sp->read();
				if (special != -1) count++;
			} while (count < 2);

			//lcd.setCursor(10, 3);
			if (special == 'A')			// Up arrow pressed
			{
				// TODO: Scroll up through the command buffer.
				//lcd.print("UP");
				while(rxIdx > 0)
				{
					rxBuffer[--rxIdx] = 0;
					sp->print("\b \b");
				}

				if (history_buffer.size() > 0) 
				{
					std::copy(history_buffer.at(history_buffer_idx).begin(), history_buffer.at(history_buffer_idx).end(), rxBuffer);
					rxIdx = strlen(rxBuffer);
					sp->print(rxBuffer);

					if(++history_buffer_idx >= (int)history_buffer.size()) history_buffer_idx = 0;
				}
			}
			else if (special == 'B')	// Down arrow pressed
			{
				// TODO: Scroll down through the command buffer.
				//lcd.print("DN");

				while(rxIdx > 0)
				{
					rxBuffer[--rxIdx] = 0;
					sp->print("\b \b");
				}

				if (history_buffer.size() > 0) 
				{
					std::copy(history_buffer.at(history_buffer_idx).begin(), history_buffer.at(history_buffer_idx).end(), rxBuffer);
					rxIdx = strlen(rxBuffer);
					sp->print(rxBuffer);

					if(--history_buffer_idx < 0) history_buffer_idx = history_buffer.size()-1;
				}
			}
			else if (special == 'C')	// Right arrow pressed
			{
				// TODO: ??
				//lcd.print("> ");
			}
			else if (special == 'D')	// Left arrow pressed
			{
				// TODO: ??
				//lcd.print("< ");
			}
			
		}

		else if(isPrintable(charIn))
		{
			//lcd.setCursor(6, 3);			// For debug
			//lcd.print("OVERFLOW      ");	// 

			if(RX_BUFFER_SZ == rxIdx)
			{
				rxIdx = 0;
				processMessage = true;
			}
			else
			{
				//lcd.print("              ");	// For debug
				sp->print(charIn);
				rxBuffer[rxIdx++] = charIn;
				rxBuffer[rxIdx] = 0;
			}
		}
		else if (charIn == CR)
		{
			//lcd.setCursor(6, 3);			// For debug
			//lcd.print("              ");	// For debug

			// Handle Cariage Return
			//
			if(rxIdx > 0) 
			{				
				history_buffer.push_back(rxBuffer);
				processMessage = true;
				sp->print("\r\n");

				//message = rxBuffer;
			}
			else
			{
				sp->print("\r\n->");
			}
			break;
		}
		else if (charIn == BS)
		{
			//lcd.setCursor(6, 3);			// For debug
			//lcd.print("              ");	// For debug

			// Handle Backspace
			//
			if(rxIdx > 0)
			{	
				rxBuffer[--rxIdx] = 0;
				sp->print("\b \b");
			}
		}
	}

	return (processMessage == false) ? 0 : strlen(rxBuffer);
}

bool USBSerialport::got_message()
{
	return (processMessage == false) ? 0 : processMessage;
}

std::string USBSerialport::get_message()
{
	std::string s = rxBuffer;
	memset(rxBuffer, 0, sizeof(rxBuffer));
	rxIdx = 0;
	processMessage = false;
	return s;
}

void USBSerialport::prompt()
{
	sp->print("\r\n=>");
}