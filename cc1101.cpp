#include "Arduino.h"
#include "SPI.h"

#include "cc1101.h"

//#include "CyberLib.h"

#include "debug.h"



#if defined CyberLib_H

#elif defined _SPI_H 

SPI spi;

#else

#endif




/**
* readReg
*
* Read CC1101 register via SPI
*
* 'regAddr'	Register address
* 'regType'	Type of register: CC1101_CONFIG_REGISTER or CC1101_STATUS_REGISTER
*
* Return:
* 	Data byte returned by the CC1101 IC
*/
byte CC1101_readReg(byte regAddr, byte regType)
{
	byte addr, val;
	SPI_SELECT();
	WAIT_SPI();

	addr = regAddr | regType;
	SPI_SEND(addr);                       // Send register address
	val = SPI_READ();                 // Read result

									  //wait_Miso();                          // Wait until MISO goes low

	SPI_DESELECT();

	return val;
}

/**
* CC1101_writeReg
*
* Write single register into the CC1101 IC via SPI
*
* 'regAddr'	Register address
* 'value'	Value to be writen
*/
void CC1101_writeReg(byte regAddr, byte value)
{
	SPI_SELECT();
	WAIT_SPI();

	SPI_SEND(regAddr);                    // Send register address
	SPI_SEND(value);                      // Send value

										  //wait_Miso();                          // Wait until MISO goes low
	SPI_DESELECT();
}
void CC1101_writeReg(byte regAddr, byte * values, byte count)
{
	SPI_SELECT();
	WAIT_SPI();

	SPI_SEND(regAddr);// Send register address
	for (byte i = 0; i < count; i++)
		SPI_SEND(values[i]);                      // Send value

	SPI_DESELECT();
}

/**
* cmdStrobe
*
* Send command strobe to the CC1101 IC via SPI
*
* 'cmd'	Command strobe
*/
byte CC1101_cmdStrobe(byte cmd)
{
	SPI_SELECT();
	WAIT_SPI();

	byte res = SPI_SEND(cmd);                        // Send strobe command

	SPI_DESELECT();
	return res;
}


void CC1101_init() {

	/* Sync word qualifier mode = 15/16 + carrier-sense above threshold */
	/* Device address = 0 */
	/* Channel spacing = 199.951172 */
	/* Modulation format = 2-FSK */
	/* Packet length mode = Fixed packet length mode. Length configured in PKTLEN register */
	/* Modulated = true */
	/* RX filter BW = 270.833333 */
	/* TX power = 12 */
	/* CRC autoflush = false */
	/* Preamble count = 4 */
	/* Data format = Normal mode */
	/* Carrier frequency = 868.299866 */
	/* Data rate = 32.7301 */
	/* PA ramping = false */
	/* Whitening = false */
	/* Channel number = 0 */
	/* Address config = No address check */
	/* Deviation = 47.607422 */
	/* Manchester enable = false */
	/* Base frequency = 868.299866 */
	/* Packet length = 255 */
	/* CRC enable = false */
	/***************************************************************
	*  SmartRF Studio(tm) Export
	*
	*  Radio register settings specifed with C-code
	*  compatible #define statements.
	*
	*  RF device: CC1101
	*
	***************************************************************/
	cmdStrobe(SRES); // Send reset command strobe	
					 //	delay(100);

					 //writeReg(0x0000, 0x03); //IOCFG2- GDO2 Output Pin Configuration
					 //writeReg(0x0001, 0x2E); //IOCFG1- GDO1 Output Pin Configuration
					 //writeReg(0x0002, 0x00); //IOCFG0- GDO0 Output Pin Configuration
					 //	writeReg(0x0003, 0x41); //FIFOTHR- RX FIFO and TX FIFO Thresholds
					 //	writeReg(0x0004, 0x76); //SYNC1- Sync Word, High Byte
					 //	writeReg(0x0005, 0x96); //SYNC0- Sync Word, Low Byte
					 //	writeReg(0x0006, 0x64); //PKTLEN- Packet Length
					 //	writeReg(0x0007, 0x00); //PKTCTRL1- Packet Automation Control
					 //	writeReg(0x0008, 0x00); //PKTCTRL0- Packet Automation Control
					 //	writeReg(0x0009, 0x00); //ADDR- Device Address
					 //	writeReg(0x000A, 0x00); //CHANNR- Channel Number
	writeReg(0x000B, 0x08); //FSCTRL1- Frequency Synthesizer Control
							//	writeReg(0x000C, 0x00); //FSCTRL0- Frequency Synthesizer Control
							//	writeReg(0x000D, 0x21); //FREQ2- Frequency Control Word, High Byte
							//	writeReg(0x000E, 0x65); //FREQ1- Frequency Control Word, Middle Byte
							//	writeReg(0x000F, 0x6A); //FREQ0- Frequency Control Word, Low Byte
							//	writeReg(0x0010, 0x6A); //MDMCFG4- Modem Configuration
							//	writeReg(0x0011, 0x4A); //MDMCFG3- Modem Configuration
							//	writeReg(0x0012, 0x05); //MDMCFG2- Modem Configuration //05 def
							//	writeReg(0x0013, 0x22); //MDMCFG1- Modem Configuration
							//	writeReg(0x0014, 0xF8); //MDMCFG0- Modem Configuration
							//	writeReg(0x0015, 0x47); //DEVIATN- Modem Deviation Setting
							//	writeReg(0x0016, 0x07); //MCSM2- Main Radio Control State Machine Configuration
	writeReg(0x0017, 0x3C); //Def:30 MCSM1- Main Radio Control State Machine Configuration
	writeReg(0x0018, 0x18); //MCSM0- Main Radio Control State Machine Configuration
	writeReg(0x0019, 0x2E); //FOCCFG- Frequency Offset Compensation Configuration
	writeReg(0x001A, 0x6D); //BSCFG- Bit Synchronization Configuration
	writeReg(0x001B, 0x04); //AGCCTRL2- AGC Control
	writeReg(0x001C, 0x09); //AGCCTRL1- AGC Control
	writeReg(0x001D, 0xB2); //AGCCTRL0- AGC Control
	writeReg(0x001E, 0x87); //WOREVT1- High Byte Event0 Timeout
	writeReg(0x001F, 0x6B); //WOREVT0- Low Byte Event0 Timeout
	writeReg(0x0020, 0xFB); //WORCTRL- Wake On Radio Control
	writeReg(0x0021, 0xB6); //FREND1- Front End RX Configuration
	writeReg(0x0022, 0x10); //FREND0- Front End TX Configuration

	writeReg(0x0023, 0xE9); //FSCAL3- Frequency Synthesizer Calibration
	writeReg(0x0024, 0x2A); //FSCAL2- Frequency Synthesizer Calibration
	writeReg(0x0025, 0x00); //FSCAL1- Frequency Synthesizer Calibration
	writeReg(0x0026, 0x1F); //FSCAL0- Frequency Synthesizer Calibration

	writeReg(0x0027, 0x41); //RCCTRL1- RC Oscillator Configuration
	writeReg(0x0028, 0x00); //RCCTRL0- RC Oscillator Configuration
	writeReg(0x0029, 0x59); //FSTEST- Frequency Synthesizer Calibration Control
	writeReg(0x002A, 0x7F); //PTEST- Production Test
	writeReg(0x002B, 0x3F); //AGCTEST- AGC Test
							//	writeReg(0x002C, 0x81); //TEST2- Various Test Settings
							//	writeReg(0x002D, 0x35); //TEST1- Various Test Settings
							//	writeReg(0x002E, 0x09); //TEST0- Various Test Settings


}
