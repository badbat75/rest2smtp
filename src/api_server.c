#include <microhttpd.h>
#include <sys/stat.h>
#include <json-c/json.h>
#include <curl/curl.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "sw_config.h"
#include "smtp_client.h"
#include "queue.h"

char *load_file(const char *filename) {
    FILE *fp;
    long file_size;
    char *buffer;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        fclose(fp);
        return NULL;
    }

    fread(buffer, file_size, 1, fp);
    buffer[file_size] = '\0';

    fclose(fp);
    return buffer;
}

static enum MHD_Result answer_to_connection (void *cls, struct MHD_Connection *connection,
											 const char *url,
											 const char *method, const char *version,
											 const char *upload_data,
											 size_t *upload_data_size, void **con_cls)
{
	struct sw_config_struct *sw_config = cls; // retrieve the sw_config variable from the cls argument

	if (0 != strcmp (method, "POST"))
		return MHD_NO;

	struct json_smtp_struct *json_smtp = (struct json_smtp_struct *)(*con_cls);

	if (*con_cls == NULL)
	{
		json_smtp = malloc(sizeof(struct json_smtp_struct));
		if (json_smtp == NULL)
			return MHD_NO;

		memset(json_smtp->date, 0, sizeof(json_smtp->date));
		json_smtp->from = NULL;
		json_smtp->to = NULL;
		memset(json_smtp->message_id, 0, sizeof(json_smtp->message_id));
		json_smtp->subject = NULL;
		json_smtp->message = NULL;

		gen_message_id(json_smtp->message_id);

		*con_cls = (void *) json_smtp;

		return MHD_YES;
	}

	if (*upload_data_size != 0)
	{
		json_object *jobj = json_tokener_parse(upload_data);
		json_object_object_foreach(jobj, key, val) {
			if(strcmp(key, "from") == 0) {
				json_smtp->from = strdup(json_object_get_string(val));
			} else if(strcmp(key, "to") == 0) {
				json_smtp->to = strdup(json_object_get_string(val));
			} else if(strcmp(key, "subject") == 0) {
				json_smtp->subject = strdup(json_object_get_string(val));
			} else if(strcmp(key, "message") == 0) {
				json_smtp->message = strdup(json_object_get_string(val));
			}
		}
		json_object_put(jobj);

		*upload_data_size = 0;

		return MHD_YES;
	}
	else
	{
		int ret;
		struct MHD_Response *response;

		// Validate the input data
		if(json_smtp->from == NULL || json_smtp->to == NULL || json_smtp->subject == NULL || json_smtp->message == NULL) {
			response = MHD_create_response_from_buffer (strlen ("Invalid input data\n"), (void *) "Invalid input data\n", MHD_RESPMEM_PERSISTENT);
			ret = MHD_queue_response (connection, MHD_HTTP_BAD_REQUEST, response);
			MHD_destroy_response (response);
			return ret;
		}

		printf ("JSON received");

		/* Get the current date and time */
		time_t t = time(NULL);
		struct tm *tm = localtime(&t);
		strftime(json_smtp->date, sizeof(json_smtp->date), "%a, %d %b %Y %H:%M:%S %z", tm);

		if ( sw_config->queue_isenabled == 0 )
		{
			printf (", sending email \"%s\"... ", json_smtp->message_id);
			int res = smtp_client(sw_config, json_smtp);

			if(res != CURLE_OK) {
				const char *error_message = curl_easy_strerror(res);
				response = MHD_create_response_from_buffer (strlen (error_message), (void *) error_message, MHD_RESPMEM_PERSISTENT);
				ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
			} else {
				response = MHD_create_response_from_buffer (strlen ("Sent\n"), (void *) "Sent\n", MHD_RESPMEM_PERSISTENT);
				ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
			}
		} else {
			printf (", queuing email \"%s\"... ", json_smtp->message_id);
			int res = queue_add(sw_config, json_smtp);

			response = MHD_create_response_from_buffer (strlen ("Queued\n"), (void *) "Queued\n", MHD_RESPMEM_PERSISTENT);
			ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
			printf ("OK.\n");
		}

		MHD_destroy_response (response);

		return ret;
	}
}

struct server_daemon_struct start_api_server (struct sw_config_struct *sw_config)
{
 

	#if SYSTEMD_FOUND
		sd_journal_print(LOG_INFO, "\nQueue config:\n");
		sd_journal_print(LOG_INFO, "queue_path: %s\n", sw_config->queue_path);
		sd_journal_print(LOG_INFO, "queue_isenabled: %d\n", sw_config->queue_isenabled);
		sd_journal_print(LOG_INFO, "\nSMTP config:\n");
		sd_journal_print(LOG_INFO, "smtp_url: %s\n", sw_config->smtp_url);
		sd_journal_print(LOG_INFO, "smtp_user: %s\n", sw_config->smtp_user);
		sd_journal_print(LOG_INFO, "smtp_passwd: %s\n", sw_config->smtp_passwd);
	#else
		// Print the values of the variables
		printf("\nQueue config:\n");
		printf("queue_path: %s\n", sw_config->queue_path);
		printf("queue_isenabled: %d\n", sw_config->queue_isenabled);

		printf("\nSMTP config:\n");
		printf("smtp_url: %s\n", sw_config->smtp_url);
		printf("smtp_user: %s\n", sw_config->smtp_user);
		printf("smtp_passwd: %s\n", sw_config->smtp_passwd);
	#endif

	struct server_daemon_struct server_daemons;
	server_daemons.http = NULL;
	server_daemons.https = NULL;

	unsigned int flags = MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_DEBUG;

	printf("\nStarting HTTP Server on port %d...", sw_config->http_port);
	server_daemons.http = MHD_start_daemon (flags,
		sw_config->http_port,
		NULL, NULL,
		&answer_to_connection, sw_config,
		MHD_OPTION_NOTIFY_COMPLETED,
		NULL,
		NULL,
		MHD_OPTION_END);
	if ( server_daemons.http == NULL ) {
		printf ("failed.\n");
	} else {
		printf ("ok.\n");
	}

	// Verifica se i file cert_path e key_path esistono e sono accessibili
	struct stat buffer;
	int key_exists = (stat(sw_config->key_path, &buffer) == 0);
	int cert_exists = (stat(sw_config->cert_path, &buffer) == 0);
	if (cert_exists && key_exists) {
		flags |= MHD_USE_SSL;
		printf("Starting HTTPS Server on port %d...", sw_config->https_port);
		// Load the certificate and private key data from files
		char *cert_data = load_file(sw_config->cert_path);
		char *key_data = load_file(sw_config->key_path);
		// Starts the server daemon
		server_daemons.https = MHD_start_daemon (flags,
			sw_config->https_port,
			NULL, NULL,
			&answer_to_connection, sw_config,
			MHD_OPTION_HTTPS_MEM_CERT, cert_data,
			MHD_OPTION_HTTPS_MEM_KEY, key_data,
			MHD_OPTION_NOTIFY_COMPLETED,
			NULL,
			NULL,
			MHD_OPTION_END);
			if ( server_daemons.https == NULL ) {
				printf ("failed.\n");
			} else {
				printf ("ok.\n");
			}
	} else {
		printf("%s", "Key and certificate files are not present.\n");
	}

	return server_daemons;
}
