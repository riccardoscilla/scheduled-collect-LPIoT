#ifndef SCHED_BEACON_H
#define SCHED_BEACON_H
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include "contiki.h"
#include "net/rime/rime.h"
#include "net/netstack.h"
#include "core/net/linkaddr.h"
#include "sched_collect.h"
/*---------------------------------------------------------------------------*/
/* Routing and synchronization beacons */
struct beacon_msg { // Beacon message structure
  uint16_t seqn;
  uint16_t metric;
  clock_time_t delay; // embed the transmission delay to help nodes synchronize
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
void send_beacon(struct sched_collect_conn* conn);
/*---------------------------------------------------------------------------*/
void beacon_timer_cb(void* ptr);
/*---------------------------------------------------------------------------*/
#endif //SCHED_BEACON_H