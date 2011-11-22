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

int main (int argc, char *argv[])
{
    char *groupaddress = "ff18:583:786d:8ec9:d3d6:fd2b:1155:e066";
    char *interface = NULL;

    openlog("ubavahid",LOG_PID | LOG_PERROR ,LOG_DAEMON);
    segfault_init();

    if (!g_thread_supported ()) g_thread_init (NULL);
    g_type_init();
    GMainLoop * mainloop = g_main_loop_new(NULL,FALSE);
    
    if( argc < 2 ){
        syslog(LOG_ERR, "Please specify an interface to bind to.");
        return 1;
    }
    interface = argv[1];

    if( argc > 2 ){
        groupaddress = argv[2];
    }

    avahi_init(mainloop);
    ubavahid_init(interface, groupaddress);

    if( argc < 4 ){
        daemon_init();
        openlog("ubavahid", LOG_PID , LOG_DAEMON);
        daemon_close_stderror();
    }

    g_main_loop_run(mainloop);
    return 0;
}

