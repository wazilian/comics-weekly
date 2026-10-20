
extern const int DATE_MAX;

extern const int PULL_WEEKS;

void get_store_dates(char **);

time_t get_timestamp_from_date_str(char *date_str);

int test_issue_store_date(char **store_dates, char *issue_store_date);

void get_date_end_range(char *store_date, char *date_range_str, int flag);
