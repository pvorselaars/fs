#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "mbr.h"

#pragma pack(1)
typedef struct {
	uint8_t jmp[3];
	uint8_t oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t num_fats;
	uint16_t root_entries;
	uint16_t total_sectors;
	uint8_t media_type;
	uint16_t fat_size;
	uint16_t sectors_per_track;
	uint16_t num_heads;
	uint32_t hidden_sectors;
	uint32_t total_sectors_32;
	uint8_t drive_number;
	uint8_t reserved;
	uint8_t ext_boot_signature;
	uint32_t serial_number;
	uint8_t volume_label[11];
	uint8_t file_system[8];
	uint8_t boot_code[448];
	uint16_t signature;
} bpb16_t;

typedef uint8_t fat12_t;

typedef struct {
	uint8_t filename[8];
	uint8_t extension[3];
	uint8_t attributes;
	uint8_t reserved[2];
	uint16_t create_time;
	uint16_t create_date;
	uint16_t last_access_date;
	uint8_t ignored[2];
	uint16_t last_write_time;
	uint16_t last_write_date;
	uint16_t cluster;
	uint32_t size;
} root_entry_t;

bpb16_t *get_bpb16(FILE * disk)
{
	bpb16_t *bpb = (bpb16_t *) malloc(sizeof(bpb16_t));

	fseek(disk, 0, SEEK_SET);
	if (!fread(bpb, sizeof(*bpb), 1, disk)) {
		fprintf(stderr, "%s", strerror(errno));
		free(bpb);
		return NULL;
	}

	if (bpb->signature != 0xaa55) {
		fprintf(stderr, "Invalid boot sector signature\n");
		free(bpb);
		return NULL;
	}

	return bpb;
}

int display_bpb16(bpb16_t * bpb)
{

	printf("Jump instruction:\t");
	for (int i = 0; i < 3; i++) {
		printf("%02X ", bpb->jmp[i]);
	}
	printf("\n");

	printf("OEM name:\t\t");
	for (int i = 0; i < 8; i++) {
		printf("%c", (bpb->oem_name[i] >= 32 && bpb->oem_name[i] <= 126) ? bpb->oem_name[i] : '.');
	}
	printf("\n");

	printf("Bytes per sector:\t%u\n", bpb->bytes_per_sector);

	printf("Sectors per cluster:\t%u\n", bpb->sectors_per_cluster);

	printf("Reserved sectors:\t%u\n", bpb->reserved_sectors);

	printf("Number of FATs:\t\t%u\n", bpb->num_fats);

	printf("Root directory entries:\t%u\n", bpb->root_entries);

	printf("Total sectors (16-bit):\t%u\n", bpb->total_sectors);

	printf("Media type:\t\t0x%02X\n", bpb->media_type);

	printf("FAT size:\t\t%u\n", bpb->fat_size);

	printf("Sectors per track:\t%u\n", bpb->sectors_per_track);

	printf("Number of heads:\t%u\n", bpb->num_heads);

	printf("Hidden sectors:\t\t%u\n", bpb->hidden_sectors);

	printf("Total sectors (32-bit):\t%u\n", bpb->total_sectors_32);

	printf("Drive number:\t\t0x%02X\n", bpb->drive_number);

	printf("Reserved:\t\t0x%02X\n", bpb->reserved);

	printf("Ext. boot signature:\t0x%02X\n", bpb->ext_boot_signature);

	printf("Serial number:\t\t0x%08X\n", bpb->serial_number);

	printf("Volume label:\t\t");
	for (int i = 0; i < 11; i++) {
		printf("%c", (bpb->volume_label[i] >= 32 && bpb->volume_label[i] <= 126) ? bpb->volume_label[i] : '.');
	}
	printf("\n");

	printf("File system:\t\t");
	for (int i = 0; i < 8; i++) {
		printf("%c", (bpb->file_system[i] >= 32 && bpb->file_system[i] <= 126) ? bpb->file_system[i] : '.');
	}
	printf("\n");

	printf("Boot code:\t\t");
	for (int i = 0; i < 448; i++) {
		if (i % 16 == 0 && i != 0) {
			printf("\n\t\t\t");
		}
		printf("%02X ", bpb->boot_code[i]);
	}
	printf("\n");

	printf("Signature:\t\t0x%04X\n", bpb->signature);

	return 0;
}

fat12_t *get_fat12(FILE * disk, bpb16_t * bpb)
{
	// Calculate base address
	uint32_t offset = bpb->reserved_sectors * bpb->bytes_per_sector;
	uint32_t size = bpb->fat_size * bpb->bytes_per_sector;

	// Read FAT from disk
	fseek(disk, offset, SEEK_SET);
	fat12_t *fat = (fat12_t *) malloc(size);
	if (!fread(fat, 1, size, disk)) {
		fprintf(stderr, "%s", strerror(errno));
		return NULL;
	}

	return fat;
}

uint16_t get_fat12_entry(uint16_t i, fat12_t * fat)
{
	int entry_index = i * 12 / 8;
	int entry_offset = (i * 12) % 8;
	uint16_t entry = (fat[entry_index] >> entry_offset) | (fat[entry_index + 1] << (8 - entry_offset));

	return entry;
}

uint32_t cluster_to_address(uint16_t cluster, bpb16_t * bpb)
{

	return (bpb->reserved_sectors + bpb->num_fats * bpb->fat_size +
		(cluster - 2) * bpb->sectors_per_cluster) * bpb->bytes_per_sector +
	    bpb->root_entries * sizeof(root_entry_t);
}

