#include "common.h"
#include "mem.h"

#define FILENAME	"/proc/meminfo"

void glibtop_get_mem (struct glibtop_mem *buf)
{
	char buffer [4096];

	file_to_buffer(buffer, sizeof buffer, FILENAME);

	buf->total  = get_scaled(buffer, "MemTotal:");
	buf->free   = get_scaled(buffer, "MemFree:");
	buf->used   = buf->total - buf->free;
}

