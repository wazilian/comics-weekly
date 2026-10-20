
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "dates.h"

const int DATE_MAX = 11;	// YYYY-MM-DD
const int PULL_WEEKS = 3;
const int DAY_SECONDS = 86400;
const int THREE_DAY_SECONDS = 259200;
const int WEEK_SECONDS = 604800;

const int day_delta_a[] = {
	DAY_SECONDS * 3,		// Sunday, seconds till Wednesday
	DAY_SECONDS * 2,		// Monday, seconds till Wednesday
	DAY_SECONDS,			// Tuesday, seconds till Wednesday
	0,						// Wednesday
	-(DAY_SECONDS * 1),	 	// Thursday, seconds since Wednesday
	-(DAY_SECONDS * 2),	 	// Friday, seconds since Wednesday
	-(DAY_SECONDS * 3)		// Saturday, seconds since Wednesday
};

const int week_delta_a[] = {
	-WEEK_SECONDS,	// Previous week, seconds since last week
	0,				// Current week
	WEEK_SECONDS	// Next week, seconds till last week
};

// Function to get the store dates for Wednesday of the previous, current, and next week
void get_store_dates(char **dates) {
	int dst = 3600;		// Accomodate for DST, assuming cron job is Sunday at midnight (00:00:00)
	time_t current_time = time(NULL) + dst;
	struct tm tm_temp = *localtime(&current_time);
	time_t timestamp;
	struct tm tm;
	int i;
	char temp[DATE_MAX];

	for (i = 0; i < PULL_WEEKS; i++) {
		timestamp = current_time + week_delta_a[i] + day_delta_a[tm_temp.tm_wday];
		tm = *localtime(&timestamp);

		snprintf(temp, DATE_MAX, "%d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

		if ((dates[i] = (char *) malloc(strlen(temp) + 1)) == NULL) {
			printf("\n\tError: Could not allocate memory for store dates.\n\n");
			exit(EXIT_FAILURE);
		}

		memset(dates[i], '\0', strlen(temp));
		memcpy(dates[i], temp, strlen(temp));
	}
}

// Function to get a UNIX Epoch timestamp from given date str of YYY-MM-DD
time_t get_timestamp_from_date_str(char *date_str) {
	int year = 0, month = 0, day = 0;
	struct tm time_info = {0};

	sscanf(date_str, "%d-%d-%d", &year, &month, &day);
	time_info.tm_year = year - 1900;
	time_info.tm_mon	= month - 1;
	time_info.tm_mday = day;
	time_info.tm_hour = 0;
	time_info.tm_min = 0;
	time_info.tm_sec = 0;
	time_info.tm_isdst = -1;

	return mktime(&time_info);
}

/*
	Function to test given issue_store_date against the 3 weekly store dates
	to determine "week" this given issue date falls under
*/
int test_issue_store_date(char **store_dates, char *issue_store_date) {
	int delta = THREE_DAY_SECONDS;
	time_t last_week_ts = get_timestamp_from_date_str(store_dates[0]);
	time_t this_week_ts = get_timestamp_from_date_str(store_dates[1]);
	time_t next_week_ts = get_timestamp_from_date_str(store_dates[2]);
	time_t issue_store_date_ts = get_timestamp_from_date_str(issue_store_date);

	if (last_week_ts == (issue_store_date_ts - delta) ||
		last_week_ts == (issue_store_date_ts + delta)) {
		sprintf(issue_store_date, "%s", store_dates[0]);
	} else if (this_week_ts == (issue_store_date_ts - delta) ||
				this_week_ts == (issue_store_date_ts + delta)) {
		sprintf(issue_store_date, "%s", store_dates[1]);
	} else if (next_week_ts == (issue_store_date_ts - delta) ||
				next_week_ts == (issue_store_date_ts + delta)) {
		sprintf(issue_store_date, "%s", store_dates[2]);
	} else {
		// This should never occur but just in case, show an error
		printf("Error: test_issue_store_date()\n");
		printf("last_week_ts: %d\n", (int) last_week_ts);
		printf("this_week_ts: %d\n", (int) this_week_ts);
		printf("next_week_ts: %d\n", (int) next_week_ts);
		printf("issue_store_date_ts: %d\n", (int) issue_store_date_ts);

		return 0;
	}

	return 1;
}

// Function to get date end range
void get_date_end_range(char *store_date, char *date_range_str, int flag) {
	time_t date_range_int;
	struct tm *time_info;
	int delta = THREE_DAY_SECONDS;

	// delta by itself gets us the Thu-Sat for "next week"
	// Making delta negative gets us the Sun-Tue for "last week"
	if (flag) {
		delta = -delta;
	}

	date_range_int = get_timestamp_from_date_str(store_date) + delta;
	time_info = localtime(&date_range_int);
    strftime(date_range_str, DATE_MAX, "%Y-%m-%d", time_info);
}