void display_fat12(fat12_t * fat, bpb16_t * bpb)
{
	uint32_t size = bpb->fat_size * bpb->bytes_per_sector;
	int entries = size * 8 / 12;
	printf("FAT12 entries:\n");
	for (int i = 0; i < entries; i++) {
		int entry_index = i * 12 / 8;
		int entry_offset = (i * 12) % 8;
		uint16_t entry = (fat[entry_index] >> entry_offset) | (fat[entry_index + 1] << (8 - entry_offset));

		if (i % 4 == 0)
			printf("\n");

		if (entry >= 0xFF8) {
			printf("%-4d: In use (0x%04x) ", i, entry);
		} else {
			printf("%-4d: %-15d ", i, entry);
		}
	}
	printf("\n");
}

root_entry_t *get_rootdir(FILE * disk, bpb16_t * bpb)
{

	// Calculate base address
	uint32_t offset = (bpb->reserved_sectors + bpb->num_fats * bpb->fat_size) * bpb->bytes_per_sector;
	uint32_t size = (bpb->root_entries * sizeof(root_entry_t));

	// Read root directory from disk
	fseek(disk, offset, SEEK_SET);
	root_entry_t *rootdir = (root_entry_t *) malloc(size);
	if (!fread(rootdir, 1, size, disk)) {
		fprintf(stderr, "%s", strerror(errno));
		return NULL;
	}

	return rootdir;
}

void get_fullname(char *buffer, const root_entry_t * entry)
{
	char name[9], ext[4];
	name[8] = '\0';
	ext[3] = '\0';
	buffer[0] = '\0';

	memcpy(name, entry->filename, 8);
	memcpy(ext, entry->extension, 3);

	// null terminate space padded strings
	for (int c = 7; c >= 0; c--) {
		if (name[c] == ' ') {
			name[c] = '\0';
		} else {
			break;
		}
	}

	for (int c = 2; c >= 0; c--) {
		if (ext[c] == ' ') {
			ext[c] = '\0';
		} else {
			break;
		}
	}

	if (ext[0] != '\0') {
		strcat(buffer, name);
		strcat(buffer, ".");
		strcat(buffer, ext);
	} else {
		strcat(buffer, name);
	}
}

void display_rootdir(char details, root_entry_t * rootdir, bpb16_t * bpb)
{
	char name[13];
	uint8_t hours, minutes, dseconds, month, day;
	uint16_t year;

	// Print all valid entries
	for (int i = 0; i < bpb->root_entries; i++) {

		if (rootdir[i].filename[0] == 0)
			continue;

		// Print filename and extension
		get_fullname(name, &rootdir[i]);

		if (details) {
			day = rootdir[i].create_date & 0x1f;
			month = (rootdir[i].create_date >> 5) & 0xf;
			year = ((rootdir[i].create_date >> 9) & 0x7f) + 1980;

			hours = (rootdir[i].create_time >> 11) & 0x1f;
			minutes = (rootdir[i].create_time >> 5) & 0x0f;
			dseconds = (rootdir[i].create_time & 0x1f) * 2;
			printf("%-12s %16d B %02d:%02d:%02d %d-%d-%d\n", name, rootdir[i].size, hours, minutes,
			       dseconds, day, month, year);
		} else {
			printf("%-12s\n", name);
		}
	}
}

void get_file(const char *filename, FILE * disk, root_entry_t * rootdir, fat12_t * fat, bpb16_t * bpb)
{
	char name[13];

	for (int i = 0; i < bpb->root_entries; i++) {
		if (rootdir[i].filename[0] == 0)
			continue;

		get_fullname(name, &rootdir[i]);

		if (strcmp(name, filename) == 0) {
			char *buffer = (char *)malloc(bpb->sectors_per_cluster * bpb->bytes_per_sector);

			uint32_t offset;
			uint16_t cluster = rootdir[i].cluster;

			while (cluster < 0xff6 && cluster != 0) {

				offset = cluster_to_address(cluster, bpb);
				fseek(disk, offset, SEEK_SET);
				fread(buffer, 1, bpb->sectors_per_cluster * bpb->bytes_per_sector, disk);
				printf("%s", buffer);

				cluster = get_fat12_entry(cluster, fat);
			}

			free(buffer);
			break;
		}
	}
}

void usage(char *bin)
{
	printf("Usage: %s [options] <disk>\n", bin);
	printf("\n");
	printf("Options:\n");
	printf("  -h		show this help message and exit\n");
	printf("  -l		list files\n");
	printf("  -d		list file details\n");
	printf("  -b		show boot parameter block\n");
	printf("  -f		show FAT\n");
}

int main(int argc, char *argv[])
{
	char *filename;
	char list_flag, bpb_flag, fat_flag, details_flag = 0;
	int c;

	while ((c = getopt(argc, argv, "hldbgf")) != -1) {
		switch (c) {
		case 'l':
			list_flag = 1;
			break;

		case 'd':
			details_flag = 1;
			break;

		case 'b':
			bpb_flag = 1;
			break;

		case 'f':
			fat_flag = 1;
			break;

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

	if (optind < argc) {
		filename = argv[optind];
	} else {
		usage(argv[0]);
		return 1;
	}

	FILE *disk = fopen(filename, "rb");

	bpb16_t *bpb = get_bpb16(disk);
	if (bpb == NULL)
		return -1;

	if (bpb_flag) {
		display_bpb16(bpb);
	}

	fat12_t *fat = get_fat12(disk, bpb);
	if (fat == NULL)
		return -1;

	if (fat_flag) {
		display_fat12(fat, bpb);
	}

	root_entry_t *rootdir = get_rootdir(disk, bpb);
	if (rootdir == NULL)
		return -1;

	if (list_flag) {
		display_rootdir(details_flag, rootdir, bpb);
	}

	fclose(disk);

	free(bpb);
	free(fat);
	free(rootdir);

	return 0;
}
