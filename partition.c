#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mbr.h"

int read_mbr(const char *filename, mbr_t * mbr)
{
	FILE *disk = fopen(filename, "wb+");
	if (!disk) {
		return -1;
	}

	if (!fread(mbr, sizeof(mbr_t), 1, disk)) {
		return -2;
	}

	fclose(disk);
	return 0;
}

int write_mbr(const char *filename, mbr_t * mbr)
{
	FILE *disk = fopen(filename, "rb+");
	if (!disk) {
		return -1;
	}

	if (!fwrite(mbr, sizeof(mbr_t), 1, disk)) {
		return -1;
	}

	fclose(disk);
	return 0;
}

void create_partition(mbr_t * mbr)
{
	int index, prev = 0;
	char bootable;

	for (int i = 0; i < NUM_PARTITIONS; i++) {
		if (mbr->partitions[i].type == 0 && mbr->partitions[i].size == 0) {
			index = i;
			break;
		}
	}

	printf("Partition ID (e.g., %d): ", index + 1);
	scanf(" %d", &index);

	if (index > 4 || index < 1) {
		printf("Invalid partition ID\n");
		return;
	}
	index--;

	partition_entry_t *entry = &mbr->partitions[index];

	if (index > 1)
		prev = index - 2;

	printf("Bootable partition (y/n): ");
	scanf(" %c", &bootable);
	entry->boot = (bootable == 'y') ? 0x80 : 0x00;

	printf("Partition type (e.g., 0x%02x): ", entry->type);
	scanf(" %hhx", &entry->type);

	printf("Start sector (e.g., %d): ", mbr->partitions[prev].start_lba + mbr->partitions[prev].size + 1);
	scanf(" %u", &entry->start_lba);

	printf("Number of sectors (e.g., %d): ", entry->size);
	scanf(" %u", &entry->size);
}

void display_partitions(mbr_t * mbr)
{
	printf("%-10s %-10s %-10s %-10s %-4s\n", "Partition", "Bootable", "Start LBA", "Sectors", "Type");
	for (int i = 0; i < NUM_PARTITIONS; i++) {
		partition_entry_t *entry = &mbr->partitions[i];

		if (entry->type == 0 && entry->size == 0)
			continue;

		printf("%-10d %-10c %-10d %-10d 0x%02X\n", i + 1, entry->boot ? '*' : ' ', entry->start_lba,
		       entry->size, entry->type);
	}
}

void usage(char *bin)
{
	printf("usage: %s file\n", bin);
	printf("\t\tfile: name of file to partition\n");
}

int main(int argc, char *argv[])
{
	mbr_t mbr;
	int done = 0;
	const char *filename = "disk.img";
	int c;

	while ((c = getopt(argc, argv, "h")) != -1) {
		switch (c) {
		case 'h':
			usage(argv[0]);
			return 0;
			break;

		case '?':
			usage(argv[0]);
			return 1;
			break;
		}
	}

	if (optind < argc)
		filename = argv[optind];

	// Read MBR from disk
	if (read_mbr(filename, &mbr) == -1) {
		fprintf(stderr, "%s: %s, %s\n", argv[0], filename, strerror(errno));
		return 1;
	}

	if (mbr.boot_signature != 0xaa55)
		mbr.boot_signature = 0xaa55;

	// Display MBR
	printf("Disk signature: %.8x\n", *mbr.disk_signature);

	char option;
	while (!done) {
		display_partitions(&mbr);
		printf("Do you want to create/edit a partition? (y/n): ");
		scanf(" %c", &option);

		switch (option) {
		case 'y':
			create_partition(&mbr);
			break;

		case 'n':
			done = 1;
			break;

		default:
			printf("Invalid input: %c\n", option);
			break;
		}

	}

	printf("Do you want to write the partition table back to disk (y/n)?: ");
	scanf(" %c", &option);

	if (option == 'y') {
		if (write_mbr(filename, &mbr) != 0) {
			fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
			return 1;
		}
	}

}
