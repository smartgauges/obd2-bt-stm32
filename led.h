#ifndef LED_H
#define LED_H

void led_setup(void);
void led_tick(void);

void led_red_on (void);
void led_red_off (void);
uint8_t led_red_is_on (void);

void led_yellow_on (void);
void led_yellow_off (void);
uint8_t led_yellow_is_on (void);

void led_green_on (void);
void led_green_off (void);
uint8_t led_green_is_on (void);

#endif

