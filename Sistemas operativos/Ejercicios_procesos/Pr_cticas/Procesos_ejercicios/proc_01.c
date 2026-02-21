//proc_01.c

#include <stdio.h>
#include <unistd.h>
main() {
	pid_t p;
	p = fork();
	if (p != 0) while (1) sleep(1000);
	else exit (2);
}