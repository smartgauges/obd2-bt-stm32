#ifndef BL_H
#define BL_H

#define ADDR_BL 0x08000000

#define ADDR_APP 0x08001000
#define ADDR_APP_END 0x08007fff

#define ADDR_APP_LEN (ADDR_APP_END - 3)
#define ADDR_APP_CRC (ADDR_APP_END - 1)
#define MAX_APP_LEN (ADDR_APP_LEN - ADDR_APP)

#endif

