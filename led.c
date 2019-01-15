#include <libopencm3/stm32/gpio.h>
#include "tick.h"

#define LED_RED_PORT GPIOB
#define LED_RED_PIN GPIO3
static uint16_t led_red_div = 0;
void led_red_on (void)
{
	gpio_set(LED_RED_PORT, LED_RED_PIN);
	led_red_div = 1;
}
void led_red_off (void)
{
	gpio_clear(LED_RED_PORT, LED_RED_PIN);
	led_red_div = 0;
}
uint8_t led_red_is_on (void)
{
	return led_red_div;
}

#define LED_YELLOW_PORT GPIOB
#define LED_YELLOW_PIN GPIO4
static uint16_t led_yellow_div = 0;
void led_yellow_on (void)
{
	gpio_set(LED_YELLOW_PORT, LED_YELLOW_PIN);
	led_yellow_div = 1;
}
void led_yellow_off (void)
{
	gpio_clear(LED_YELLOW_PORT, LED_YELLOW_PIN);
	led_yellow_div = 0;
}
uint8_t led_yellow_is_on (void)
{
	return led_yellow_div;
}

#define LED_GREEN_PORT GPIOB
#define LED_GREEN_PIN GPIO5
static uint16_t led_green_div = 0;
void led_green_on (void)
{
	gpio_set(LED_GREEN_PORT, LED_GREEN_PIN);
	led_green_div = 1;
}
void led_green_off (void)
{
	gpio_clear(LED_GREEN_PORT, LED_GREEN_PIN);
	led_green_div = 0;
}
uint8_t led_green_is_on (void)
{
	return led_green_div;
}

void led_setup(void)
{
	gpio_mode_setup(LED_RED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_RED_PIN);
	gpio_mode_setup(LED_YELLOW_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_YELLOW_PIN);
	gpio_mode_setup(LED_GREEN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GREEN_PIN);
}

void led_tick(void)
{
	if (led_red_is_on()) {

		if (led_red_div++ >= MSEC_TO_TICK(50)) {

			led_red_off();
		}
	}

	if (led_yellow_is_on()) {

		if (led_yellow_div++ >= MSEC_TO_TICK(50)) {

			led_yellow_off();
		}
	}

	if (led_green_is_on()) {

		if (led_green_div++ >= MSEC_TO_TICK(50)) {

			led_green_off();
		}
	}
}

