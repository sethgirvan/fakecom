/**
 * Allocates a pseudoterminal, prints the slave file name to STDERR, and writes
 * data read from STDIN to the master end and writes data read from the master
 * end to STDOUT.
 *
 * You can then pass the slave file name to a program that expects to
 * communicate with a serial port or terminal device in general (eg a program
 * to which you would normally be passing the path of something like a
 * USB-serial converter at ttyUSB0) and emulate input from that serial device by
 * writing to stdin of this program, and see the output from that program on
 * stdout of this program.
 *
 * Pass the -r flag to have this program set the pseudoterminal to raw mode.
 */
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 600

#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>


int set_term_raw(int fd, int baud, int parity)
{
	struct termios tty;
	if (tcgetattr(fd, &tty)) {
		// error from tcgetattr
		return -1;
	}

	cfsetospeed(&tty, baud);
	cfsetispeed(&tty, baud);

	tty.c_iflag &= ~(
			IGNBRK | // BREAK condition on input
			BRKINT | // SIGINT controlling process on BREAK condition
			PARMRK | // mark bytes with parity or framing errors
			ISTRIP | // strip off the eight bit
			INLCR | // translate NL to CR on input
			IGNCR | // ignore CR on input
			IXON ); // enable XON/XOFF flow control on output

	tty.c_lflag &= ~(
			ECHO | // echo input characters
			ECHONL | // echo newline character
			ICANON | // canonical mode
			ISIG | // generate signals for control character
			IEXTEN); // implementation-defined input processing

	tty.c_oflag &= ~OPOST; // implementation-defined output processing
	tty.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
	tty.c_cflag |= CS8 | parity; // 8 data bits

	tty.c_cc[VMIN] = 1; // read blocks until at least one char
	tty.c_cc[VTIME] = 0; // no read timeout

	if (tcsetattr(fd, TCSANOW, &tty)) {
		// error from tcsetattr
		return -1;
	}
	return 0;
}

void stdin_to_ptm(int fd)
{
	for (char c; fread(&c, 1, 1, stdin) == 1;) {
		if (write(fd, &c, 1) != 1) {
			return;
		}
	}
	fprintf(stderr, "Closing stdin_to_ptm.\n");
}

void *ptm_to_stdout(void *arg)
{
	fprintf(stderr, "Starting ptm_to_stdout\n");
	for (char c; read(*(int *)arg, &c, 1) == 1;) {
		fwrite(&c, 1, 1, stdout);
		fflush(stdout);
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	bool raw = false;

	for (int opt; (opt = getopt(argc, argv, "r")) != -1;) {
		switch (opt) {
			case 'r':
				raw = true;
				break;
			default:
				fprintf(stderr, "Unexpected option argument");
				return EXIT_FAILURE;
		}
	}

	int masterfd = posix_openpt(O_RDWR | O_NOCTTY);
	grantpt(masterfd);
	unlockpt(masterfd);

	if (raw) {
		if (set_term_raw(masterfd, B115200, 0))
			return -1;
	}

	fprintf(stderr, "%s\n", ptsname(masterfd));

	pthread_t thread_ptm_to_stdout;
	pthread_create(&thread_ptm_to_stdout, NULL, ptm_to_stdout, &masterfd);

	stdin_to_ptm(masterfd);

	int err = pthread_join(thread_ptm_to_stdout, NULL);
	if (err) {
		fprintf(stderr, "pthread_join(): %s", strerror(err));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
