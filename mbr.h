#ifndef MBR_H
#define MBR_H

#include <stdint.h>

#define BOOTLOADER_SIZE 440
#define NUM_PARTITIONS 4

#pragma pack(1)
typedef struct {
	uint8_t boot;
	uint8_t start_head;
	uint8_t start_sector;
	uint8_t start_track;
	uint8_t type;
	uint8_t end_head;
	uint8_t end_sector;
	uint8_t end_track;
	uint32_t start_lba;
	uint32_t size;
} partition_entry_t;

typedef struct {
	uint8_t bootloader[BOOTLOADER_SIZE];
	uint8_t disk_signature[4];
	uint8_t reserved[2];
	partition_entry_t partitions[4];
	uint16_t boot_signature;
} mbr_t;
#pragma pack()

#endif
