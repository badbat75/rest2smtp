#include <curl/curl.h>
#include <uuid/uuid.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>

#include "sw_config.h"

void gen_message_id(char *buffer)
{
	uuid_t uuid;

	uuid_generate_random(uuid);
	uuid_unparse(uuid, buffer);
}

int smtp_client (struct sw_config_struct *sw_config, struct json_smtp_struct *json_smtp)
{

	printf("from \"%s\" to \"%s\"... ", json_smtp->from, json_smtp->to);

	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	curl_mime *mime;
	curl_mimepart *part;

	/* Create the email headers */
	char headers[1024];
	snprintf(headers, sizeof(headers),
			 "Date: %s\r\n"
			 "To: %s\r\n"
			 "From: %s\r\n"
			 "Message-ID: %s\r\n"
			 "Subject: %s\r\n",
			 json_smtp->date, json_smtp->to, json_smtp->from, json_smtp->message_id, json_smtp->subject);

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, sw_config->smtp_url);

		if (sw_config->smtp_user != NULL && sw_config->smtp_passwd != NULL) {
			curl_easy_setopt(curl, CURLOPT_USERNAME, sw_config->smtp_user);
			curl_easy_setopt(curl, CURLOPT_PASSWORD, sw_config->smtp_passwd);
		}

		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_TRY);
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, json_smtp->from);

		recipients = curl_slist_append(recipients, json_smtp->to);
		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

		mime = curl_mime_init(curl);

		part = curl_mime_addpart(mime);
		curl_mime_data(part, headers, CURL_ZERO_TERMINATED);

		part = curl_mime_addpart(mime);
		curl_mime_data(part, "\r\n", CURL_ZERO_TERMINATED);

		part = curl_mime_addpart(mime);
		curl_mime_data(part, json_smtp->message, CURL_ZERO_TERMINATED);

		curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

		res = curl_easy_perform(curl);

		if(res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			printf ("failed.\n");
		} else {
			printf ("ok\n");
		}
		curl_slist_free_all(recipients);
		curl_easy_cleanup(curl);
		curl_mime_free(mime);
	}
	return (int)res;
}
