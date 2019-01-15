#ifndef RING_H
#define RING_H

#include <inttypes.h>

typedef int32_t ring_size_t;

struct ring
{
	uint8_t *data;
	ring_size_t size;
	volatile uint32_t begin;
	volatile uint32_t end;
};

void ring_init(struct ring *ring, uint8_t *buf, ring_size_t size);
int32_t ring_write_ch(struct ring *ring, uint8_t ch);
int32_t ring_write(struct ring *ring, uint8_t *data, ring_size_t size);
uint8_t ring_read_ch(struct ring *ring, uint8_t *ch);

#endif

