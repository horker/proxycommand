#include <stdio.h>

int main(int argc, char* argv[])
{
	if (argc > 1) {
		int i;
		for (i = 1; i < argc - 1; ++i) {
			printf("%s ", argv[i]);
		}
		puts(argv[i]);
	}

    return 0;
}

