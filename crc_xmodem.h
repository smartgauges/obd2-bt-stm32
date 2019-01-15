#ifndef CRC_XMODEM_H
#define CRC_XMODEM_H

#include <stdint.h>

/** CRC-XMODEM calculation.
 *
 * Polynomial: x^16 + x^12 + x^5 + 1 (0x1021)
 * Initial value: 0x0
 * This is the CRC used by the Xmodem-CRC protocol.
 *
 */
uint16_t crc_xmodem_update (uint16_t crc, uint8_t _data);

#endif

