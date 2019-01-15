#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/scb.h>

#include <string.h>

#include "led.h"
#include "strings.h"
#include "tick.h"
#include "msg.h"
#include "ring.h"
#include "crc_xmodem.h"
#include "tick.h"
#include "hdlc.h"
#include "bl.h"

/* set STM32 to clock by 48MHz from HSI oscillator */
static void clock_setup(void)
{
	rcc_clock_setup_in_hsi_out_48mhz();

	/* Enable clocks to the GPIO subsystems */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);

	rcc_periph_clock_enable(RCC_USART2);
}

static void usart_setup(int id, uint32_t speed)
{
	/* Setup USART parameters. */
	usart_set_baudrate(id, speed);
	usart_set_databits(id, 8);
	usart_set_parity(id, USART_PARITY_NONE);
	usart_set_stopbits(id, USART_CR2_STOP_1_0BIT);
	usart_set_mode(id, USART_MODE_TX_RX);
	usart_set_flow_control(id, USART_FLOWCONTROL_NONE);

	/* Enable USART Receive interrupt. */
	USART_CR1(id) |= USART_CR1_RXNEIE;

	/* Finally enable the USART. */
	usart_enable(id);
}

static void gpio_setup(void)
{
	led_setup();

	/* Setup GPIO pins for USART2 transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
	/* Setup USART2 TX pin as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF1, GPIO2);
	/* Setup GPIO pin GPIO_USART2_RX on GPIO port A for receive. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3);
	/* Setup USART2 RX pin as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF1, GPIO3);

	led_red_off();
	led_yellow_off();
	led_green_off();
}

typedef struct
{
	volatile uint16_t flag_tick;
	volatile uint16_t flag_250ms;
} tick_t;
volatile tick_t timer = { 0, 0 };

static void systick_setup(void)
{
	/* 48MHz / 8 => 6000000 counts per second */
	systick_set_clocksource(STK_CSR_CLKSOURCE_EXT);
	/* clear counter so it starts right away */
	STK_CVR = 0;

	systick_set_reload(6000000 / TICK_HZ);

	systick_interrupt_enable();

	/* Start counting. */
	systick_counter_enable();
}

static uint16_t div_250ms = 0;

void sys_tick_handler(void)
{
	timer.flag_tick = 1;

	if (++div_250ms >= MSEC_TO_TICK(250)) {

		div_250ms = 0;
		timer.flag_250ms = 1;
	}

	led_tick();
}

void usart_send_string(uint32_t id, const char * str, uint32_t size)
{
	uint32_t i;
	for (i = 0; i < size; i++)
		usart_send_blocking(id, str[i]);
}

#define BUFFER_SIZE 200

static struct ring tx_ring;
static uint8_t tx_ring_buffer[BUFFER_SIZE];
static struct ring rx_ring;
static uint8_t rx_ring_buffer[BUFFER_SIZE];

void usart2_isr(void)
{
	/* Check if we were called because of RXNE. */
	if (((USART_CR1(USART2) & USART_CR1_RXNEIE) != 0) &&
	    ((USART_ISR(USART2) & USART_ISR_RXNE) != 0)) {

		led_yellow_on();

		/* Retrieve the data from the peripheral. */
		ring_write_ch(&rx_ring, usart_recv(USART2));
	}

	/* Check if we were called because of TXE. */
	if (((USART_CR1(USART2) & USART_CR1_TXEIE) != 0) &&
	    ((USART_ISR(USART2) & USART_ISR_TXE) != 0)) {

		uint8_t ch;
		if (!ring_read_ch(&tx_ring, &ch)) {

			/* Disable the TXE interrupt, it's no longer needed. */
			USART_CR1(USART2) &= ~USART_CR1_TXEIE;
		} else {

			/* Put data into the transmit register. */
			usart_send(USART2, ch);
		}
	}
}

int usart2_write(const uint8_t * ptr, int len)
{
	int ret = ring_write(&tx_ring, (uint8_t *)ptr, len);

	USART_CR1(USART2) |= USART_CR1_TXEIE;

	return ret;
}

void usart_put_debug(const char * ptr)
{
	uint16_t len = strlen(ptr);
	uint8_t buf[len + 2];

	struct msg_t * msg = (msg_t *)buf;
	msg->len = sizeof(msg_t) + sizeof(msg_debug_t) + len;
	msg->type = e_type_debug;

	struct msg_debug_t * dbg = (struct msg_debug_t *)msg->data;
	dbg->len = len;
	for (uint8_t i = 0; i < len; i++) {
		dbg->data[i] = ptr[i];
	}

	hdlc_put_msg(msg);
}

#define FLASH_PAGE_SIZE 0x400

#define sei() __asm__ __volatile__ ("cpsie i")
#define cli() __asm__ __volatile__ ("cpsid i")

static uint8_t flash_read_byte(uint32_t address)
{
	return *(uint8_t *)(address);
}

