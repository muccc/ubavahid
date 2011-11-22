#ifndef _UBAVAHID_H_
#define _UBAVAHID_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>
#include "avahi.h"

void ubavahid_init(char *interface, char *groupaddress);

#endif
