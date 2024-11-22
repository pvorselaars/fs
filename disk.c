#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

void usage(char *bin){
	printf("usage: %s [-s sectors] [-f file]\n", bin);
	printf("\t\tsize: number of sectors (default 10)\n");
	printf("\t\tfile: name of output file\n");
}

int main(int argc, char *argv[])
{
	int output;
	int size = 10;
	char *file = "disk.img";
	int c;

	while ((c = getopt(argc, argv, "s:f:h")) != -1){
		switch (c) {
        case 's':
						if (isdigit(*optarg)){
							size = atoi(optarg);
						} else {
							fprintf(stderr, "%s is not a valid number of sectors\n", optarg);
						}
            break;
        case 'f':
						file = optarg;
            break;
				case 'h':
						usage(argv[0]);
						exit(EXIT_SUCCESS);
					  break;
        case '?':
						usage(argv[0]);
						exit(EXIT_FAILURE);
            break;
        }
  }

	size = size * 512;

	// Process remaining non-option arguments
	for (; optind < argc; optind++) {
			printf("Extra argument: %s\n", argv[optind]);
	}

	output = open(file, 
								O_CREAT | O_WRONLY | O_TRUNC, 
								S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if (output == -1) {
		fprintf(stderr, "%s: %s", argv[0], strerror(errno));
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < size; i++){
		if (write(output, "\0", 1) != 1) {
			fprintf(stderr, "%s: %s", argv[0], strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	if (close(output) == -1) {
		fprintf(stderr, "%s: %s", argv[0], strerror(errno));
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
