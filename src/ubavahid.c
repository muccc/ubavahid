#include "ubavahid.h"
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <gio/gio.h>
#include <syslog.h>
#include <json/json.h>

#include "debug.h"
#include "net_multicast.h"

static GSocketAddress *sa;
static GSocket *ubavahidsocket;
static gboolean ubavahid_read(GSocket *socket, GIOCondition condition,
                                        gpointer user_data);

static void ubavahid_updateServiceCmd(struct json_object *json);
static void ubavahid_deleteServiceCmd(struct json_object *json);
static void ubavahid_deleteService(const char *id);

static enum commandlist ubavahid_parseCommand(const char *cmd);
static gboolean ubavahid_tick(gpointer data);
enum commandlist {
NO_COMMAND, UPDATE_SERVICE, DELETE_SERVICE
};

struct command {
    char *command;
    enum commandlist id;
};

struct command commands[] = 
{
{"update-service", UPDATE_SERVICE},
{"delete-service", DELETE_SERVICE}
};

#define COMMAND_COUNT (sizeof(commands)/sizeof(struct command))

static GHashTable *services;

void ubavahid_init(void)
{
    syslog(LOG_DEBUG,"ubavahid_init: starting listener");
    services = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    ubavahidsocket = multicast_createSocket("directoryserver", 2323, &sa);
    if( socket != NULL){
        syslog(LOG_DEBUG,"ubavahid_init: socket open");
        GSource *source = g_socket_create_source(ubavahidsocket,
                            G_IO_IN, NULL);
        ub_assert(source != NULL);
        g_source_set_callback(source, (GSourceFunc)ubavahid_read,
                                NULL, NULL);
        g_source_attach(source, g_main_context_default());

    }else{
        syslog(LOG_WARNING,
                "ubavahid.c: warning: could not create socket");
    }
    g_timeout_add_seconds(1,ubavahid_tick,NULL);
}

static const char *ubavahid_getJsonString(struct json_object *json,
                                    const char *field, const char *dflt)
{
    struct json_object *json_tmp;
    json_tmp = json_object_object_get(json, field);
    if( json_tmp != NULL &&
        !is_error(json_tmp) &&
        json_object_get_type(json_tmp) == json_type_string ){
        return json_object_get_string(json_tmp);
    }
    return dflt;
}

static gboolean ubavahid_getJsonBool(struct json_object *json,
                                    const char *field, const gboolean dflt)
{
    struct json_object *json_tmp;
    json_tmp = json_object_object_get(json, field);
    if( json_tmp != NULL &&
        !is_error(json_tmp) &&
        json_object_get_type(json_tmp) == json_type_boolean ){
        return json_object_get_boolean(json_tmp)?TRUE:FALSE;
    }
    return dflt;
}

static int32_t ubavahid_getJsonInt(struct json_object *json,
                                    const char *field, const int32_t dflt)
{
    struct json_object *json_tmp;
    json_tmp = json_object_object_get(json, field);
    if( json_tmp != NULL &&
        !is_error(json_tmp) &&
        json_object_get_type(json_tmp) == json_type_int ){
        return json_object_get_int(json_tmp);
    }
    return dflt;
}

//Make sure data is sanatized
static void ubavahid_newMCData(const char *data)
{
    struct json_object *json_tmp;
    struct json_object *json = json_tokener_parse(data);

    ub_assert(json != NULL);
    if( is_error(json) ){
        syslog(LOG_DEBUG,"ubavahid_newMCData: invalid json");
        return;
    }
    json_tmp = json_object_object_get(json, "cmd");
    if( json_tmp == NULL || is_error(json_tmp) ){
        syslog(LOG_DEBUG,"ubavahid_newMCData: no cmd field in json");
        return;
    }
    if( json_object_get_type(json_tmp) != json_type_string ){
        syslog(LOG_DEBUG,"ubavahid_newMCData: no cmd field is not a string");
        return;
    }

    enum commandlist cmd = ubavahid_parseCommand(
            json_object_get_string(json_tmp));
    json_object_object_del(json, "cmd");

    switch( cmd ){
        case UPDATE_SERVICE:
            ubavahid_updateServiceCmd(json);
            break;
        case DELETE_SERVICE:
            ubavahid_deleteServiceCmd(json);
        case NO_COMMAND:
            break;
    };

    json_object_put(json);
}

static void ubavahid_deleteService(const char *key)
{
    struct service *service = g_hash_table_lookup(services, key);
    if( service == NULL ){
        syslog(LOG_DEBUG,"ubavahid_deleteService: id unknown");
        return;
    }
    syslog(LOG_DEBUG,"ubavahid_deleteService: deleting %s", key);
    avahi_removeService(service);
    g_free(service->id);
    g_free(service->name);
    g_free(service->service_type);
    g_free(service->ip);
    g_hash_table_remove(services, key);
}

static void ubavahid_deleteServiceCmd(struct json_object *json)
{
    const char *id = ubavahid_getJsonString(json,"id", NULL);
    const char *service_type = ubavahid_getJsonString(json,"service-type", NULL);
    if( id == NULL ) syslog(LOG_DEBUG,"ubavahid_deleteService: invalid id");
    if( service_type == NULL ) syslog(LOG_DEBUG,"ubavahid_deleteService: invalid service-type");
    
    if( id == NULL || service_type == NULL)
        return;
    char *key = g_strdup_printf("%s%s", id, service_type);
    ubavahid_deleteService(key);
    g_free(key);
}

