#include <libconfig.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "sw_config.h"
#include "config.h"

int read_configuration (struct sw_config_struct *sw_config)
{
    config_t cfg;

    int http_port = HTTP_PORT;
    int https_port = HTTPS_PORT;
    const char *cert_path = CERT_PATH;
    const char *key_path = KEY_PATH;

    const char *queue_path = QUEUE_PATH;
    int queue_isenabled = QUEUE_ISENABLED;

    const char *smtp_url = SMTP_URL;
    const char *smtp_user = NULL;
    const char *smtp_passwd = NULL;

    // Initialize the configuration structure
    config_init(&cfg);

    // Read the configuration file
    if (config_read_file(&cfg, CONFIG_FILE) == CONFIG_TRUE) {
        // Get the values of the configuration variables
        config_lookup_int(&cfg, "http_port", &http_port);
        config_lookup_int(&cfg, "https_port", &https_port);
        config_lookup_string(&cfg, "cert_path", &cert_path);
        config_lookup_string(&cfg, "key_path", &key_path);
        config_lookup_string(&cfg, "queue_path", &queue_path);
        config_lookup_bool(&cfg, "queue_isenabled", &queue_isenabled);
        config_lookup_string(&cfg, "smtp_url", &smtp_url);
        config_lookup_string(&cfg, "smtp_user", &smtp_user);
        config_lookup_string(&cfg, "smtp_passwd", &smtp_passwd);
    } /* else {
        printf("%s not present, using defaults.\n", CONFIG_FILE);
        // Handle error
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        return(EXIT_FAILURE);
    }
    */

    sw_config->queue_path = queue_path;
    sw_config->queue_isenabled = queue_isenabled;
    sw_config->http_port = http_port;
    sw_config->https_port = https_port;
    sw_config->key_path = key_path;
    sw_config->cert_path = cert_path;
    sw_config->smtp_url = smtp_url;
    sw_config->smtp_user = smtp_user;
    sw_config->smtp_passwd = smtp_passwd;

    // Destroy the configuration structure
    config_destroy(&cfg);

    return 0;
}