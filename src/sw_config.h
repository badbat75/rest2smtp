#if SYSTEMD_FOUND
#include <systemd/sd-journal.h>
#endif

struct sw_config_struct {
    /* Queue config */
    const char *queue_path;
    int queue_isenabled;
    /* HTTP config */
    int http_port;
    int https_port;
    const char *key_path;
    const char *cert_path;
    /*# SMTP Config */
    const char *smtp_url;
    const char *smtp_user;
    const char *smtp_passwd;   
};

struct server_daemon_struct {
    struct MHD_Daemon *http;
    struct MHD_Daemon *https;
    // Queue daemons could be here
};

struct json_smtp_struct {
    char date[64];
    char *from;
    char *to;
    char message_id[37];
    char *subject;
    char *message;
};

int read_configuration (struct sw_config_struct *sw_config);