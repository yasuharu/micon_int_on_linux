default:
	gcc -g -o test main.c ringbuf.c -Wall -DLINUX_TEST -lpthread
