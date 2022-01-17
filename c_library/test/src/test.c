#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#include <bmstools/serial.h>

uint16_t calc_cksum(const uint8_t *data, size_t len) {
	uint16_t cksum = 0;
	while(len--){
		cksum -= *data++;
	}
	return cksum;
}

typedef long long milliseconds_t;
milliseconds_t millis() {
    long            ms; // Milliseconds
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return (milliseconds_t)spec.tv_sec * 1000 + spec.tv_nsec / 1.0e6; // Convert nanoseconds to milliseconds
}

size_t make_cmd(int is_read, uint8_t reg, const uint8_t *data, size_t data_len, uint8_t *dest, size_t dest_len) {
	// start [1] 0xdd
	// cmd [1]
	// reg [1]
	// len [1]
	// data [len]
	// chksum [2]
	// end [1] 0x77

	if (data_len + 7 > dest_len) return 0;

	uint8_t *cur = dest;
	*cur++ = 0xdd;
	*cur++ = is_read ? 0xA5 : 0x5A;
	*cur++ = reg;
	*cur++ = data_len;
	if (data != NULL) memcpy(cur, data, data_len);
	cur += data_len;
	*((uint16_t*)cur) = htons(calc_cksum(dest + 2, data_len + 2));
	cur += sizeof(uint16_t);
	*cur++ = 0x77;
	return cur - dest;
}

size_t make_read_cmd(uint8_t reg, const uint8_t *payload, size_t payload_len, uint8_t *dest, size_t dest_len) {
	return make_cmd(1, reg, payload, payload_len, dest, dest_len);
}
size_t make_write_cmd(uint8_t reg, const uint8_t *payload, size_t payload_len, uint8_t *dest, size_t dest_len) {
	return make_cmd(0, reg, payload, payload_len, dest, dest_len);
}

void hexdump(const uint8_t *buf, size_t len) {
	while(len--) {
		printf("%02X ", *buf++);
	}
	printf("\n");
}

int main (int argc, char **argv) {
	uint8_t buf[64];
	int s;
	size_t len;
	bmstools_port_t port;
	s = bmstools_port_init("/dev/ttyUSB0", &port);
	printf("init: %d\n", s);
	s = bmstools_port_open(&port);
	printf("open: %d\n", s);
	len = make_read_cmd(0x04, NULL, 0, buf, sizeof(buf));
	printf("packet is %ld bytes\n", len);
	hexdump(buf, len);

	for (int i = 0; i < 2; i++) {
		printf("---\n");
		bmstools_port_write(&port, buf, len);

#if 0
printf("now: %lld\n", millis());
sleep(1);
printf("now: %lld\n", millis());
#endif

		milliseconds_t then = millis();
		while ((millis() - then) < 1000) {
			int c = bmstools_port_getchar(&port);
			if (c >= 0) {
				printf("%02X ", c);
			}
		}
		printf("\n");
	}
}