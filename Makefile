default:
	gcc -g -o test main.c ringbuf.c -DLINUX_TEST -lpthread
