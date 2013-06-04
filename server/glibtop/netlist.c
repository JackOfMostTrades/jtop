#include "common.h"
#include "netlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char** glibtop_get_netlist(struct glibtop_netlist *buf)
{
	char line[1024];
	FILE *f;

	f = fopen("/proc/net/dev", "r");

        buf->number = 0;
        uint32_t list_len = 8;
        char **list = malloc(sizeof(char*) * list_len);

	while(fgets(line, sizeof line, f))
	{
		char *sep = strchr(line, ':');

		if(!sep) continue;

		*sep = '\0'; /* truncate : we only need the name */
                // Trim spaces from front
                sep = line;
                while (*sep == ' ') sep++;

                if (buf->number == list_len) {
                  list_len += 8;
                  list = realloc(list, sizeof(char*) * list_len);
                }
                list[buf->number] = malloc(sizeof(char) * (strlen(sep)+1));
                strcpy(list[buf->number], sep);
		buf->number++;
	}

	fclose(f);
        list = realloc(list, sizeof(char*) * buf->number);
	return list;
}

