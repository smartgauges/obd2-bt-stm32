#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/stm32/iwdg.h>

#include <string.h>

#include "can.h"
#include "led.h"
#include "tick.h"
#include "strings.h"
#include "msg.h"
#include "ring.h"
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
	volatile uint16_t flag_40ms;
	volatile uint16_t flag_1000ms;
} tick_t;
volatile tick_t timer = { 0, 0, 0 };

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

static uint16_t div_40ms = 0;
static uint16_t div_1000ms = 0;

void sys_tick_handler(void)
{
	timer.flag_tick = 1;

	if (++div_40ms >= MSEC_TO_TICK(40)) {

		div_40ms = 0;
		timer.flag_40ms = 1;
	}

	if (++div_1000ms >= SEC_TO_TICK(1)) {

		div_1000ms = 0;
		timer.flag_1000ms = 1;
	}

	led_tick();
}

void usart_send_string(uint32_t id, const char * str, uint32_t size)
{
	uint32_t i;
	for (i = 0; i < size; i++)
		usart_send_blocking(id, str[i]);
}

struct ring tx_ring2;
uint8_t tx_ring_buffer2[2560];
struct ring rx_ring2;
uint8_t rx_ring_buffer2[256];

void usart2_isr(void)
{
	/* Check if we were called because of RXNE. */
	if (((USART_CR1(USART2) & USART_CR1_RXNEIE) != 0) &&
	    ((USART_ISR(USART2) & USART_ISR_RXNE) != 0)) {

		/* Retrieve the data from the peripheral. */
		ring_write_ch(&rx_ring2, usart_recv(USART2));
	}

	/* Check if we were called because of TXE. */
	if (((USART_CR1(USART2) & USART_CR1_TXEIE) != 0) &&
	    ((USART_ISR(USART2) & USART_ISR_TXE) != 0)) {

		uint8_t ch;
		if (!ring_read_ch(&tx_ring2, &ch)) {

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
	int ret = ring_write(&tx_ring2, (uint8_t *)ptr, len);

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

#define sei() __asm__ __volatile__ ("cpsie i")
#define cli() __asm__ __volatile__ ("cpsid i")

void start_boot(void)
{
	//wait usart transfer
	while (USART_CR1(USART2) & USART_CR1_TXEIE)
		;

	cli();
	systick_interrupt_disable();
	nvic_disable_irq(NVIC_USART2_IRQ);
	nvic_disable_irq(NVIC_CEC_CAN_IRQ);

	//hw reset
	iwdg_set_period_ms(100);
	iwdg_start();
	iwdg_reset();
	while(1)
		;
}

void usart_process(void)
{
	uint8_t ch;
	if (!ring_read_ch(&rx_ring2, &ch))
		return;

	struct msg_t * msg = hdlc_get_msg(ch);
	if (!msg)
		return;

	led_green_on();

	if ((msg->type == e_type_ping) && (msg->len == sizeof(struct msg_t))) {

		msg->type = e_type_pong;
		hdlc_put_msg(msg);
	}
	else if ((msg->type == e_type_can) && (msg->len == (sizeof(struct msg_t) + sizeof(struct msg_can_t)))) {

		struct msg_can_t * m = (struct msg_can_t *)msg->data;
		led_yellow_on();
		can_snd_msg(m);
		msg->len = sizeof(struct msg_t);
		hdlc_put_msg(msg);
	}
	else if ((msg->type == e_type_start) && (msg->len == sizeof(struct msg_t))) {

		hdlc_put_msg(msg);
		start_boot();
	}
	else {

		msg->type = e_type_unknown;
		msg->len = sizeof(struct msg_t);
		hdlc_put_msg(msg);
	}
}

e_speed_t speed = e_speed_125;
void can_autoselect_speed(void)
{
	if (can_get_msgs_num())
		return;

	if (++speed > e_speed_1000)
		speed = e_speed_125;

	can_set_speed(speed);
}

/*
 * 38400 kbit/sec = 3840 kbytes/sec
 */

static uint32_t can_cycle = 0;
void can_process(void)
{
	uint8_t msgs_num = can_get_msgs_num();

	if (!msgs_num)
		return;

	can_cycle++;
	uint8_t odd_mask = (can_cycle & 1) ? e_can_odd : 0;

	for (uint8_t i = 0; i < msgs_num; i++) {

		uint8_t buf[64];
		struct msg_can_t msg;

		if (can_get_msg(&msg, i)) {

			led_yellow_on();

			msg.type |= odd_mask;

			struct msg_t * m = (struct msg_t *)buf;
	
			uint8_t len = sizeof(struct msg_can_t);
			uint8_t * p = (uint8_t *)&msg;
			for (int j = 0; j < len; j++)
				m->data[j] = p[j];

			m->type = e_type_can;
			m->len = sizeof(struct msg_t) + len;
			hdlc_put_msg(m);
		}
	}
}

void snd_status(void)
{
	uint8_t buf[sizeof(struct msg_t) + sizeof(struct msg_status_t)];
	struct msg_t * msg = (struct msg_t *)buf;
	msg->len = sizeof(buf);
	msg->type = e_type_status;
	msg_status_t * st = (msg_status_t *)msg->data;
	st->mode = e_mode_sniffer;
	st->version = 3;
	st->speed = speed;
	st->num_ids = 0;
	st->num_bytes = can_cycle;
	hdlc_put_msg(msg);
}

int main(void)
{
	// Copy interrupt vector table to the RAM.
	volatile uint32_t *vtable = (volatile uint32_t *)0x20000000;
	uint32_t i;
	for (i = 0; i < 48; i++)
	{
		vtable[i] = *(volatile uint32_t *)(ADDR_APP + (i << 2));
	}
	SYSCFG_CFGR1 |= SYSCFG_CFGR1_MEM_MODE_SRAM;
	
	clock_setup();
	gpio_setup();
	systick_setup();

	can_setup();

	ring_init(&rx_ring2, rx_ring_buffer2, sizeof(rx_ring_buffer2));
	ring_init(&tx_ring2, tx_ring_buffer2, sizeof(tx_ring_buffer2));
	/* Enable the USART2 interrupt. */
	nvic_enable_irq(NVIC_USART2_IRQ);
	usart_setup(USART2, 38400);

	sei();

	while (1) {

		usart_process();

		if (timer.flag_tick) {

			timer.flag_tick = 0;

			if (timer.flag_40ms) {

				timer.flag_40ms = 0;
				can_autoselect_speed();
			}

			if (timer.flag_1000ms) {

				led_red_on();

				snd_status();

				can_process();
				timer.flag_1000ms = 0;
			}
		}
	}

	return 0;
}

