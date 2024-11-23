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

typedef struct {
	uint8_t filename[8];
	uint8_t extension[3];
	uint8_t attributes;
	uint8_t reserved[10];
	uint16_t time;
	uint16_t date;
	uint16_t cluster;
	uint32_t size;
} root_entry_t;

int display_bpb16(FILE * disk, bpb16_t * bpb)
{

	fseek(disk, 0, SEEK_SET);
	if (!fread(bpb, sizeof(*bpb), 1, disk)) {
		fprintf(stderr, "%s", strerror(errno));
		return -1;
	}

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

	printf("Signature:\t\t0x%04X\n\n", bpb->signature);

	return 0;
}

int display_fat12(FILE * disk, bpb16_t * bpb)
{

	// Calculate base address
	uint32_t offset = bpb->reserved_sectors * bpb->bytes_per_sector;
	uint32_t size = bpb->fat_size * bpb->bytes_per_sector;

	// Read FAT from disk
	fseek(disk, offset, SEEK_SET);
	uint8_t *fat = (uint8_t *) malloc(size);
	if (!fread(fat, 1, size, disk)) {
		fprintf(stderr, "%s", strerror(errno));
		return -1;
	}

	// Print all entries
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
	printf("\n");

	free(fat);
	return 0;
}

int display_rootdir(FILE * disk, bpb16_t * bpb)
{
	// Calculate base address
	uint32_t offset = (bpb->reserved_sectors + bpb->num_fats * bpb->fat_size) * bpb->bytes_per_sector;
	uint32_t size = (bpb->root_entries * sizeof(root_entry_t));

	// Read root directory from disk
	fseek(disk, offset, SEEK_SET);
	root_entry_t *root_dir = (root_entry_t *) malloc(size);
	if (!fread(root_dir, 1, size, disk)) {
		fprintf(stderr, "%s", strerror(errno));
		return -1;
	}

	// Print all valid entries
	printf("Root directory entries:\n");
	for (int i = 0; i < bpb->root_entries; i++){

		if (root_dir[i].filename[0] == 0)
			continue;

		// Print filename and extension
		for (int c = 0; c < 8; c++) {
			printf("%c", root_dir[i].filename[c]);
		}
		printf(".");
		for (int c = 0; c < 3 && root_dir[i].extension[c] != 0; c++) {
			printf("%c", root_dir[i].extension[c]);
		}
		printf("\t%dB @%d\n", root_dir[i].size, root_dir[i].cluster);
	}

	free(root_dir);
	return 0;
}

int main(int argc, char *argv[])
{

	bpb16_t bpb;
	const char *filename = "disk.img";

	FILE *disk = fopen(filename, "rb");

	if (display_bpb16(disk, &bpb) != 0)
		return -1;

	if (display_fat12(disk, &bpb) != 0)
		return -1;

	if (display_rootdir(disk, &bpb) != 0)
		return -1;

	fclose(disk);

	return 0;
}
