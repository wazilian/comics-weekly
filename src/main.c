
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dates.h"
#include "dir.h"
#include "system_info.h"
#include "base64.h"
#include "TLS.h"
#include "cJSON.h"

const char SERVER_NAME[] = "metron.cloud";
const char SERVER_PORT[] = "443";

/*
	HTTP_REQUEST_TEMPLATE string template

	string parameter:
		1) Publisher, Store Date After, and Store Date Before (e.g. 2023-10-17)
		2) Server name and port
		3) User-agent string (APP_NAME/VERSION (both from Makefile) (system_info) tls_version)
		4) Username and password in base64 (string)
*/
char HTTP_REQUEST_TEMPLATE[] =
	"GET /api/issue/?publisher_name=%s&store_date_range_after=%s&store_date_range_before=%s "
	"HTTP/1.1\r\n"
	"Host: %s:%s\r\n"
	"Connection: Keep-Alive\r\n"
	"Accept: application/json\r\n"
	"User-Agent: %s/%s (%s) %s\r\n"
	"Authorization: Basic %s\r\n\r\n";

int main(int argc, char *argv[]) {
	char *output_dir;
	char *store_dates[PULL_WEEKS];
	char date_range_after_str[DATE_MAX];
	char date_range_before_str[DATE_MAX];
	char *system_info;
	char *tls_version;
	char *base_64_creds;
	int i;
	FILE *fp_last_week = NULL;
	FILE *fp_this_week = NULL;
	FILE *fp_next_week = NULL;

	// Check for proper command line arguments
	if (argc < 5) {
		printf("\n\tError: Incorrect parameter list.\n");
		printf("\n\t# ./%s <Metron.cloud usernme> <Metron.cloud password> ", APP_NAME);
		printf("<output text files directory> <space separated list of comic publishers>\n\n");
		return -1;
	}

	// Get API username and password from command line & convert to base64
	base_64_creds = get_base_64_creds(argv[1], argv[2]);

	// Get output file directory from command line
	output_dir = argv[3];

	// Clear output file directory
	clear_output_dir(output_dir);

	// Get the store dates for Wednesday of the last, current, and next weeks
	get_store_dates(store_dates);

	// Get platform/system information
	system_info = get_system_info();

	// Open the 3 output text files for the last, current, and next weeks
	open_output_files(&fp_last_week, &fp_this_week, &fp_next_week, output_dir, store_dates);

	// Get the date end ranges for this query
	get_date_end_range(store_dates[0], date_range_after_str, 1);
	get_date_end_range(store_dates[2], date_range_before_str, 0);

	// Initialize the TLS connection
	init_TLS();

	// Start the TLS connection
	connect_TLS(SERVER_NAME, SERVER_PORT);

	// Get TLS version
	tls_version = get_TLS_version();

	// Loop through the publishers list and obtain JSON packet for each
	for (i = 4; i < argc; i++) {
		char http_request[512];
		char *json_packet;
		cJSON *json_ob, *results;

		// Build out the HTTP request string for store date
		memset(http_request, '\0', 512);
		snprintf(http_request, 512, HTTP_REQUEST_TEMPLATE,
				argv[i], date_range_after_str, date_range_before_str,
				SERVER_NAME, SERVER_PORT,
				APP_NAME, VERSION, system_info, tls_version,
				base_64_creds);

		// Read the HTTP response & extract the JSON packet
		if (!get_json_packet(http_request, &json_packet)) {
			printf("\n\tError occured using this HTTP request:\n\n'%s'\n\n", http_request);
			disconnect_TLS();
			return 1;
		}

		// Parse JSON object
		json_ob = cJSON_Parse(json_packet);
		results = cJSON_GetObjectItem(json_ob, "results");

		if (results) {
			cJSON *issue = results->child;

			// Loop over each result grabbing the issue name & store date
			while (issue) {
				cJSON *issue_title = cJSON_GetObjectItemCaseSensitive(issue, "issue");
				cJSON *issue_store_date = cJSON_GetObjectItemCaseSensitive(issue, "store_date");
				int flag = 0;

				// Write this issue to the appropriate file
				do {
					flag = 1;

					if (strcmp(store_dates[0], issue_store_date->valuestring) == 0) {
						fprintf(fp_last_week, "%s\n", issue_title->valuestring);
					} else if (strcmp(store_dates[1], issue_store_date->valuestring) == 0) {
						fprintf(fp_this_week, "%s\n", issue_title->valuestring);
					} else if (strcmp(store_dates[2], issue_store_date->valuestring) == 0) {
						fprintf(fp_next_week, "%s\n", issue_title->valuestring);
					} else {
						// This issue is released on a non-Wednesday and is rare to occur or
						// the date is wrong in the API (more rare)
						flag = 0;

						if (!test_issue_store_date(store_dates, issue_store_date->valuestring)) {
							printf("Issue error: (%s) - %s\n",
									issue_store_date->valuestring, issue_title->valuestring);
							break;
						}
					}
				} while (flag == 0);

				// Move to next issue in the results
				issue = issue->next;
			}
		}

		// Free that memory
		free(json_packet);
		json_packet = NULL;
		cJSON_Delete(json_ob);
	}

	// Disconnect TLS
	disconnect_TLS();

	// Close the output text files
	fclose(fp_last_week);
	fclose(fp_this_week);
	fclose(fp_next_week);

	return 0;
}
