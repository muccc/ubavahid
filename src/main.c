#include <config.h>
#include <stdio.h>
#include <glib.h>
#include <fcntl.h>
#include <termios.h>
#include <stdint.h>
#include <string.h>
#include <gio/gio.h>
#include <syslog.h>

#include "debug.h"
#include "avahi.h"
#include "daemon.h"
#include "segfault.h"
#include "ubavahid.h"

struct service s;

static gboolean adds(gpointer userdata)
{
    //avahi_registerService(&s);
    return FALSE;
}

static gboolean removes(gpointer userdata)
{
    //avahi_removeService(&s);
    return FALSE;
}


int main (int argc, char *argv[])
{
    argv = NULL;
    openlog("ubavahid",LOG_PID | LOG_PERROR ,LOG_DAEMON);
    segfault_init();

    if (!g_thread_supported ()) g_thread_init (NULL);
    g_type_init();
    GMainLoop * mainloop = g_main_loop_new(NULL,FALSE);
    
    avahi_init(mainloop);
    ubavahid_init();

    if( argc < 2 ){
        daemon_init();
        openlog("ubavahid", LOG_PID , LOG_DAEMON);
        daemon_close_stderror();
    }
    s.id = "moodlamp,xort.eu";
    s.name = "moodlamp23";
    s.service_type = "moodlamp";
    s.ip = "::1";
    s.port = 2323;
    s.udp = TRUE;
    s.tcp = TRUE;
    g_timeout_add_seconds(5,adds,NULL);
    g_timeout_add_seconds(10,adds,NULL);
    g_timeout_add_seconds(15,removes,NULL);
    g_timeout_add_seconds(20,adds,NULL);
    g_timeout_add_seconds(30,removes,NULL);
    g_timeout_add_seconds(35,removes,NULL);
    g_main_loop_run(mainloop);
    return 0;
}

