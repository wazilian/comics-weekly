
#include "mbedtls/build_info.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "string.h"
#include "TLS.h"

mbedtls_net_context server_fd;
mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_ssl_context ssl;
mbedtls_ssl_config conf;
mbedtls_x509_crt cacert;

enum { FALSE, TRUE };

const int HTTP_ERROR = FALSE;
const int HTTP_SUCCESS = TRUE;
const char HTTP_200[] = "200 OK";
const char HTTP_CONTENT_LENGTH[] = "ength: ";	// Removes need for case (Content-Length vs. content-length)
const char HTTP_HEADER_END[] = "\r\n\r\n";
const int BUFFER_SIZE = 1024;					// Can be adjusted as needed

// Function for mbedTLS config debug output
static void my_debug(void *ctx, int level, const char *file, int line, const char *str) {
	((void) level);

	mbedtls_fprintf((FILE *) ctx, "%s:%04d: %s", file, line, str);
	fflush((FILE *) ctx);
}

// Function to return this version of mbedTLS
char *get_TLS_version() {
	return MBEDTLS_VERSION_STRING_FULL;
}

// Function to initialize mbedTLS
void init_TLS() {
	int ret = 1;
	const char *pers = "wannaseesomemagic?";	// 100 points if you know the quote

	// Initialize the RNG and the session data
	mbedtls_net_init(&server_fd);
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_x509_crt_init(&cacert);
	mbedtls_ctr_drbg_init(&ctr_drbg);
	mbedtls_entropy_init(&entropy);

	if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg,
									mbedtls_entropy_func, &entropy,
									(const unsigned char *) pers,
									strlen(pers))) != 0) {
		mbedtls_printf(" failed\n! mbedtls_ctr_drbg_seed returned %d\n", ret);
		disconnect_TLS();
	}
}

// Function to make a TLS tunnel connection to the provided server over the given port
void connect_TLS(const char *server_name, const char *server_port) {
	int ret = 1;

	// Start the connection
	if ((ret = mbedtls_net_connect(&server_fd, server_name, server_port, MBEDTLS_NET_PROTO_TCP)) != 0) {
		mbedtls_printf(" failed\n! mbedtls_net_connect returned %d\n\n", ret);
		disconnect_TLS();
	}

	// More setup stuff
	if ((ret = mbedtls_ssl_config_defaults(&conf,
											MBEDTLS_SSL_IS_CLIENT,
											MBEDTLS_SSL_TRANSPORT_STREAM,
											MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
		mbedtls_printf(" failed\n! mbedtls_ssl_config_defaults returned %d\n\n", ret);
		disconnect_TLS();
	}

	// OPTIONAL is not optimal for security, but not worried about website certificate at this time
	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);

	// Set protocol to TLS 1.2, would like to see to 1.3 but we need to verify the host cert?!?
	mbedtls_ssl_conf_min_tls_version(&conf, MBEDTLS_SSL_VERSION_TLS1_2);

	if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
		mbedtls_printf(" failed\n! mbedtls_ssl_setup returned %d\n\n", ret);
		disconnect_TLS();
	}

	if ((ret = mbedtls_ssl_set_hostname(&ssl, server_name)) != 0) {
		mbedtls_printf(" failed\n! mbedtls_ssl_set_hostname returned %d\n\n", ret);
		disconnect_TLS();
	}

	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

	// TLS handshake
	while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_printf(" failed\n! mbedtls_ssl_handshake returned -0x%x\n\n", (unsigned int) -ret);
			disconnect_TLS();
		}
	}
}

