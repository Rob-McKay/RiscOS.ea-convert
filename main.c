/*
 * MIT License
 *
 * Copyright (c) 2020 Rob McKay
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <sys/stat.h>

#define MAX_PATH 2048

#define DEFAULT_FILE_TYPE 0xFFFu

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

static bool is_directory(const char* base_path, const char* name)
{
	struct stat file_stat;
	char buffer[MAX_PATH];

	int count = snprintf(buffer, sizeof(buffer), "%s/%s", base_path, name);

	if (stat(buffer, &file_stat) == 0)
	{
		if (file_stat.st_mode & S_IFDIR)
			return true;
	}

	return false;
}

static bool file_exists(const char* base_path, const char* name)
{
	struct stat file_stat;
	char buffer[MAX_PATH];

	int count = snprintf(buffer, sizeof(buffer), "%s/%s", base_path, name);

	if (stat(buffer, &file_stat) == 0)
	{
		return true;
	}

	return false;
}

static void process_record(const EA_RECORD* record, const char* base_path)
{
	if (record->filename[0] != 0) /* The record is in use */
	{
		char filename[40];

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

		if (!is_directory(base_path, record->filename))
		{
			if ((record->attr.loadaddr & 0xFFF00000) == 0xFFF00000) /* file has type and date */
			{
				int file_type = (record->attr.loadaddr & ~0xFFF00000u) >> 8;

				if (file_type != DEFAULT_FILE_TYPE)
				{
					char suffix[20];
					sprintf(suffix, ",%x", file_type);
					strcat(filename, suffix);
				}
			}
			else /* File has load and exec addresses */
			{
				char suffix[20];
				sprintf(suffix, ",%x-%x", record->attr.loadaddr, record->attr.execaddr);
				strcat(filename, suffix);
			}
		}

		/* File is different */
		if ((strcmp(filename, record->filename) != 0) && file_exists(base_path, record->filename))
		{
			char orig_name[MAX_PATH];
			char new_name[MAX_PATH];

			printf("Found file entry '%s': Load %08X Exec %08X Flags %08X\n", filename, record->attr.loadaddr, record->attr.execaddr, record->attr.flags);

			snprintf(orig_name, sizeof(orig_name), "%s/%s", base_path, record->filename);
			snprintf(new_name, sizeof(new_name), "%s/%s", base_path, filename);

			printf("Renaming '%s' to '%s'\n", orig_name, new_name);
			if (rename(orig_name, new_name))
			{
				fprintf(stderr, "Failed to rename '%s' to '%s': %s\n", orig_name, new_name, strerror(errno));
			}
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
			process_record(&record, argv[1]);
		}
	}

	fclose(riscos_ea);

	return EXIT_SUCCESS;
}