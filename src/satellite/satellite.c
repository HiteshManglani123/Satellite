/*
    The client socket connects to the server socket.
    Client = Satellite
    Server = Ground Station (Earth)
*/

#include <sys/un.h>
#include <sys/socket.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "../tftp.h"

#define SATELLITE_SOCKET_PATH "temp/server-socket"

#define BUF_SIZE 100

#define BACKLOG 5

void exit_error(char *s) {
    perror(s);
    exit(1);
}

int main (int argc, char *argv[]) {
    int sfd;
	struct sockaddr_un addr;

	sfd = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (sfd < 0) {
	    exit_error("unable to open socket datagram");
	}

	if (remove(SATELLITE_SOCKET_PATH) == -1 && errno != ENOENT) {
	    exit_error("unable to remove socket file");
	}

	// Construct and bind satellite address
	addr.sun_family = AF_UNIX;
	memset(&addr, 0, sizeof(struct sockaddr_un));
	strncpy(addr.sun_path, SATELLITE_SOCKET_PATH, sizeof(addr.sun_path) - 1);


	// Bind the socket to an established address
	if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
		exit_error("unable to bind socket");
	}

	printf("Socket --> %s\n", SATELLITE_SOCKET_PATH);

	char *buf = "101010";

    printf("Ready to send file...\n");
	tftp_send_file(sfd, buf, strlen(buf));

	close(sfd);
	unlink(SATELLITE_SOCKET_PATH);
	exit(0);
}