// Function to capture the JSON packet from server using provided HTTP request
int get_json_packet(char *http_request, char **json_packet) {
	int ret = 1;
	int len = 0;
	unsigned char buffer[BUFFER_SIZE];
	char *temp_ptr;
	int http_ok = FALSE;		// Boolean, received HTTP 200 OK response
	int header = FALSE;			// Boolean, reached end of the HTTP header
	int content_length = 0;		// Captured from HTTP header
	int bytes_read = 0;			// Bytes read into json_packet

	// Write the GET request
	memset(buffer, 0, BUFFER_SIZE);
	len = snprintf((char *) buffer, BUFFER_SIZE, http_request);
	while ((ret = mbedtls_ssl_write(&ssl, buffer, len)) <= 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_printf(" failed\n! mbedtls_ssl_write returned %d\n\n", ret);
			return HTTP_ERROR;
		}
	}

	// Read the HTTP response
	do {
		memset(buffer, 0, BUFFER_SIZE);
		ret = mbedtls_ssl_read(&ssl, buffer, BUFFER_SIZE - 1);

		// Server was expecting a read or write and got neither
		if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_printf("MBEDTLS_ERR_SSL_WANT_READ/MBEDTLS_ERR_SSL_WANT_WRITE\n\n");
			continue;
		}

		// Connection was closed by the server
		if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			mbedtls_printf("MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY\n\n");
			break;
		}

		// Some other mbedTLS error occured
		if (ret < 0) {
			mbedtls_printf("failed\n! mbedtls_ssl_read returned %d\n\n", ret);
			break;
		}

		// No more data to be read
		if (ret == 0) {
			mbedtls_printf("\n\nEOF\n\n");
			break;
		}

		// Should have a valid HTTP header
		if (ret > 0) {
			/***** NOTICE THE ORDER OF THESE 'IF' STATEMENTS *****/

			// Check HTTP header for HTTP error (not '200 OK')
			if (!header && !http_ok && (strstr((const char *) buffer, HTTP_200) == NULL)) {
				// Print some info to a log file?
				printf("\n\tError: '%s' not found.\n\n\tCurrent read buffer:\n\n'%s'\n\n", HTTP_200, buffer);
				break;
			}

			// Find HTTP 200 OK and then find content length
			// For now, the assumption is, Content-Length is near top of header
			if (!header && !http_ok && (strstr((const char *) buffer, HTTP_200) != NULL)) {
				// Find content length in the HTTP header
				temp_ptr = strstr((const char *) buffer, HTTP_CONTENT_LENGTH);

				// Why would you not put Content-Length in the HTTP response?!?
				if (temp_ptr == NULL) {
					return HTTP_ERROR;
				}

				temp_ptr += strlen(HTTP_CONTENT_LENGTH);	// Move pointer past content length string
				sscanf(temp_ptr, "%d", &content_length);	// Capture content length

				// Allocate memory for the size (content_length) of the JSON packet
				if ((*json_packet = (char *) calloc(content_length + 1, sizeof(char))) == NULL) {
					printf("\n\tError: Could not allocate memory for JSON packet. Size required: %d.\n\n", content_length);
					break;
				}

				// Found HTTP 200 OK, update flag
				http_ok = TRUE;
			}

			// HTTP 200 found, parse and move past the HTTP header
			if (!header && http_ok) {
				// Find end of HTTP header
				if ((temp_ptr = strstr((const char *) buffer, HTTP_HEADER_END)) == NULL) {
					continue;
				}

				// Move pointer past HTTP header
				temp_ptr += strlen(HTTP_HEADER_END);

				// Append any data at the end of current read buffer to json_packet
				strcat(*json_packet, temp_ptr);
				bytes_read += strlen(temp_ptr);

				// Have we read 'content_length' into 'json_packet'
				if (bytes_read == content_length) {
					return HTTP_SUCCESS;
				}

				// Finished parsing the header, update flag
				header = TRUE;

				// Continue so we can read more data into json_packet
				continue;
			}

			// Finished parsing the header, continue to capture the content
			if (header && http_ok) {
				// Append the read buffer to the json_packet
				strcat(*json_packet, (const char *) buffer);
				bytes_read += strlen((const char *) buffer);

				// Have we read 'content_length' into 'json_packet'
				if (bytes_read == content_length) {
					return HTTP_SUCCESS;
				}
			}
		}
	} while (1);

	return HTTP_ERROR;
}

// Disconnect TLS connection and cleanup
void disconnect_TLS() {
	mbedtls_ssl_close_notify(&ssl);
	mbedtls_net_free(&server_fd);
	mbedtls_x509_crt_free(&cacert);
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
	mbedtls_exit(MBEDTLS_EXIT_SUCCESS);
}
