#include <stdbool.h>
#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include "leds.h"
#include "net/netstack.h"
#include <stdio.h>
#include "core/net/linkaddr.h"
#include "node-id.h"
#include "sched_beacon.h"
/*---------------------------------------------------------------------------*/
#define RSSI_THRESHOLD -95 // filter bad links
/*---------------------------------------------------------------------------*/
/* Beacon timer callback */
void
beacon_timer_cb(void* ptr)
{
  struct sched_collect_conn *conn = ptr;
  send_beacon(conn);
  if(conn->metric == 0){
    conn->beacon_seqn += 1;
    ctimer_set(&conn->beacon_timer, BEACON_INTERVAL, beacon_timer_cb, conn);
  }
}
/*---------------------------------------------------------------------------*/
/* Send beacon using the current seqn, metric and delay*/
void
send_beacon(struct sched_collect_conn* conn)
{
  // create beacon with seqn, metric and accumulated delay
  struct beacon_msg beacon = {
    .seqn = conn->beacon_seqn, 
    .metric = conn->metric
  };

  // if it is the sink, delay sent is 0, else is the accumulated
  if(conn->metric == 0){
    beacon.delay = 0;
  }
  else{
    beacon.delay = conn->sum_delay;
  }

  // fill packetbuf and send broadcast
  packetbuf_clear();
  packetbuf_copyfrom(&beacon, sizeof(beacon));
  printf("beacon: sending beacon: seqn %d metric %d ts %u diff %u\n", 
    conn->beacon_seqn, conn->metric, (uint16_t)clock_time(), (uint16_t)clock_time()-(uint16_t)(conn->rx+conn->my_delay));
  broadcast_send(&conn->bc);
}
/*---------------------------------------------------------------------------*/
/* Beacon receive callback */
void
bc_recv(struct broadcast_conn *bc_conn, const linkaddr_t *sender)
{
  struct beacon_msg beacon;
  int16_t rssi;
  /* Get the pointer to the overall structure sched_collect_conn from its field bc */
  struct sched_collect_conn* conn = (struct sched_collect_conn*)(((uint8_t*)bc_conn) - 
    offsetof(struct sched_collect_conn, bc));

  if (packetbuf_datalen() != sizeof(struct beacon_msg)) {
    printf("beacon: broadcast of wrong size\n");
    return;
  }
  memcpy(&beacon, packetbuf_dataptr(), sizeof(struct beacon_msg));
  rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
  printf("beacon: recv beacon from %02x:%02x seqn %u metric %u rssi %d ts %u\n", 
      sender->u8[0], sender->u8[1], 
      beacon.seqn, beacon.metric, rssi, (uint16_t)clock_time());

  clock_time_t my_delay;

  if(rssi > RSSI_THRESHOLD){ // if enough power

    // UPDATE 1. if the beacon metric improve the current metric more than 1
    //           (to avoid that multiple potential parents are chosen one after the other)
    //        2. if is the new seqn, without checking the metric (to account network changes)
    if( beacon.metric+1 < conn->metric || conn->beacon_seqn < beacon.seqn){

      conn->beacon_seqn = beacon.seqn;
      conn->metric = beacon.metric+1;
      linkaddr_copy(&conn->parent, sender);

      my_delay =  random_rand() % BEACON_MAX_DELAY; // sample a random delay in range [0, BEACON_MAX_DELAY-1]
      conn->my_delay = my_delay;
      conn->rx = clock_time();
      conn->sum_delay = beacon.delay + my_delay;
      conn->epoch_begin = conn->rx-beacon.delay;
      
      printf("beacon: UPDATE parent %02x seqn %u  metric %u\nepoch_begin %u, beacon_tx %u, my_delay %u  recv_delay %u\n",  
            conn->parent.u8[0], conn->beacon_seqn, conn->metric,
            (uint16_t)(conn->epoch_begin), (uint16_t)(conn->rx+my_delay),
            (uint16_t)my_delay, (uint16_t)beacon.delay);
    
      // notify synch phase or guard phase that the routing beacon has been received
      process_post(&node_process, BEACON_ARRIVED, conn);

      // wait a random delay and send the beacon
      ctimer_set(&conn->beacon_timer, my_delay, beacon_timer_cb, conn);
    }
  }
}