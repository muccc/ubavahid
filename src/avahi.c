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
#include <avahi-common/alternative.h>
#include <avahi-common/address.h>
#include <avahi-core/core.h>
#include <avahi-core/publish.h>

#include <syslog.h>

#include "avahi.h"
#include "debug.h"

const AvahiPoll *poll_api;
AvahiGLibPoll *glib_poll;
GMainLoop *loop;
AvahiServer *server;
AvahiServerConfig serverconfig;

static GHashTable *services;
static GHashTable *addresses;

static void avahi_server_callback(AvahiServer *s,
                AvahiServerState state, void* userdata )
{
    GMainLoop *loop = userdata;
    syslog(LOG_INFO, "Avahi Server State Change: %d", state);

    if (state == AVAHI_SERVER_FAILURE)
    {
        syslog(LOG_INFO, "Error in server: %s",
                avahi_strerror(avahi_server_errno(s)));

        g_main_loop_quit (loop);
    }

}

static void service_address_callback(AvahiServer *s, AvahiSEntryGroup *group,
        AvahiEntryGroupState state, void *userdata) {
    /* Called whenever the entry group state changes */
    group = NULL;
    ub_assert(userdata);
    struct avahiaddress *n = userdata;
    gchar *name = n->hostname;

    switch (state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED :
            /* The entry group has been established successfully */
            syslog(LOG_INFO, "Address '%s' successfully established.\n", name);
            break;

        case AVAHI_ENTRY_GROUP_COLLISION : 
            /* An address collision with a remote address
             * happened.*/

            syslog(LOG_ERR, "Address collision for %s\n", name);
            syslog(LOG_ERR, "Is another ubavahid running in the local network?\n");
            g_main_loop_quit (loop);
        break;

        case AVAHI_ENTRY_GROUP_FAILURE :
            syslog(LOG_ERR, "Entry group failure: %s\n",
                avahi_strerror(avahi_server_errno(s)));

            g_main_loop_quit (loop);
            break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            ;
    }
}

static void service_service_callback(AvahiServer *s, AvahiSEntryGroup *group,
        AvahiEntryGroupState state, void *userdata) {
    s = NULL;
    group = NULL;
    /* Called whenever the entry group state changes */
    ub_assert(userdata);
    struct avahiservice *sd = userdata;
    gchar *name = sd->udpservicename;

    switch (state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED :
            /* The entry group has been established successfully */
            syslog(LOG_INFO, "Service '%s' successfully established.\n", name);
            break;

        case AVAHI_ENTRY_GROUP_COLLISION : {
            char *n;

            /* A service name collision with a remote service
             * happened. Let's pick a new name */
            n = avahi_alternative_service_name(name);
            avahi_free(name);
            name = n;

            syslog(LOG_ERR, "Service name collision,"
                                "renaming service to '%s'\n", name);
            while(1);
            /* And recreate the services */
            //create_services(avahi_entry_group_get_client(g));
            break;
        }

        case AVAHI_ENTRY_GROUP_FAILURE :
            syslog(LOG_ERR, "Entry group failure: %s\n",
                avahi_strerror(avahi_server_errno(server)));

            while(1);
            //g_main_loop_quit (loop);
            break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            ;
    }
}

void avahi_init(GMainLoop *mainloop)
{
    int error;

    loop = mainloop;
    avahi_set_allocator(avahi_glib_allocator());
    glib_poll = avahi_glib_poll_new(NULL, G_PRIORITY_DEFAULT);
    poll_api = avahi_glib_poll_get(glib_poll);
    avahi_server_config_init(&serverconfig);
    
    serverconfig.publish_addresses=0;
    
    server = avahi_server_new(poll_api,
            &serverconfig,
            avahi_server_callback,
            mainloop,
            &error); 

    if( server == NULL ){
        g_warning("Error initializing Avahi: %s", avahi_strerror(error));
        g_main_loop_quit(loop);
    }

    services = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    addresses = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}



