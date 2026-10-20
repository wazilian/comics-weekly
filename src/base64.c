
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "base64.h"

const int AUTH_MAX = 128;	 // Combined length of given username and password

static char encoding_table[] = {
						'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
						'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
						'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
						'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
						'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
						'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
						'w', 'x', 'y', 'z', '0', '1', '2', '3',
						'4', '5', '6', '7', '8', '9', '+', '/'};
static int mod_table[] = { 0, 2, 1 };

// Function to convert provided string to a base64 signature
char *base_64_encode(char *data, size_t input_length, size_t *output_length) {
	*output_length = 4 * ((input_length + 2) / 3);

	char *encoded_data = malloc(*output_length + 1);
	if (encoded_data == NULL) return NULL;

	for (unsigned int i = 0, j = 0; i < input_length;) {
		u_int32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
		u_int32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
		u_int32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

		u_int32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
	}

	for (int i = 0; i < mod_table[input_length % 3]; i++) {
		encoded_data[*output_length - 1 - i] = '=';
	}

	encoded_data[*output_length] = '\0';

	return encoded_data;
}

// Function to create a base 64 string of provided username and password
char *get_base_64_creds(char *username, char *password) {
	size_t creds_len = 0;
	char creds[AUTH_MAX];

	memset(creds, '\0', AUTH_MAX);
	snprintf(creds, AUTH_MAX - 1, "%s:%s", username, password);
	creds_len = strlen(creds);

	return base_64_encode(creds, creds_len, &creds_len);
}
