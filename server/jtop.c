
#define _XOPEN_SOURCE 600     // For flockfile() on Linux

#include "glibtop/mem.h"
#include "glibtop/swap.h"
#include "glibtop/cpu.h"
#include "glibtop/conntrack.h"
#include "glibtop/netlist.h"
#include "glibtop/netload.h"
#include "glibtop/wireless.h"

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>


#define MAX_HISTORY 30
struct info_data {
  float cpu_total;
  float cpu_used;
  float memory_usage;
  float swap_usage;
  float network_in;
  float network_out;
  float tcp_conns;
  float udp_conns;
  float iw_signal;
  float iw_noise;
  time_t timestamp;
};
static struct info_data data_history[MAX_HISTORY];
int last_index=MAX_HISTORY-1;
pthread_mutex_t info_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t new_info_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t new_info_cond = PTHREAD_COND_INITIALIZER;

int active_thread_count=0;
pthread_mutex_t active_thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t active_thread_returned_cond = PTHREAD_COND_INITIALIZER;

static void append_sysinfo()
{
  struct glibtop_cpu cpu;
  struct glibtop_mem mem;
  struct glibtop_swap swap;
  struct glibtop_conntrack conntrack;
#ifndef NETLOAD_IFACE
  struct glibtop_netlist netlist;
  int i;
  char **ifnames;
#endif

  const time_t timestamp = time(NULL);
  if (timestamp == data_history[last_index].timestamp) return;

  glibtop_get_cpu(&cpu);
  glibtop_get_mem(&mem);
  glibtop_get_swap(&swap);
  glibtop_get_conntrack(&conntrack);

  uint64_t in = 0;
  uint64_t out = 0;
#ifndef NETLOAD_IFACE
  ifnames = glibtop_get_netlist(&netlist);
  for (i = 0; i < netlist.number; ++i)
#endif
  {
    struct glibtop_netload netload;
#ifndef NETLOAD_IFACE
    glibtop_get_netload (&netload, ifnames[i]);

    if (netload.is_loopback)
      continue;

    /* Skip interfaces without any IPv4/IPv6 address (or
       those with only a LINK ipv6 addr) However we need to
       be able to exclude these while still keeping the
       value so when they get online (with NetworkManager
       for example) we don't get a suddent peak.  Once we're
       able to get this, ignoring down interfaces will be
       possible too.  */
    if (!netload.has_addr || (netload.has_addr && netload.is_linklocal))
        continue;
#else
    glibtop_get_netload (&netload, NETLOAD_IFACE);
#endif

    /* Don't skip interfaces that are down (GLIBTOP_IF_FLAGS_UP)
       to avoid spikes when they are brought up */

    in  += netload.bytes_in;
    out += netload.bytes_out;

#ifndef NETLOAD_IFACE
    free(ifnames[i]);
#endif
  }
#ifndef NETLOAD_IFACE
  free(ifnames);
#endif

#ifdef IW_IFACE
  struct glibtop_wireless wireless;
  glibtop_get_wireless(&wireless, IW_IFACE);
#endif

  pthread_mutex_lock(&info_mutex);
  last_index = (last_index+1) % MAX_HISTORY;
  data_history[last_index].cpu_total = (float)cpu.total;
  data_history[last_index].cpu_used = (float)(cpu.user + cpu.nice + cpu.sys);
  data_history[last_index].memory_usage = ((float)mem.used) / ((float)mem.total);
  data_history[last_index].swap_usage = swap.total == 0 ? 0.0f : ((float)swap.used) / ((float)swap.total);
  data_history[last_index].timestamp = timestamp;
  data_history[last_index].network_in = (float) in;
  data_history[last_index].network_out = (float) out;
  data_history[last_index].tcp_conns = (float) conntrack.tcp_conns;
  data_history[last_index].udp_conns = (float) conntrack.udp_conns;

#ifdef IW_IFACE
  data_history[last_index].iw_signal = (float) wireless.signal_dbm;
  data_history[last_index].iw_noise = (float) wireless.noise_dbm;
#else
  data_history[last_index].iw_signal = 0.0f;
  data_history[last_index].iw_noise = 0.0f;
#endif

  pthread_mutex_unlock(&info_mutex);
}

static int data_thread_shutdown_flag = 0;
static void* data_thread_main(void *arg) {
  while(data_thread_shutdown_flag == 0) {
    append_sysinfo();

    // Wake all threads sending info to clients
    pthread_mutex_lock(&new_info_mutex);
    pthread_cond_broadcast(&new_info_cond);
    pthread_mutex_unlock(&new_info_mutex);

    sleep(1);
  }
  return NULL;
}

static int shuttingdown_flag = 0;
static pthread_mutex_t shuttingdown_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

static void sig_handler(int signo) {
  if (signo == SIGINT) {
    int handled;
    pthread_mutex_lock(&shuttingdown_flag_mutex);
    if (shuttingdown_flag) {
      handled = 1;
    } else {
      handled = 0;
      shuttingdown_flag = 1;
    }
    pthread_mutex_unlock(&shuttingdown_flag_mutex);
    if (handled) {
      printf("SIGINT already causing shutdown. Ignoring signal.\n");
      return;
    }

    printf("Received SIGINT, shutting down...\n");
  }
}

