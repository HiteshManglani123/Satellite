/*
    The client socket connects to the server socket.
    Client = Earth
    Server = Satellite

    The Earth (Client) sends a read request to the Satellite (Server) such that the Satellite can begin
    sending data towards Earth.
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
#define GROUND_STATION_SOCKET_PATH "temp/client-socket"

#define DATA "Hello, world!\n"

#define BUF_SIZE 100


void exit_error(char *s) {
	perror(s);
	exit(1);
}

int main(int argc, char *argv[]) {
   	int sfd;
	struct sockaddr_un satellite_addr, ground_station_addr;

	/* Create socket. It is automatically marked as "active" and can be used to connect to a
	    "passive" socket
	*/
	sfd = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (sfd < 0) {
	    exit_error("unable to open socket stream");
	}

	if (remove(GROUND_STATION_SOCKET_PATH) == -1 && errno != ENOENT) {
	    exit_error("unable to remove socket file");
	}

	// Construct ground station address
	ground_station_addr.sun_family = AF_UNIX;
	memset(&ground_station_addr, 0, sizeof(struct sockaddr_un));
	strncpy(ground_station_addr.sun_path, GROUND_STATION_SOCKET_PATH, sizeof(ground_station_addr.sun_path) -1);


	if (bind(sfd, (struct sockaddr *) &ground_station_addr, sizeof(struct sockaddr_un)) == -1) {
	    exit_error("unable to bind client socket");
	}

	// Construct server address (no need to bind)
	satellite_addr.sun_family = AF_UNIX;
	memset(&satellite_addr, 0, sizeof(struct sockaddr_un));
	strncpy(satellite_addr.sun_path, SATELLITE_SOCKET_PATH, sizeof(satellite_addr.sun_path) -1 );

	tftp_retrieve_file(sfd, satelite_addr, NULL, 1);

	close(sfd);
	unlink(GROUND_STATION_SOCKET_PATH);
	exit(0);
}