static void ubavahid_updateServiceCmd(struct json_object *json)
{
    const char *service_type = ubavahid_getJsonString(json,"service-type", NULL);
    const char *url = ubavahid_getJsonString(json,"url", NULL);
    const char *id = ubavahid_getJsonString(json,"id", NULL);
    const char *name = ubavahid_getJsonString(json,"name", NULL);
    int32_t port = ubavahid_getJsonInt(json,"port", 0);

    if( service_type == NULL ) syslog(LOG_DEBUG,"ubavahid_updateService: invalid service-type");
    if( url == NULL ) syslog(LOG_DEBUG,"ubavahid_updateService: invalid url");
    if( id == NULL ) syslog(LOG_DEBUG,"ubavahid_updateService: invalid id");
    if( name == NULL ) syslog(LOG_DEBUG,"ubavahid_updateService: invalid name");
    if( port < 1 || port > 65635) syslog(LOG_DEBUG,"ubavahid_updateService: invalid port");
    if( service_type == NULL || url == NULL || id == NULL
        || name == NULL || port < 1 || port > 65635 ){
        syslog(LOG_DEBUG,"ubavahid_updateService: invalid fields");
        return;
    }

    char *key = g_strdup_printf("%s%s", id, service_type);
    syslog(LOG_DEBUG, "update for service %s", key);

    struct service *service = g_hash_table_lookup(services, key);
    if( service == NULL ){
        syslog(LOG_DEBUG,"ubavahid_updateService: adding new service");
        service = g_new0(struct service,1);
        //service->json = g_strdup(json_object_to_json_string(json));
        service->id = g_strdup(id);
        service->name = g_strdup(name);
        service->service_type = g_strdup(service_type);
        service->ip = g_strdup(url); //TODO: resole url to ip!
        service->port = port;
        service->tcp = ubavahid_getJsonBool(json, "tcp", FALSE);
        service->udp = ubavahid_getJsonBool(json, "udp", FALSE);
        service->ttl = 120;
        g_hash_table_insert(services, g_strdup(key), service);
        avahi_registerService(service);
    }else{
        service->ttl = 120; 
        //if(strcmp(service->json, json_object_to_json_string(json)) != 0 ){
        //    syslog(LOG_DEBUG,"ubavahid_updateService: updating service");
        //    syslog(LOG_DEBUG,"old=%s new=%s",
        //            service->json, json_object_to_json_string(json));
        //    g_free(service->json);
        //    service->json = g_strdup(json_object_to_json_string(json));
        //}
    }
    g_free(key);
}

static enum commandlist ubavahid_parseCommand(const char *cmd)
{
    guint i;
    for(i=0; i<COMMAND_COUNT; i++){
        if( strcmp(cmd, commands[i].command) == 0 )
            return commands[i].id;
    }
    return NO_COMMAND;
}


static gboolean ubavahid_read(GSocket *socket, GIOCondition condition,
                                        gpointer user_data)
{
    uint8_t buf[1500];
    user_data = NULL;
    gssize len;    
    if( condition == G_IO_IN ){
        len = g_socket_receive_from(socket,NULL,(gchar*)buf,
                                sizeof(buf)-1,NULL,NULL);
        if( len > 0 ){
            syslog(LOG_DEBUG,"ubavahid_read: Received:");
            debug_hexdump(buf,len);
            
            buf[len] = 0;
            //validate(buf, len);
            ubavahid_newMCData((char*)buf);
        }else{
            syslog(LOG_WARNING,
                "ubavahid_read: Error while receiving: len=%d",len);
        }
    }else{
        syslog(LOG_DEBUG,"ubavahid_read: Received ");
        if( condition == G_IO_ERR ){
            syslog(LOG_DEBUG,"G_IO_ERR");
        }else if( condition == G_IO_HUP ){ 
            syslog(LOG_DEBUG,"G_IO_HUP");
        }else if( condition == G_IO_OUT ){ 
            syslog(LOG_DEBUG,"G_IO_OUT");
        }else if( condition == G_IO_PRI ){ 
            syslog(LOG_DEBUG,"G_IO_PRI");
        }else if( condition == G_IO_NVAL ){ 
            syslog(LOG_DEBUG,"G_IO_NVAL - dropping source");
            return FALSE;
        }else{
            syslog(LOG_DEBUG,"unkown condition = %d",condition);
        }
    }
    return TRUE;
}


static gboolean ubavahid_tick(gpointer data)
{
    data = NULL;
    GHashTableIter iter;
    char *id;
    struct service *service;
    g_hash_table_iter_init (&iter, services);
    //syslog(LOG_DEBUG,"ubavahid_tick: decrement");
    while( g_hash_table_iter_next (&iter, (void**)&id, (void**)&service) ){
        //syslog(LOG_DEBUG,"ubavahid_tick: decrementing %s", id);
        if( service->ttl )
            service->ttl--;
    }

    //syslog(LOG_DEBUG,"ubavahid_tick: delete");
    while( TRUE ){
        gboolean clean = TRUE;
        g_hash_table_iter_init (&iter, services);
        while( g_hash_table_iter_next (&iter, (void**)&id, (void**)&service) ){
            if( service->ttl == 0 ){
                ubavahid_deleteService(id);
                clean = FALSE;
                break;
            }
        }
        if( clean == TRUE ){
            //syslog(LOG_DEBUG,"ubavahid_tick: clean");
            break;
        }
    }
    return TRUE;
}

