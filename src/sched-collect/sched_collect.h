#ifndef SCHED_COLLECT_H
#define SCHED_COLLECT_H
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdlib.h>
#include "contiki.h"
#include "net/rime/rime.h"
#include "net/netstack.h"
#include "core/net/linkaddr.h"
/*---------------------------------------------------------------------------*/
/* Time and periods used for the different phases */
#define n 50
#define p 10

#define EPOCH_DURATION (30 * CLOCK_SECOND)

#define BEACON_INTERVAL (30 * CLOCK_SECOND)
#define BEACON_MAX_DELAY CLOCK_SECOND / n
#define SYNCH_DURATION MAX_HOPS*(BEACON_MAX_DELAY+30)

#define SCHED_COLLECT (node_id-2)*p*(MAX_HOPS) // formula (ID-2)*p of slides
#define COLLECT_DURATION (MAX_NODES-1)*p*(MAX_HOPS)

#define GUARD_DURATION 30

#define RADIO_OFF EPOCH_DURATION-SYNCH_DURATION-COLLECT_DURATION-GUARD_DURATION

process_event_t BEACON_ARRIVED;

/*---------------------------------------------------------------------------*/
#ifndef CONTIKI_TARGET_SKY
/* Testbed experiments with Zoul Firefly platform */
#define MAX_HOPS 4
#define MAX_NODES 36
#else
/* Cooja experiments with Tmote Sky platform */
#define MAX_HOPS 3
#define MAX_NODES 9
#endif
/*---------------------------------------------------------------------------*/
PROCESS_NAME(node_process);
/*---------------------------------------------------------------------------*/
#define COLLECT_CHANNEL 0xAA
/*---------------------------------------------------------------------------*/
/* Callback structure */
struct sched_collect_callbacks {
  void (* recv)(const linkaddr_t *originator, uint8_t hops);
};
/*---------------------------------------------------------------------------*/
/* Connection object */
struct sched_collect_conn {
  struct broadcast_conn bc;
  struct unicast_conn uc;
  const struct sched_collect_callbacks* callbacks;
  linkaddr_t parent;
  struct ctimer beacon_timer;
  uint16_t metric;
  uint16_t beacon_seqn;

  // you can add other useful variables to the object
  clock_time_t rx;          // beacon reception timestamp
  clock_time_t prev_delay;  // accumulated delay in the received packet
  clock_time_t sum_delay;   // accumulated delay sent in the next packet
  clock_time_t epoch_begin; // synchronized beginning timestamp of the epoch
  clock_time_t my_delay;    // sampled delay
};
/*---------------------------------------------------------------------------*/
/* Initialize a collect connection
 *  - conn -- a pointer to a connection object
 *  - channels -- starting channel C (the collect uses two: C and C+1)
 *  - is_sink -- initialize in either sink or router mode
 *  - callbacks -- a pointer to the callback structure */
void sched_collect_open(
    struct sched_collect_conn* conn,
    uint16_t channels,
    bool is_sink,
    const struct sched_collect_callbacks *callbacks);
/*---------------------------------------------------------------------------*/
/* Send packet to the sink 
 * Parameters:
 *  - conn -- a pointer to a connection object
 *  - data -- a pointer to the data packet to be sent
 *  - len  -- data length to be send in bytes
 * 
 * Returns zero if the packet cannot be stored nor sent.
 * Non-zero otherwise.
 */
int sched_collect_send(
    struct sched_collect_conn *c,
    uint8_t *data,
    uint8_t len);
/*---------------------------------------------------------------------------*/
/* Header structure for data packets */
struct collect_header {
  linkaddr_t source;
  uint8_t hops;
} __attribute__((packed));

void send_msg(struct sched_collect_conn *conn);
/*---------------------------------------------------------------------------*/

#endif //SCHED_COLLECT_H
