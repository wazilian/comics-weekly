
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <dirent.h>

#include "global.h"
#include "base64.h"
#include "TLS.h"
#include "cJSON.h"

const char VERSION[]  = "1.0";
const char SERVER_NAME[] = "metron.cloud";
const char SERVER_PORT[] = "443";
const char OUTPUT_FILE_EXT[] = ".txt";
const int AUTH_MAX = 128;           /* combined length of given username and password */
const int HTTP_REQUEST_MAX = 512;
const int SYS_INFO_MAX = 256;
const int PULL_WEEKS = 3;

/* HTTP_REQUEST_TEMPLATE string template
  string parameter:

  1) Publisher, Store Date After, and Store Date Before (e.g. 2023-10-17)
  2) Server name and port
  3) User-agent string
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
  char auth[AUTH_MAX];
  size_t authLen = 0;
  char *base64UnP = NULL;
  char *output_dir;
  struct dirent *file_ptr;
  DIR *dir;
  char *storeDates[PULL_WEEKS];
  struct utsname uts;
  char systemInfo[SYS_INFO_MAX];
  int i;
  char http_request[512];
  char *json_packet;
  cJSON *json_ob, *results;
  FILE *fp_last_week = NULL;
  FILE *fp_this_week = NULL;
  FILE *fp_next_week = NULL;

  /* check for proper command line arguments */
  if (argc < 5) {
    printCmdArgs(APP_NAME);   /* APP_NAME is defined at the Makefile */
    return -1;
  }

  /* get API username and password from command line & convert to base64 */
  memset(auth, 0, AUTH_MAX);
  snprintf(auth, AUTH_MAX - 1, "%s:%s", argv[1], argv[2]);
  authLen = strlen(auth);
  base64UnP = base64Encode(auth, authLen, &authLen);

  /* get text directory provided on command line */
  output_dir = argv[3];

  /* remove all <store date>.OUTPUT_FILE_EXT files from given directory */
  if ((dir = opendir(output_dir)) == NULL) {
    printf("\n\tError: %s could not be opened. Permissions?\n\n", output_dir);
    return -1;
  } else {
    while ((file_ptr = readdir(dir)) != NULL) {
      /* only remove file if its extension is OUTPUT_FILE_EXT */
      if (strcmp(strrchr(file_ptr->d_name, '\0') - strlen(OUTPUT_FILE_EXT), OUTPUT_FILE_EXT) == 0) {
        char temp[512];
        snprintf(temp, 512, "%s/%s", output_dir, file_ptr->d_name);

        if (remove(temp) != 0) {
          printf("\n\tError: %s could not be deleted. Permissions?\n\n", temp);
          return -1;
        }
      }
    }

    /* close directory */
    closedir(dir);
  }

  /* get the store dates for Wednesday of the previous, current, and next weeks */
  getStoreDates(storeDates);

  /* get platform/system information */
  memset(systemInfo, 0, SYS_INFO_MAX);
  uname(&uts);
  snprintf(systemInfo, SYS_INFO_MAX, "%s %s/%s", uts.sysname, uts.machine, uts.release);

  /* open the 3 output text files for the last, current, and next weeks */
  if (!open_output_files(&fp_last_week, &fp_this_week, &fp_next_week, output_dir, storeDates)) {
    return -1;
  }

  /* initialize the TLS connection */
  init_TLS();

  /* start the TLS connection */
  connect_TLS(SERVER_NAME, SERVER_PORT);

  /* loop through the publishers list and obtain JSON packet for each */
  for (i = 4; i < argc; i++) {
    /* build out the HTTP request string for store date */
    memset(http_request, 0, HTTP_REQUEST_MAX);
    snprintf(http_request, HTTP_REQUEST_MAX, HTTP_REQUEST_TEMPLATE,
            argv[i], storeDates[0], storeDates[2],
            SERVER_NAME, SERVER_PORT,
            APP_NAME, VERSION, systemInfo, get_TLS_version(),
            base64UnP);

    /* read the HTTP response & extract the JSON packet */
    if (!get_json_packet(http_request, &json_packet)) {
      printf("\n\tError occured using this HTTP request:\n\n'%s'\n\n", http_request);
      disconnect_TLS();
      return 1;
    }

    /* parse JSON object */
    json_ob = cJSON_Parse(json_packet);
    results = cJSON_GetObjectItem(json_ob, "results");

    if (results) {
      cJSON *issue = results->child;

      /* loop over each result grabbing the issue name & store date */
      while (issue) {
        cJSON *title = cJSON_GetObjectItemCaseSensitive(issue, "issue");
        cJSON *store_date = cJSON_GetObjectItemCaseSensitive(issue, "store_date");

        /* write this issue to the appropriate file */
        if (strcmp(storeDates[0], store_date->valuestring) == 0) {
          fprintf(fp_last_week, "%s\n", title->valuestring);
        } else if (strcmp(storeDates[1], store_date->valuestring) == 0) {
          fprintf(fp_this_week, "%s\n", title->valuestring);
        } else if (strcmp(storeDates[2], store_date->valuestring) == 0) {
          fprintf(fp_next_week, "%s\n", title->valuestring);
        } else {
          printf("Error (%s) -> %s\n", store_date->valuestring, title->valuestring);
        }

        /* move to next issue in the results */
        issue = issue->next;
      }
    }

    /* free that memory */
    free(json_packet);
    json_packet = NULL;
    cJSON_Delete(json_ob);
  }

  /* disconnect TLS */
  disconnect_TLS();

  /* close the output text files */
  fclose(fp_last_week);
  fclose(fp_this_week);
  fclose(fp_next_week);

  return 0;
}
