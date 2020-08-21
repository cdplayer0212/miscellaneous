#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<limits.h>
#include	<errno.h>

unsigned long fic_recursion(unsigned long num)
{
	if (num == 0)
		return 0;
	else if (num == 1 || num == 2)
		return 1;

	return fic_recursion(num - 1) + fic_recursion(num - 2);
}

unsigned long fic_iteration(unsigned long num)
{
	unsigned long i;
	unsigned long n1 = 0;
	unsigned long n2 = 1;
	unsigned long result = num;
	for (i = 2; i <= num; i++) {
		result = n1 + n2;
		n1 = n2;
		n2 = result;
	}
	return result;

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
	if (data > 0) {
		printf("%s %lu, result: %lu\n", argv[0], data,
						fic_recursion(data));
		printf("%s %lu, result: %lu\n", argv[0], data,
						fic_iteration(data));
	}

	return 0;
}
