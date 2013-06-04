#include "common.h"
#include "swap.h"

#define MEMINFO		"/proc/meminfo"

void glibtop_get_swap (struct glibtop_swap *buf)
{
	char buffer [4096];
	file_to_buffer(buffer, sizeof buffer, MEMINFO);

	buf->total = get_scaled(buffer, "SwapTotal:");
	buf->free = get_scaled(buffer, "SwapFree:");
	buf->used = buf->total - buf->free;
}
