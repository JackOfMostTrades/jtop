#include "common.h"
#include "netload.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <net/if.h>
#include <sys/ioctl.h>

#ifndef NO_IPV6
#include <ifaddrs.h>
#endif /* NO_IPV6 */

#ifndef NO_IPV6
static void get_ipv6(struct glibtop_netload *buf, const char *interface)
{
	struct ifaddrs *ifa0, *ifr6;

	if(getifaddrs (&ifa0) != 0)
	{
		return;
	}

	for (ifr6 = ifa0; ifr6; ifr6 = ifr6->ifa_next) {
		if (strcmp (ifr6->ifa_name, interface) == 0
		    && ifr6->ifa_addr != NULL
		    && ifr6->ifa_addr->sa_family == AF_INET6)
			break;
	}

	if(ifr6) {
	if (IN6_IS_ADDR_LINKLOCAL ( &((struct sockaddr_in6 *) ifr6->ifa_addr)->sin6_addr ))
                buf->is_linklocal = 1;
        else
                buf->is_linklocal = 0;
   }

	freeifaddrs(ifa0);
}
#endif /* NO_IPV6 */

static void linux_2_4_stats(struct glibtop_netload *buf, const char *interface)
{
    char buffer [4096], *p;
    int have_bytes, fields;
    FILE *f;

    /* Ok, either IP accounting is not enabled in the kernel or
     * it was not enabled for the requested interface. */

    f = fopen ("/proc/net/dev", "r");
    if (!f) {
	return;
    }

    /* Skip over the header line. */
    fgets (buffer, BUFSIZ-1, f);
    fgets (buffer, BUFSIZ-1, f);

    /* Starting with 2.1.xx (don't know exactly which version)
     * /proc/net/dev contains both byte and package counters. */

    p = strchr (buffer, '|');
    if (!p) {
	fclose (f);
	return;
    }

    /* Do we already have byte counters ? */
    have_bytes = strncmp (++p, "bytes", 5) == 0;

    /* Count remaining 'Receive' fields so we know where
     * the first 'Transmit' field starts. */

    fields = 0;
    while (*p != '|') {
	if (*p++ != ' ') continue;
	while (*p++ == ' ') ;
	fields++;
    }

    /* Should never happen. */
    if (fields < 2) {
	fclose (f);
	return;
    }
    fields--;

    while (fgets (buffer, BUFSIZ-1, f)) {
	char *p, *dev;

	dev = buffer;
	while (*dev == ' ') dev++;

	p = strchr (dev, ':');
	if (!p) continue;
	*p++ = 0;

	/* If it's not a digit, then it's most likely an error
	 * message like 'No statistics available'. */
	while (*p == ' ') p++;
	if (*p < '0' || *p > '9') continue;

	if (strcmp (dev, interface))
	    continue;

	/* Ok, we've found the interface */

	/* Only read byte counts if we really have them. */

	if (have_bytes) {
	    buf->bytes_in = strtoull (p, &p, 0);
	    fields--;
	}

        // Packets in
	strtoull (p, &p, 0);
        // Errors in
	strtoull (p, &p, 0);

	p = skip_multiple_token (p, fields);

	if (have_bytes)
	    buf->bytes_out = strtoull (p, &p, 0);

        // Packets out
	strtoull (p, &p, 0);
        // Errors out
	strtoull (p, &p, 0);

	p = skip_multiple_token (p, 2);

        // collisions
	strtoull (p, &p, 0);

	break; /* finished */
    }

    fclose (f);
}




/* Provides network statistics. */

void glibtop_get_netload(struct glibtop_netload *buf, const char *interface)
{
    int skfd;

    skfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (skfd) {
	struct ifreq ifr;

	strlcpy (ifr.ifr_name, interface, sizeof ifr.ifr_name);
	if (!ioctl (skfd, SIOCGIFFLAGS, &ifr)) {
	    const unsigned long long flags = ifr.ifr_flags;
            buf->is_loopback = (flags & IFF_LOOPBACK) ? 1 : 0; 
	}

	strlcpy (ifr.ifr_name, interface, sizeof ifr.ifr_name);
	if (!ioctl (skfd, SIOCGIFADDR, &ifr)) {
            buf->has_addr = 1;
	} else {
            buf->has_addr = 0;
        }
	close (skfd);
    }

    linux_2_4_stats(buf, interface);

    buf->is_linklocal = 0;
#ifndef NO_IPV6
    get_ipv6(buf, interface);
#endif /* NO_IPV6 */
}

