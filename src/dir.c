
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "dir.h"

const char OUTPUT_FILE_EXT[] = ".txt";

// Function to remove all OUTPUT_FILE_EXT files from given directory
void clear_output_dir(char *output_dir) {
	DIR *dir;
	struct dirent *file_ptr;

	if ((dir = opendir(output_dir)) == NULL) {
		printf("\n\tError: %s could not be opened. Permissions?\n\n", output_dir);
		exit(EXIT_FAILURE);
	} else {
		while ((file_ptr = readdir(dir)) != NULL) {
			// Only remove file if its extension is OUTPUT_FILE_EXT
			if (strcmp(strrchr(file_ptr->d_name, '\0') - strlen(OUTPUT_FILE_EXT), OUTPUT_FILE_EXT) == 0) {
				char temp[512];
				snprintf(temp, 512, "%s/%s", output_dir, file_ptr->d_name);

				if (remove(temp) != 0) {
					printf("\n\tError: %s could not be deleted. Permissions?\n\n", temp);
					exit(EXIT_FAILURE);
				}
			}
		}

		// Close directory
		closedir(dir);
	}
}

// Function to open the provided output text files
void open_output_files(FILE **fp1, FILE **fp2, FILE **fp3, char *dir, char **store_dates_a) {
	char filename[255];

	memset(filename, '\0', 255);
	sprintf(filename, "%s/%s.txt", dir, store_dates_a[0]);

	if ((*fp1 = fopen(filename, "w")) == NULL) {
		printf("\n\tError: Failure opening the file %s file for writing.\n\n", store_dates_a[0]);
		exit(EXIT_FAILURE);
	}

	memset(filename, '\0', 255);
	sprintf(filename, "%s/%s.txt", dir, store_dates_a[1]);

	if ((*fp2 = fopen(filename, "w")) == NULL) {
		printf("\n\tError: Failure opening the file %s file for writing.\n\n", store_dates_a[1]);
		exit(EXIT_FAILURE);
	}

	memset(filename, '\0', 255);
	sprintf(filename, "%s/%s.txt", dir, store_dates_a[2]);

	if ((*fp3 = fopen(filename, "w")) == NULL) {
		printf("\n\tError: Failure opening the file %s file for writing.\n\n", store_dates_a[2]);
		exit(EXIT_FAILURE);
	}
}
