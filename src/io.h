#pragma once

#include <stdint.h>

#include "os.h"
#include "os_io_seproxyhal.h"

#include "types.h"
#include "common/buffer.h"

#ifdef HAVE_BAGL
void io_seproxyhal_display(const bagl_element_t *element);
#endif

uint8_t io_event(uint8_t channel __attribute__((unused)));

uint16_t io_exchange_al(uint8_t channel, uint16_t tx_len);

void io_init(void);

int io_recv_command(void);

int io_send_response(const buffer_t *rdata, uint16_t sw);

int io_send_sw(uint16_t sw);
