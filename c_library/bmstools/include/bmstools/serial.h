
#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>
#include <stddef.h>

typedef struct {
	char *portname;
	int fd;
} bmstools_port_t;

#define BMSTOOLS_PORT_BAD -1

int bmstools_port_init(const char *portname, bmstools_port_t *port);
int bmstools_port_deinit(bmstools_port_t *port);
int bmstools_port_open(bmstools_port_t *port);
int bmstools_port_close(bmstools_port_t *port);

int bmstools_port_write(const bmstools_port_t *port, const uint8_t *buf, size_t len);
int bmstools_port_getchar(const bmstools_port_t *port);

#endif
