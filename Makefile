CC=gcc
CFLAGS=-Wall -Werror -O3 -g -Ilibnet

CPPFLAGS=-Wall -Werror -O2 -g -I../libnet

utime:

clean:
	rm -rf utime *.o utime.dSYM

