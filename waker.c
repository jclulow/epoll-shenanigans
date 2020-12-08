#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <err.h>

void
xclose(int fd)
{
	if (close(fd) != 0) {
		err(1, "close(%d)", fd);
	}
}

void *
xmalloc(size_t sz)
{
	void *x;

	if ((x = malloc(sz)) == NULL) {
		err(1, "malloc(%lu)", (unsigned long)sz);
	}

	return (x);
}

int
main(int argc, char *argv[])
{
	char *rbuf = NULL, *wbuf = NULL;
	unsigned read_every = 10;
	unsigned write_every = 10;
	size_t rbufsz = 0, wbufsz = 1;
	int c;

	while ((c = getopt(argc, argv, ":r:w:R:W:")) != -1) {
		switch (c) {
		case 'r':
			rbufsz = atoi(optarg);
			break;
		case 'w':
			wbufsz = atoi(optarg);
			break;
		case 'W':
			write_every = atoi(optarg);
			break;
		case 'R':
			read_every = atoi(optarg);
			break;
		case ':':
			errx(1, "option -%c requires an operand", optopt);
			break;
		case '?':
			errx(1, "option -%c unrecognised", optopt);
			break;
		}
	}

	if (rbufsz > 0) {
		rbuf = xmalloc(rbufsz);
	}
	if (wbufsz > 0) {
		wbuf = xmalloc(wbufsz);
		for (size_t x = 0; x < wbufsz; x++) {
			wbuf[x] = (char)x;
		}
	}

	int fd[2];
	if (pipe2(fd, O_NONBLOCK) != 0) {
		err(1, "pipe2");
	}
	printf("pipe -> [%d, %d]\n", fd[0], fd[1]);

	int ep;
	if ((ep = epoll_create1(EPOLL_CLOEXEC)) < 0) {
		err(1, "epoll_create1");
	}

	struct epoll_event epe = {
		.events = EPOLLIN | EPOLLRDHUP | EPOLLET,
		.data = { .u32 = 10 },
	};
	if (epoll_ctl(ep, EPOLL_CTL_ADD, fd[0], &epe) != 0) {
		err(1, "epoll_ctl(EPOLL_CTL_ADD, %d)", fd[0]);
	}

	struct epoll_event recv;
	(void) memset(&recv, 0, sizeof (recv));
	unsigned lc = 0;
	for (;;) {
		int r = epoll_wait(ep, &recv, 1, 100);
		printf("wait -- %d\n", r);

		if (r == 1) {
			printf("*** WAKE! (%u) ***\n", recv.data.u32);
		}

		++lc;
		if (lc % read_every == 0 && rbufsz > 0) {
			ssize_t rsz;

			if ((rsz = read(fd[0], rbuf, rbufsz)) >= 0) {
				printf("\tread: [");
				for (ssize_t x = 0; x < rsz; x++) {
					printf(" %u",
					    (unsigned)rbuf[x]);
				}
				printf(" ]\n");
			}
		}
		if (lc % write_every == 0) {
			printf("\twrite: [");
			for (size_t x = 0; x < wbufsz; x++) {
				printf(" %u", (unsigned)wbuf[x]);
			}
			printf(" ]\n");
			if (write(fd[1], wbuf, wbufsz) != (ssize_t)wbufsz) {
				err(1, "write failure");
			}
		}
	}

	xclose(fd[0]);
	xclose(fd[1]);
	xclose(ep);
}