static uint32_t flash_read_word(uint32_t address)
{
	return *(uint32_t *)(address);
}

static uint8_t check_app(void)
{
	uint16_t len = flash_read_word(ADDR_APP_LEN);
	uint16_t crc = flash_read_word(ADDR_APP_CRC);

	if ((len > (uint32_t)MAX_APP_LEN) || !len) {

		usart_put_debug("app len error");
		return 1;
	}

	uint16_t crc_u16 = 0;
	for (uint16_t i = 0; i < len; i++) {

		uint8_t byte_u8 = flash_read_byte(ADDR_APP + i);
		crc_u16 = crc_xmodem_update(crc_u16, byte_u8);
	}

	if (crc != crc_u16) {

		usart_put_debug("app crc error");

		return 1;
	}
	else {

		usart_put_debug("app crc ok");

		return 0;
	}

	return 0;
}

void start_app(void)
{
	/* Boot the application if it's valid. */
	if (!check_app()) {

		usart_put_debug("start app");

		//wait usart transfer
		while (USART_CR1(USART2) & USART_CR1_TXEIE)
			;

		cli();
		systick_interrupt_disable();
		nvic_disable_irq(NVIC_USART2_IRQ);

		/* Set vector table base address. */
		SCB_VTOR = ADDR_APP;
		/* Initialise master stack pointer. */
		asm volatile("msr msp, %0"::"g" (*(volatile uint32_t *)ADDR_APP));
		/* Jump to application. */
		(*(void (**)())(ADDR_APP + 4))();
	}
}

//10 secs
#define WAIT_UPTIME 40
uint32_t uptime = 0;

void usart_process(void)
{
	uint8_t ch;
	if (!ring_read_ch(&rx_ring, &ch))
		return;

	struct msg_t * msg = hdlc_get_msg(ch);
	if (!msg)
		return;


	led_green_on();

	uptime = WAIT_UPTIME + 1;

	if ((msg->type == e_type_ping) && (msg->len == 2)) {

		msg->type = e_type_pong;
		hdlc_put_msg(msg);
	}
	else if ((msg->type == e_type_erase) && (msg->len == 2)) {

		flash_unlock();
		uint32_t i;
		/* 28kb */
		for (i = 0; i < 28; i++)
			flash_erase_page(ADDR_APP + i * FLASH_PAGE_SIZE);
		flash_lock();

		hdlc_put_msg(msg);
	}
	else if ((msg->type == e_type_write) && (msg->len > 5)) {

		struct msg_flash_t * info = (struct msg_flash_t *)msg->data;

		flash_unlock();
		int i;
		for (i = 0; i < info->num; i++)
			flash_program_half_word(ADDR_APP + info->addr + 2 * i, info->data[i]);
		flash_lock();

		msg->len = 5;
		hdlc_put_msg(msg);
	}
	else if ((msg->type == e_type_read) && (msg->len == 5)) {

		struct msg_flash_t * info = (struct msg_flash_t *)msg->data;

		int i;
		for (i = 0; i < info->num; i++)
			info->data[i] = *((uint16_t *)(ADDR_APP + info->addr + 2 * i));

		msg->len = 5 + info->num * sizeof(uint16_t);
		hdlc_put_msg(msg);
	}
	else if ((msg->type == e_type_start) && (msg->len == 2)) {

		hdlc_put_msg(msg);

		start_app();
	}
	else {

		msg->type = e_type_unknown;
		msg->len = 2;
		hdlc_put_msg(msg);
	}
}

void timer_tick(void)
{
	if (timer.flag_tick) {

		timer.flag_tick = 0;

		if (timer.flag_250ms) {

			led_red_on();

			timer.flag_250ms = 0;
			uptime++;

			//send status
			uint8_t buf[sizeof(struct msg_t) + sizeof(struct msg_status_t)];
			struct msg_t * msg = (struct msg_t *)buf;
			msg->len = sizeof(buf);
			msg->type = e_type_status;
			msg_status_t * st = (msg_status_t *)msg->data;
			st->mode = e_mode_bl;
			st->version = 0x80 | 0x1;
			st->speed = 0;
			st->num_ids = 0;
			st->num_bytes = 0;
			hdlc_put_msg(msg);

			if (uptime == WAIT_UPTIME)
				start_app();
		}
	}
}

int main(void)
{
	cli();
	systick_interrupt_disable();
	nvic_disable_irq(NVIC_USART2_IRQ);

	clock_setup();
	gpio_setup();
	systick_setup();

	ring_init(&tx_ring, tx_ring_buffer, BUFFER_SIZE);
	ring_init(&rx_ring, rx_ring_buffer, BUFFER_SIZE);
	/* Enable the USART2 interrupt. */
	nvic_enable_irq(NVIC_USART2_IRQ);
	usart_setup(USART2, 38400);

	sei();

	usart_put_debug("bl");

	while (1) {

		usart_process();
		timer_tick();
	}

	return 0;
}

