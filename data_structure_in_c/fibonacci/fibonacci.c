#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<limits.h>
#include	<errno.h>

unsigned long fic(unsigned long num)
{
	if (num == 0)
		return 0;
	else if (num == 1 || num == 2)
		return 1;

	return fic(num - 1) + fic(num - 2);
}

int main(int argc, char *argv[])
{
	int opt;
	unsigned long data = 0;

	while ((opt = getopt(argc, argv, "n:")) != -1) {
		switch (opt) {
		case 'n':
			data = strtoul(optarg, NULL, 10);
			break;
		default:
			fprintf(stderr, "Usage: %s -n <number>\n", argv[0]);
			break;
		}
	}
	if (data > 0)
		printf("%s %lu, result: %lu\n", argv[0], data, fic(data));

	return 0;
}
