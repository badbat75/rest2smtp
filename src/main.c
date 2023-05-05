#include <microhttpd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#include "sw_config.h"
#include "api_server.h"
#include "queue.h"

#define VERSION "0.9"

int main ()
{
	printf ("rest2smtp version %s", VERSION);
	#if SYSTEMD_FOUND
		printf (" +systemd");
	#endif
	printf ("\n");

	struct sw_config_struct sw_config;
	int result = read_configuration (&sw_config);
	if (result != 0) {
		return EXIT_FAILURE;
	}

	/* Creates outbox and sent directories if they don't exist */
	char outbox_dir[512];
	char sent_dir[512];
	sprintf(outbox_dir, "%s/outbox", sw_config.queue_path);
	sprintf(sent_dir, "%s/sent", sw_config.queue_path);
	struct stat st = {0};
	if (stat(outbox_dir, &st) == -1) {
		mkdir(outbox_dir, 0755);
	}
	if (stat(sent_dir, &st) == -1) {
		mkdir(sent_dir, 0755);
	}

	/* Run the API Server */
	struct server_daemon_struct server_daemons = start_api_server (&sw_config);
	if (server_daemons.http == NULL) {
		printf("HTTP server not started.\n");
	}

	if (server_daemons.https == NULL) {
		printf("HTTPS server not started\n");
	}

	/* Run the Queue daemon */
	pthread_t queue_server;
	int queue_status = pthread_create(&queue_server, NULL, queue_poll, &sw_config);

	printf("\nPress ENTER to exit...\n");
	getchar ();

	if (server_daemons.http != NULL) {
		MHD_stop_daemon (server_daemons.http);
	}

	if (server_daemons.https != NULL) {
		MHD_stop_daemon (server_daemons.https);
	}

	pthread_cancel(queue_server);


}