void* connection_thread_main(void *arg) {
  const int sockfd = *((int*)arg);
  free(arg);
  char content[16384];
  int content_length;
  int ret;
 
  while (!shuttingdown_flag) {
    pthread_mutex_lock(&new_info_mutex);
    pthread_cond_wait(&new_info_cond, &new_info_mutex);
    pthread_mutex_unlock(&new_info_mutex);

    pthread_mutex_lock(&info_mutex);

    int index = last_index;
    int lindex = (last_index + MAX_HISTORY - 1) % MAX_HISTORY;
    if (data_history[index].timestamp == 0 || data_history[lindex].timestamp == 0) {
      pthread_mutex_unlock(&info_mutex);
      continue;
    }

    content_length = 0;
    content_length += snprintf(content + content_length, sizeof(content) - content_length,
      "{\"timestamp\":%lu,\"cpu_usage\":%f,\"mem_usage\":%f,\"swap_usage\":%f,\"net_in\":%f,\"net_out\":%f,\"tcp_conns\":%f,\"udp_conns\":%f,\"iw_signal\":%f,\"iw_noise\":%f}",
        data_history[index].timestamp,
        (data_history[index].cpu_used - data_history[lindex].cpu_used) / (data_history[index].cpu_total - data_history[lindex].cpu_total),
        data_history[index].memory_usage,
        data_history[index].swap_usage,
        (data_history[index].network_in - data_history[lindex].network_in) / ((float)(data_history[index].timestamp - data_history[lindex].timestamp)),
        (data_history[index].network_out - data_history[lindex].network_out) / ((float)(data_history[index].timestamp - data_history[lindex].timestamp)),
        data_history[index].tcp_conns,
        data_history[index].udp_conns,
        data_history[index].iw_signal,
        data_history[index].iw_noise
    );

    pthread_mutex_unlock(&info_mutex);
    // Write the result, including the null terminator
    ret = write(sockfd, content, content_length+1);
    if (ret < 0) break;
    //ret = write(sockfd, ";", 1);
    //if (ret < 0) break;
  }

  printf("Closing child thread.\n"); 
  close(sockfd);

  pthread_mutex_lock(&active_thread_count_mutex);
  active_thread_count -= 1;
  pthread_cond_broadcast(&active_thread_returned_cond);
  pthread_mutex_unlock(&active_thread_count_mutex);

  return NULL;
}

int main(int argc, char **argv) {
  int i;
  int socketHandle;
  struct sockaddr_in socketInfo;
  pthread_t data_thread;

  if (argc != 2 || atoi(argv[1]) == 0) {
    printf("Usage: %s <port_number>\n", argv[0]);
    return -1;
  }

  // Initialize data
  for (i = 0; i < MAX_HISTORY; i++) data_history[i].timestamp = 0;
  pthread_create( &data_thread, NULL, data_thread_main, NULL);

  // Start the web server.
  bsd_signal(SIGINT, sig_handler);
  bsd_signal(SIGPIPE, SIG_IGN);

  // Main loop
  socketHandle = socket(AF_INET, SOCK_STREAM, 0);
  socketInfo.sin_family = AF_INET;
  // Use any address available to the system. This is a typical configuration for a server.
  // Note that this is where the socket client and socket server differ.
  // A socket client will specify the server address to connect to.
  socketInfo.sin_addr.s_addr = htonl(INADDR_ANY); // Translate long integer to network byte order.
  socketInfo.sin_port = htons(atoi(argv[1]));        // Set port number
  // Bind the socket to a local socket address
  if( bind(socketHandle, (struct sockaddr *) &socketInfo, sizeof(struct sockaddr_in)) < 0)
  {
     close(socketHandle);
     perror("bind");
     exit(EXIT_FAILURE);
  }
  listen(socketHandle, 1);

  printf("Entering main thread...\n");
  int socketConnection;
  while (!shuttingdown_flag) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(socketHandle, &fds);
    int ret = select(socketHandle+1, &fds, NULL, NULL, NULL);
    if (ret >= 0 && FD_ISSET(socketHandle, &fds)) {
      printf("Accepting a new connection.\n");
      socketConnection = accept(socketHandle, NULL, NULL);

      pthread_mutex_lock(&active_thread_count_mutex);
      active_thread_count += 1;
      pthread_mutex_unlock(&active_thread_count_mutex);

      int* thread_data = malloc(sizeof(int));
      *thread_data = socketConnection;
      pthread_t child_thread;
      pthread_create( &child_thread, NULL, connection_thread_main, thread_data);
    }
  }

  // Close bind socket
  close(socketHandle);

  data_thread_shutdown_flag = 1;
  printf("Waiting on data thread to shutdown.\n");
  pthread_join( data_thread, NULL );

  printf("Waiting for active threads to return.\n");
  pthread_mutex_lock(&active_thread_count_mutex);
  while (active_thread_count > 0) {
    // Make sure threads aren't just sleeping
    pthread_mutex_lock(&new_info_mutex);
    pthread_cond_broadcast(&new_info_cond);
    pthread_mutex_unlock(&new_info_mutex);

    pthread_cond_wait(&active_thread_returned_cond, &active_thread_count_mutex);
  }
  pthread_mutex_unlock(&active_thread_count_mutex);

  printf("Clean shutdown.\n");

  return 0;
}

