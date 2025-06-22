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

	unlink(SATELLITE_SOCKET_PATH);  // Clean up any old socket file

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

	printf("Sfd: %d - Socket: %s\n", sfd, SATELLITE_SOCKET_PATH);

	FILE *fp_image = fopen("images/some-random-stars.bmp", "r");

    if (fp_image == NULL) {
        fprintf(stderr, "Unable to find image\n");
        return -1;
    }

    fseek(fp_image, 0L, SEEK_END);
    long image_size = ftell(fp_image);
    rewind(fp_image);

    char *buf = malloc(image_size);

    if (!buf) {
        fprintf(stderr, "unable to allocate space for buf: %lu\n", image_size);
        fclose(fp_image);
        exit(1);
    }

    fread(buf, 1, image_size, fp_image);

    fclose(fp_image);

    printf("Ready to send file with size: %lu\n", image_size);
	tftp_send_file(sfd, buf, image_size, "[SATELLITE]");

	free(buf);

	close(sfd);
	unlink(SATELLITE_SOCKET_PATH);
	exit(0);
}
