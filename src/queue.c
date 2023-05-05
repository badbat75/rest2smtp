#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <json-c/json.h>

#include "sw_config.h"
#include "smtp_client.h"

int queue_add (struct sw_config_struct *sw_config, struct json_smtp_struct *json_smtp)
{
	FILE *fptr;
	char directory[128];
	sprintf(directory, "%s/outbox", sw_config->queue_path);
	char filename[256];
	sprintf(filename, "%s/%s.msg", directory, json_smtp->message_id);
	// Create a file
	fptr = fopen(filename, "w");

	// Create a JSON object
	json_object *jobj = json_object_new_object();
	
	// Add data to the JSON object
	json_object_object_add(jobj, "date", json_object_new_string(json_smtp->date));
	json_object_object_add(jobj, "from", json_object_new_string(json_smtp->from));
	json_object_object_add(jobj, "to", json_object_new_string(json_smtp->to));
	json_object_object_add(jobj, "message_id", json_object_new_string(json_smtp->message_id));
	json_object_object_add(jobj, "subject", json_object_new_string(json_smtp->subject));
	json_object_object_add(jobj, "message", json_object_new_string(json_smtp->message));
	
	// Write the JSON object to the file
	fprintf(fptr, "%s\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY));
	
	// Free the JSON object
	json_object_put(jobj);
	
	// Close the file
	fclose(fptr);

	return 0;
};

int process_file_content(FILE *file, struct json_smtp_struct *json_smtp) {

	if (json_smtp == NULL) {
		// Handle the error
		fprintf(stderr, "json_smtp is NULL\n");
		return -1;
	}

	// Read the entire file into a buffer
	fseek(file, 0, SEEK_END);
	long fsize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char *buffer = malloc(fsize + 1);
	fread(buffer, 1, fsize, file);
	buffer[fsize] = '\0';

	// Parse the JSON data
	json_object *jobj = json_tokener_parse(buffer);

	// Extract the data from the JSON object - method 1
	json_object_object_foreach(jobj, key, val) {
		if (strcmp(key, "date") == 0) {
			strcpy(json_smtp->date, json_object_get_string(val));
		} else if (strcmp(key, "from") == 0) {
			json_smtp->from = strdup(json_object_get_string(val));
		} else if (strcmp(key, "to") == 0) {
			json_smtp->to = strdup(json_object_get_string(val));
		} else if (strcmp(key, "message_id") == 0) {
			strcpy(json_smtp->message_id, json_object_get_string(val));
		} else if (strcmp(key, "subject") == 0) {
			json_smtp->subject = strdup(json_object_get_string(val));
		} else if (strcmp(key, "message") == 0) {
			json_smtp->message = strdup(json_object_get_string(val));
		}
	}

	/*
	// Extract the data from the JSON object - method 2
	json_object_object_get_ex(jobj, "date", &jobj);
	strcpy(json_smtp->date, json_object_get_string(jobj));
	json_object_object_get_ex(jobj, "from", &jobj);
	json_smtp->from = strdup(json_object_get_string(jobj));
	json_object_object_get_ex(jobj, "to", &jobj);
	json_smtp->to = strdup(json_object_get_string(jobj));
	json_object_object_get_ex(jobj, "message_id", &jobj);
	strcpy(json_smtp->message_id, json_object_get_string(jobj));
	json_object_object_get_ex(jobj, "subject", &jobj);
	json_smtp->subject = strdup(json_object_get_string(jobj));
	json_object_object_get_ex(jobj, "message", &jobj);
	json_smtp->message = strdup(json_object_get_string(jobj));
	*/

	// Free the JSON object and buffer
	json_object_put(jobj);
	free(buffer);

	return 0;
}

void *queue_poll(void *arg) {
	struct sw_config_struct *sw_config = (struct sw_config_struct *)arg;
	char outbox_dir[512];
	char sent_dir[512];
	sprintf(outbox_dir, "%s/outbox", sw_config->queue_path);
	sprintf(sent_dir, "%s/sent", sw_config->queue_path);

	int interval = 1;
	while (1) {
		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir(outbox_dir)) != NULL) {
			while ((ent = readdir(dir)) != NULL) {
				if (ent->d_type == DT_REG) {
					char file_path[556];
					sprintf(file_path, "%s/%s", outbox_dir, ent->d_name);
					FILE *file = fopen(file_path, "r");
					if (file != NULL) {
						struct json_smtp_struct json_smtp;
						int ret = process_file_content(file, &json_smtp);
						fclose(file);
						if ( ret != 0 ) {
							printf ("Error while processing %s.\n", ent->d_name);
						} else {
							printf ("Sending email \"%s\"... ", json_smtp.message_id);
							ret = smtp_client (sw_config, &json_smtp);
							if ( ret != 0 ) {
								printf ("Error while sending email for %s.\n", file_path);
							} else {
								char new_path[556]; // 512 + 1 + 37 + 4
								sprintf(new_path, "%s/%s", sent_dir, ent->d_name);
								rename(file_path, new_path);
							}
						}
					}
					else
					{
						printf ("I can't read %s.\n", ent->d_name);
						return (void *)1;
					}
				}
			}
			closedir(dir);
		} else {
			printf ("Could not open directory %s.\n", outbox_dir);
			return (void *)2;
		}
		sleep(interval);
	}
	return NULL;
}