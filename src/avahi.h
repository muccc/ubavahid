#ifndef _AVAHI_H_
#define _AVAHI_H_
#include <config.h>
#include <stdio.h>
#include <glib.h>
#include <stdint.h>
#include <string.h>
#include <gio/gio.h>

#include <avahi-common/error.h>
#include <avahi-common/timeval.h>
#include "avahi-glib/glib-watch.h"
#include "avahi-glib/glib-malloc.h"
#include <avahi-core/publish.h>
struct multicastgroup;

struct service{
    char *id;
    char *name;
    char *service_type;
    char *ip;
    int port;
    gboolean udp;
    gboolean tcp;
    int ttl;
};

struct avahiservice{
    AvahiSEntryGroup *entrygroup;
    char *name;
    char *hostname;
    char *tcpservicename;
    char *udpservicename;
    int port;
};

struct avahiaddress{
    AvahiSEntryGroup *entrygroup;
    AvahiAddress avahiaddress;
    char *hostname;
};

void avahi_init(GMainLoop *mainloop);
void avahi_registerService(struct service *s);
void avahi_removeService(struct service *s);
void avahi_registerAddress(struct service *s);
void avahi_removeAddress(struct service *s);
#endif
