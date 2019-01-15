/** CRC-XMODEM calculation.
 *
 * Polynomial: x^16 + x^12 + x^5 + 1 (0x1021)
 * Initial value: 0x0
 * This is the CRC used by the Xmodem-CRC protocol.
 *
 */

#include "crc_xmodem.h"

uint16_t crc_xmodem_update (uint16_t crc, uint8_t _data)
{
	int i;
	crc = crc ^ ((uint16_t)_data << 8);
	for (i = 0; i < 8; i++) {

		if (crc & 0x8000)
			crc = (crc << 1) ^ 0x1021;
		else
			crc <<= 1;
	}

	return crc;
}

