
CERRWARN =	-Wall -Wextra -Werror \
		-Wno-unused-parameter \

CC =		gcc

waker: waker.c
	$(CC) -std=c99 -D_GNU_SOURCE -D__EXTENSIONS__ $(CERRWARN) -o $@ $^

