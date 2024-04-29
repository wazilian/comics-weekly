
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "global.h"

const int DAY_SECONDS  = 86400;
const int WEEK_SECONDS = 604800;
const int DATE_MAX = 11;

const int dayDelta[] = {
  DAY_SECONDS * 3,      /* Sunday, seconds till Wednesday */
  DAY_SECONDS * 2,      /* Monday, seconds till Wednesday */
  DAY_SECONDS,          /* Tuesday, seconds till Wednesday */
  0,                    /* Wednesday */
  -(DAY_SECONDS * 1),   /* Thursday, seconds since Wednesday */
  -(DAY_SECONDS * 2),   /* Friday, seconds since Wednesday */
  -(DAY_SECONDS * 3)    /* Saturday, seconds since Wednesday */
};

const int weekDelta[] = {
  -WEEK_SECONDS,  /* previous week, seconds since last week */
  0,              /* current week */
  WEEK_SECONDS    /* next week, seconds till last week */
};

const int number_of_weeks = sizeof(weekDelta) / sizeof(weekDelta[0]);

/* function to get the store dates for Wednesday of the previous, current, and next week */
void getStoreDates(char **dates) {
  int dst = 3600;   /* accomodate for DST, assuming cron job is Sunday at midnight (00:00:00) */
  time_t currentTime = time(NULL) + dst;
  struct tm tm_temp = *localtime(&currentTime);
  time_t timestamp;
  struct tm tm;
  int i;
  char temp[DATE_MAX];

  for (i = 0; i < number_of_weeks; i++) {
    timestamp = currentTime + weekDelta[i] + dayDelta[tm_temp.tm_wday];
    tm = *localtime(&timestamp);

    snprintf(temp, DATE_MAX, "%d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    if ((dates[i] = (char *) calloc(strlen(temp) + 1, sizeof(char))) == NULL) {
      printf("\n\tError: Could not allocate memory for store dates.\n\n");
      exit(0);
    }

    memcpy(dates[i], temp, strlen(temp));
  }
}

/* add publisher to list */
void addPublisher(char *data) {
  Publisher *newPublisher = malloc(sizeof(Publisher));

  /* check if memory was available */
  if (newPublisher == NULL) {
    exit(1);
  }

  /* if list is empty */
  if (headPublisher == NULL) {
    headPublisher = newPublisher;
    newPublisher->next = NULL;
  } else {
    newPublisher->next = headPublisher;
    headPublisher = newPublisher;
  }

  newPublisher->data = malloc(strlen(data) + 1);
  strcpy(newPublisher->data, data);
}

/* function to open the provided output text files */
int open_output_files(FILE **fp1, FILE **fp2, FILE **fp3, char *dir, char **storeDates) {
  char filename[255];

  memset(filename, '\0', 255);
  sprintf(filename, "%s/%s.txt", dir, storeDates[0]);

	if ((*fp1 = fopen(filename, "w")) == NULL) {
		printf("\n\tError: Failure opening the file %s file for writing.\n\n", storeDates[0]);
		return -1;
	}

  memset(filename, '\0', 255);
  sprintf(filename, "%s/%s.txt", dir, storeDates[1]);

	if ((*fp2 = fopen(filename, "w")) == NULL) {
		printf("\n\tError: Failure opening the file %s file for writing.\n\n", storeDates[1]);
		return -1;
	}

  memset(filename, '\0', 255);
  sprintf(filename, "%s/%s.txt", dir, storeDates[2]);

	if ((*fp3 = fopen(filename, "w")) == NULL) {
		printf("\n\tError: Failure opening the file %s file for writing.\n\n", storeDates[2]);
		return -1;
	}

  return 1;
}

/* print out command line parameters */
void printCmdArgs(const char *app_name) {
  printf("\n\tError: Incorrect parameter list.\n");
  printf("\n\t# ./%s <Metron.cloud usernme> <Metron.cloud password> <out text files directory> <space separated list of comic publishers>\n\n", app_name);
}
