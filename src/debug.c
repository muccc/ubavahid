#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>

#include "debug.h"

void debug_hexdump(uint8_t * data, uint16_t len)
{
    uint16_t i,j;
    char *buf = malloc(len*5);
    if( buf == NULL ){
        syslog(LOG_ERR,"debug_hexdump: malloc failed");
        return;
    }

    j = 0;
    for(i=0; i<len; i++){
        if( data[i] < 0x10 ){
            j += sprintf(buf+j, " 0x0%X", data[i]);
        }else if (data[i] < ' ' || data[i] > 0x7F){
            j += sprintf(buf+j, " 0x%X", data[i]);
        }else{
            j += sprintf(buf+j, "%c", data[i]);
        }
    }
    syslog(LOG_DEBUG, "%s", buf);
    free(buf);
}

void ub_assertion_message_expr        (const char     *domain,
                                         const char     *file,
                                         int             line,
                                         const char     *func, 
                                         const char     *expr)
{
    syslog(LOG_ERR,"assertion failed. domain=%s file=%s line=%d func=%s expr=%s",
        domain, file, line, func, expr);
    exit(-1);
}
