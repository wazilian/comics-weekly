
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

const int SYS_INFO_MAX = 256;

// Function to get the current system info
char *get_system_info(void) {
	char *system_info;
	struct utsname uts;

	uname(&uts);

	if ((system_info = malloc(SYS_INFO_MAX)) == NULL) {
		fprintf(stderr, "Error: Memory allocation failed.\n");
		exit(EXIT_FAILURE);
	}

	memset(system_info, '\0', SYS_INFO_MAX);
	snprintf(system_info, SYS_INFO_MAX, "%s %s/%s", uts.sysname, uts.machine, uts.release);

	return system_info;
}
