#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define MAX_PATH 2048

/* RISCOS attributes */
#define ROA_READ   ((1<<0) | (1<<4))
#define ROA_WRITE  2
#define ROA_LOCKED 8

#pragma pack(push, 4)

typedef struct
{
	uint32_t  loadaddr;
	uint32_t  execaddr;
	uint32_t  flags;
} RISCOS_ATTRIBS;

typedef struct
{
	char filename[16];
	RISCOS_ATTRIBS attr;
} EA_RECORD;

#pragma pack(pop)

static bool file_exists(const char* name)
{
	FILE* file = fopen(name, "rb");

	if (file)
	{
		fclose(file);
		return true;
	}

	return false;
}

static void process_record(const EA_RECORD* record)
{
	if (record->filename[0] != 0) /* The record is in use */
	{
		char filename[20];

		/*                                                         11 */
		/*                    0123456789                 012345678901 */
		/* RISC OS file name 'abcdefghij' is encoded as 'abcdefgh.~ij'*/

		if ((record->filename[8] == '.') && (record->filename[9] == '~'))
		{
			memmove(filename, record->filename, 8);
			memmove(&filename[8], &record->filename[10], 3);
		}
		else
		{
			strcpy(filename, record->filename);
		}

		printf("Found file entry '%s': Load %08X Exec %08X Flags %08X\n", filename, record->attr.loadaddr, record->attr.execaddr, record->attr.flags);

		// TODO Determine if the load and exec addr specify a file type and append to the name if they do

		/* File is different */
		if ((strcmp(filename, record->filename) != 0) && file_exists(record->filename))
		{
			printf("TODO: Rename file %s to %s\n", record->filename, filename);
		}

	}
}

int main(int argc, char* argv[])
{
	char buffer[MAX_PATH];
	FILE* riscos_ea = NULL;

	if (argc < 2)
	{
		fprintf(stderr, "Usage %s <directory path>\n\nIf the directory contains a RISCOS.EA file, rename the files in the directory to remove the old name encoding and add the file type suffix\n", argv[0]);
		return EXIT_FAILURE;
	}

	int count = snprintf(buffer, sizeof(buffer), "%s/RISCOS.EA", argv[1]);

	if (count < 0 || count >= sizeof(buffer))
	{
		fprintf(stderr, "Directory path too long (%d)\n", count);
		return EXIT_FAILURE + 1;
	}

	if ((riscos_ea = fopen(buffer, "rb")) == NULL)
	{
		fprintf(stderr, "Failed to open '%s': %s\n", buffer, strerror(errno));
		return EXIT_FAILURE + 2;
	}

	while (!feof(riscos_ea) && !ferror(riscos_ea))
	{
		EA_RECORD record;

		if (fread(&record, sizeof(record), 1, riscos_ea) == 1)
		{
			process_record(&record);
		}
	}

	fclose(riscos_ea);

	return EXIT_SUCCESS;
}