#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "cpu.h"
#define FILENAME	"/proc/stat"

void glibtop_get_cpu(struct glibtop_cpu *buf)
{
	char* p;
	char buffer[4096];
        FILE *f = fopen(FILENAME, "r");
        fgets(buffer, sizeof(buffer), f);
        fclose(f);

	/*
	 * GLOBAL
	 */

	p = skip_token (buffer);	/* "cpu" */
	buf->user = strtoull (p, &p, 0);
	buf->nice = strtoull (p, &p, 0);
	buf->sys  = strtoull (p, &p, 0);
	buf->idle = strtoull (p, &p, 0);
	buf->total = buf->user + buf->nice + buf->sys + buf->idle;
}

