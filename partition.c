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

#pragma pack(1)
typedef struct {
	uint8_t BootIndicator;
	uint8_t StartHead;
	uint8_t StartSector;
	uint8_t StartTrack;
	uint8_t OSIndicator;
	uint8_t EndHead;
	uint8_t EndSector;
	uint8_t EndTrack;
	uint32_t StartingLBA;
	uint32_t Size;
} PartitionRecord;

typedef struct {
	uint8_t BootCode[440];
	uint8_t DiskSignature[4];
	uint8_t Reserved[2];
	PartitionRecord Partition[4];
	uint16_t Sign;
} MasterBootRecord;
#pragma pack()

int output;
MasterBootRecord mbr, disk_mbr;

void menu()
{
	printf("\n");
	printf("p\tprint partition table\n");
	printf("t\tshow partition table on disk\n");
	printf("w\twrite partition table to disk\n");
	printf("n\tnew partition\n");
	printf("h\tshow help\n");
	printf("q\tquit\n");
}

void print_mbr(MasterBootRecord * mbr)
{
	printf("Disk signature: 0x");
	for (int c = 3; c >= 0; c--)
		printf("%x", mbr->DiskSignature[c]);
	printf("\n");

	printf("%-10s %-10s %-10s %-10s %-10s\n", "Partition", "Boot", "Start LBA", "Size", "Type");
	for (int p = 0; p < 4; p++) {
		if (mbr->Partition[p].Size > 0)
			printf("%-10d %-10c %-10d %-10d %-10x\n", p + 1,
			       mbr->Partition[p].BootIndicator ? '*' : ' ', mbr->Partition[p].StartingLBA,
			       mbr->Partition[p].Size, mbr->Partition[p].OSIndicator);
	}
}

void show_mbr()
{
	if (read(output, &disk_mbr, sizeof(disk_mbr)) != sizeof(disk_mbr)) {
		printf("No MBR present on disk\n");
	} else {
		print_mbr(&disk_mbr);
	}

	lseek(output, 0, SEEK_SET);
}

int write_mbr()
{
	if (write(output, &mbr, sizeof(mbr)) != sizeof(mbr)) {
		return -1;
	}

	lseek(output, 0, SEEK_SET);
	return 0;
}

void create_partition()
{
	char *buffer;
	size_t bufsize = 32;
	PartitionRecord part;
	int id = 0;

	buffer = (char *)malloc(bufsize * sizeof(char));
	if (buffer == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	printf("Partition [%d]: ", id + 1);
	getline(&buffer, &bufsize, stdin);
	buffer[strcspn(buffer, "\n")] = 0;

	if (isdigit(*buffer)) {
		id = atoi(buffer) - 1;
		if (id > 4)
			id = 4;
	}

	if (id > 0) {
		printf("Start LBA [%d]: ", mbr.Partition[id - 1].StartingLBA + mbr.Partition[id - 1].Size + 1);
	} else {
		printf("Start LBA [%d]: ", 0);
	}

	getline(&buffer, &bufsize, stdin);
	buffer[strcspn(buffer, "\n")] = 0;

	if (isdigit(*buffer)) {
		part.StartingLBA = atoi(buffer);
	} else {
		part.StartingLBA = 0;
	}

	printf("Size in sectors [%d]: ", 1);

	getline(&buffer, &bufsize, stdin);
	buffer[strcspn(buffer, "\n")] = 0;

	if (isdigit(*buffer)) {
		part.Size = atoi(buffer);
	} else {
		part.Size = 1;
	}

	printf("Partition type [%d]: ", 0);

	getline(&buffer, &bufsize, stdin);
	buffer[strcspn(buffer, "\n")] = 0;

	if (isdigit(*buffer)) {
		part.OSIndicator = atoi(buffer);
	} else {
		part.OSIndicator = 0;
	}

	printf("Bootable [n]: ");

	getline(&buffer, &bufsize, stdin);
	buffer[strcspn(buffer, "\n")] = 0;

	if (!strcmp(buffer, "y")) {
		part.BootIndicator = 0x80;
	} else {
		part.BootIndicator = 0;
	}

	mbr.Partition[id] = part;

	print_mbr(&mbr);

	free(buffer);
}

void usage(char *bin)
{
	printf("usage: %s file\n", bin);
	printf("\t\tfile: name of file to partition\n");
}

int main(int argc, char *argv[])
{
	char *file = NULL;

	int c;

	while ((c = getopt(argc, argv, "h")) != -1) {
		switch (c) {
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

	if (optind < argc) {
		file = argv[optind];
	}

	if (!file) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	output = open(file, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if (output == -1) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		exit(EXIT_FAILURE);
	}

	struct stat sb;
	if (stat(file, &sb) == -1) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		exit(EXIT_FAILURE);
	}

	if ((sb.st_mode & S_IFMT) != S_IFBLK && (sb.st_mode & S_IFMT) != S_IFREG) {
		fprintf(stderr, "%s: %s is not supported as output\n", argv[0], file);
		exit(EXIT_FAILURE);
	}

	show_mbr();
	mbr = disk_mbr;
	mbr.Sign = 0xaa55;

	char *buffer;
	size_t bufsize = 32;

	buffer = (char *)malloc(bufsize * sizeof(char));
	if (buffer == NULL) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		exit(EXIT_FAILURE);
	}

	menu();
	while (strcmp(buffer, "q")) {
		getline(&buffer, &bufsize, stdin);
		buffer[strcspn(buffer, "\n")] = 0;

		if (strcmp(buffer, "h") == 0)
			menu();

		if (strcmp(buffer, "p") == 0)
			print_mbr(&mbr);

		if (strcmp(buffer, "t") == 0)
			show_mbr();

		if (strcmp(buffer, "n") == 0)
			create_partition();

		if (strcmp(buffer, "w") == 0) {

			if (write_mbr() == -1) {
				fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
				exit(EXIT_FAILURE);
			}
			printf("Table written to disk.\n");
		}
	}

	if (close(output) == -1) {
		fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
		exit(EXIT_FAILURE);
	}

	free(buffer);
	exit(EXIT_SUCCESS);
}
