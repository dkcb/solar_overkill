
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>

#include <bmstools/serial.h>


int bmstools_port_init(const char *portname, bmstools_port_t*port) {
    int s;
    if (port == NULL) return BMSTOOLS_PORT_BAD;

    port->fd = -1;
    port->portname = strdup(portname);
    if (port->portname == NULL) {
        return errno;
    }
    return 0;
}

int bmstools_port_open(bmstools_port_t *port) {
    int s;
    struct termios tty;

    if (port == NULL) return BMSTOOLS_PORT_BAD;

    s = open(port->portname, O_RDWR);
    if (s < 0) goto error;

    port->fd = s;

    s = tcgetattr(port->fd, & tty);
    if(s) goto error;

    tty.c_cflag &= ~PARENB; 	// no parity
    tty.c_cflag &= ~CSTOPB; 	// one stop bit
    tty.c_cflag &= ~CRTSCTS; 	// Disable RTS/CTS

    // 8 bits
    tty.c_cflag &= ~CSIZE; 
    tty.c_cflag |= CS8;

    tty.c_cflag |= CLOCAL | CREAD; // enable read, and no modem controls

    tty.c_lflag &= ~ICANON; // clear canonical mode

    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo

    tty.c_lflag &= ~ISIG;  // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);  // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
                     ICRNL);  // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST;  // Prevent special interpretation of output bytes
                            // (e.g. newline chars)
    tty.c_oflag &= ~ONLCR;  // Prevent conversion of newline to carriage return/line feed

    tty.c_cc[VTIME] = 10;
    tty.c_cc[VMIN] = 0;

    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    s = tcsetattr(port->fd, TCSANOW, &tty);
    if(s) goto error;
    
    return 0;

error:
    close(port->fd);
    port->fd = -1;
    return s;
}

int bmstools_port_close(bmstools_port_t *port) {
    if (port == NULL) return BMSTOOLS_PORT_BAD;
    if (close(port->fd)) return errno;
    return 0;
}

int bmstools_port_getchar(const bmstools_port_t *port) {
    uint8_t c;
    ssize_t bcount;
    if (port == NULL) return BMSTOOLS_PORT_BAD;

    bcount = read(port->fd, &c, sizeof(c));
    //printf("b: %d %02X\n", bcount, c);
    return bcount > 0 ? c : -1;
}

int bmstools_port_write(const bmstools_port_t *port, const uint8_t *buf, size_t len) {
    if (port == NULL) return BMSTOOLS_PORT_BAD;
    return write(port->fd, buf, len);
}