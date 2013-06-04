
#include "common.h"
#include "wireless.h"
#include <sys/socket.h>
#include <linux/wireless.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

/*
void glibtop_get_wireless(struct glibtop_wireless *buf, const char *ifname) {
	int sockfd;
	struct iw_statistics stats;
	struct iwreq req = {
		.u.data = {
			.pointer = &stats,
			.flags = 1,
			.length = sizeof(struct iw_statistics)
		}
	};
        strlcpy(req.ifr_name, ifname, sizeof req.ifr_name);

        memset(buf, 0, sizeof(struct glibtop_wireless)); 

	// Any old socket will do, and a datagram socket is pretty cheap
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("Unable to open dgram\n");
		return;
	}

	// Perform the ioctl
	if(ioctl(sockfd, SIOCGIWSTATS, &req) == -1) {
		printf("Unable to do ioctl\n");
		close(sockfd);
                return;
	}

	close(sockfd);
//        buf->signal_dbm = stats.qual.level - (stats.qual.updated & IW_QUAL_DBM ? 0 : 256);
//        buf->noise_dbm = stats.qual.noise - (stats.qual.updated & IW_QUAL_DBM ? 0 : 256);
        buf->signal_dbm = stats.qual.level;
        buf->noise_dbm = stats.qual.noise;
	printf("DBM: %d\n", stats.qual.updated & IW_QUAL_DBM);
}
*/

void glibtop_get_wireless(struct glibtop_wireless *buf, const char *ifname) {
    char buffer[BUFSIZ];

    memset(buf, 0, sizeof(struct glibtop_wireless)); 
    FILE *f = fopen("/proc/net/wireless", "r");

    while (fgets (buffer, BUFSIZ-1, f)) {
        char *p, *q, *dev;

        dev = buffer;
        while (*dev == ' ') dev++;

        p = strchr (dev, ':');
        if (!p) continue;
        *p++ = 0;

        /* If it's not a digit, then it's most likely an error
         * message like 'No statistics available'. */
        while (*p == ' ') p++;
        if (*p < '0' || *p > '9') continue;

        if (strcmp (dev, ifname))
            continue;

        /* Ok, we've found the interface */
	p = skip_multiple_token(p, 2);
	q = p;
	while (('0' <= *p && *p <= '9') || *p == '-') p++;
	*p++ = '\0';
	buf->signal_dbm = atoi(q);
        while(*p == ' ' && *p != '\0') p++;
        q = p;
	while (('0' <= *p && *p <= '9') || *p == '-') p++;
	*p++ = '\0';
	buf->noise_dbm = atoi(q);

	break;
    }
    fclose(f);

    if (buf->signal_dbm >= 0) buf->signal_dbm -= 256;
    if (buf->noise_dbm >= 0) buf->noise_dbm -= 256;

}

