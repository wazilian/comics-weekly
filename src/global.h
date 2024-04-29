
/* check global.c for function comments */

/* linked list typedef structure */
typedef struct {
  void *next;
  char *data;
} Publisher;

/* head of the Publisher linked list */
extern Publisher *headPublisher;

void getStoreDates(char **);

void addPublisher(char *);

int open_output_files(FILE **, FILE **, FILE **, char *, char **);

void printCmdArgs(const char *);