void avahi_registerAddress(struct service *s)
{
    syslog(LOG_DEBUG,"avahi_registerAddress()");

    char *key = s->id;
    syslog(LOG_DEBUG, "address for node %s", key);
    struct avahiaddress *addr = g_hash_table_lookup(addresses, key); 
    if( addr != NULL )
        return;

    addr = g_new0(struct avahiaddress,1);
    addr->hostname = g_strdup_printf("%s.local", s->name);
    //addr->ip = g_strdup(s->ip);
    syslog(LOG_DEBUG, "mapping %s to %s", addr->hostname, s->ip);
    avahi_address_parse(s->ip, AVAHI_PROTO_UNSPEC , &(addr->avahiaddress));
    
    addr->entrygroup =
            avahi_s_entry_group_new(server, service_address_callback, addr);
    ub_assert(addr->entrygroup != NULL);

    int ret; 
    if( (ret = avahi_server_add_address(
                server,
                addr->entrygroup,
                AVAHI_IF_UNSPEC,
                AVAHI_PROTO_UNSPEC,
                0,
                addr->hostname,
                &(addr->avahiaddress) )) < 0 ){
        syslog(LOG_ERR, "Failed to add address: %s\n", avahi_strerror(ret));
    }

    ub_assert(addr->entrygroup != NULL);
    
    if( (ret = avahi_s_entry_group_commit(addr->entrygroup)) < 0 ){
        syslog(LOG_ERR, "Failed to commit entry group: %s\n",
            avahi_strerror(ret));
        g_main_loop_quit(loop);
    }
    g_hash_table_insert(addresses, g_strdup(key), addr);
}

void avahi_removeAddress(struct service *s)
{
    char *key = g_strdup_printf("%s%s", s->id, s->service_type);
    struct avahiservice *service = g_hash_table_lookup(services, key); 
    g_free(key);
    if( service != NULL ){
        return;
    }

    struct avahiaddress *addr = g_hash_table_lookup(addresses, s->id); 
    if( addr == NULL )
        return;

    syslog(LOG_INFO, "Removing address of: %s", s->id);
    ub_assert(addr->entrygroup != NULL);
    avahi_s_entry_group_reset(addr->entrygroup);
    g_free(addr->hostname);
    //g_free(addr->ip);
    g_hash_table_remove(addresses, s->id);
    
}

void avahi_registerService(struct service *s)
{
    syslog(LOG_DEBUG,"avahi_registerService()");
    
    char *key = g_strdup_printf("%s%s", s->id, s->service_type);
    syslog(LOG_DEBUG, "services for node %s", key);
    struct avahiservice *service = g_hash_table_lookup(services, key); 
    if( service != NULL ){
        g_free(key);
        return;
    }

    avahi_registerAddress(s);

    service = g_new0(struct avahiservice,1);
    service->name = g_strdup(s->name);
    service->hostname = g_strdup_printf("%s.local", s->name);
    service->port = s->port;
    service->entrygroup =
            avahi_s_entry_group_new(server, service_service_callback, service);
    ub_assert(service->entrygroup != NULL);

    int ret;
    service->tcpservicename = NULL;
    service->udpservicename = NULL;
    if( s->tcp ){
        service->tcpservicename = g_strdup_printf("_%s._tcp", s->service_type);
        if( (ret = avahi_server_add_service(
                    server,
                    service->entrygroup,
                    AVAHI_IF_UNSPEC,
                    AVAHI_PROTO_UNSPEC,
                    0,
                    service->name,
                    service->tcpservicename,
                    NULL,
                    service->hostname,
                    service->port,
                    NULL)) < 0 ){
            syslog(LOG_ERR, "Failed to add service: %s\n", avahi_strerror(ret));
        }
    }

    if( s->udp ){
        service->udpservicename = g_strdup_printf("_%s._udp", s->service_type);
        if( (ret = avahi_server_add_service(
                    server,
                    service->entrygroup,
                    AVAHI_IF_UNSPEC,
                    AVAHI_PROTO_UNSPEC,
                    0,
                    service->name,
                    service->udpservicename,
                    NULL,
                    service->hostname,
                    service->port,
                    NULL)) < 0 ){
            syslog(LOG_ERR, "Failed to add service: %s\n", avahi_strerror(ret));
        }
    }

    if( (ret = avahi_s_entry_group_commit(service->entrygroup)) < 0 ){
        syslog(LOG_ERR, "Failed to commit entry group: %s\n",
            avahi_strerror(ret));
        g_main_loop_quit(loop);
    }
    g_hash_table_insert(services, key, service);
}

void avahi_removeService(struct service *s)
{
    char *key = g_strdup_printf("%s%s", s->id, s->service_type);
    struct avahiservice *service = g_hash_table_lookup(services, key); 
    if( service == NULL ){
        g_free(key);
        return;
    }
    syslog(LOG_INFO, "Removing service: %s", key);
    ub_assert(service->entrygroup != NULL);
    avahi_s_entry_group_reset(service->entrygroup);
    g_free(service->name);
    g_free(service->hostname);
    if( service->tcpservicename )
        g_free(service->tcpservicename);
    if( service->udpservicename )
        g_free(service->udpservicename);
    g_hash_table_remove(services, key);
    g_free(key);

    avahi_removeAddress(s);
}

