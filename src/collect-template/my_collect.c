#include <stdbool.h>
#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include "leds.h"
#include "net/netstack.h"
#include <stdio.h>
#include "core/net/linkaddr.h"
#include "my_collect.h"
/*---------------------------------------------------------------------------*/
#define BEACON_INTERVAL (CLOCK_SECOND * 30)
#define BEACON_MAX_DELAY CLOCK_SECOND / 10
/*---------------------------------------------------------------------------*/
#define RSSI_THRESHOLD -95
/*---------------------------------------------------------------------------*/
/* Callback function declarations */
void bc_recv(struct broadcast_conn *conn, const linkaddr_t *sender);
void uc_recv(struct unicast_conn *c, const linkaddr_t *from);
void beacon_timer_cb(void* ptr);
/*---------------------------------------------------------------------------*/
/* Rime Callback structures */
struct broadcast_callbacks bc_cb = {
  .recv = bc_recv,
  .sent = NULL
};
struct unicast_callbacks uc_cb = {
  .recv = uc_recv,
  .sent = NULL
};
/*---------------------------------------------------------------------------*/
void
my_collect_open(struct my_collect_conn* conn, uint16_t channels, 
                bool is_sink, const struct my_collect_callbacks *callbacks)
{
  /* Initialize the connector structure */
  linkaddr_copy(&conn->parent, &linkaddr_null);
  conn->metric = 65535; /* The MAX metric (the node is not connected yet) */
  conn->beacon_seqn = 0;
  conn->callbacks = callbacks;

  /* Open the underlying Rime primitives */
  broadcast_open(&conn->bc, channels,     &bc_cb);
  unicast_open  (&conn->uc, channels + 1, &uc_cb);

  /* TO DO 1: SINK
   * 1. Make the sink send beacons periodically (BEACON_INTERVAL)
   */
  if(is_sink){
    conn->metric = 0;
    ctimer_set(&conn->beacon_timer, 0, beacon_timer_cb, conn);
  }

}
/*---------------------------------------------------------------------------*/
/*                              Beacon Handling                              */
/*---------------------------------------------------------------------------*/
/* Beacon message structure */
struct beacon_msg {
  uint16_t seqn;
  uint16_t metric;
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
/* Beacon timer callback */
void
beacon_timer_cb(void* ptr)
{
  /* TO DO 2: Implement the beacon callback
   * 1. Send beacon
   * 2. Should the sink do anything else?
   */

  struct my_collect_conn *conn = ptr;

  send_beacon(conn);

  // increment beacon seq no
  if(conn->metric == 0){
    conn->beacon_seqn += 1;
    ctimer_set(&conn->beacon_timer, BEACON_INTERVAL, beacon_timer_cb, conn);
  }

}
/*---------------------------------------------------------------------------*/
/* Send beacon using the current seqn and metric */
void
send_beacon(struct my_collect_conn* conn)
{
  struct beacon_msg beacon = {
    .seqn = conn->beacon_seqn, 
    .metric = conn->metric
  };

  packetbuf_clear();
  packetbuf_copyfrom(&beacon, sizeof(beacon));
  printf("my_collect: sending beacon: seqn %d metric %d\n",
    conn->beacon_seqn, conn->metric);
  broadcast_send(&conn->bc);
}
/*---------------------------------------------------------------------------*/
/* Beacon receive callback */
void
bc_recv(struct broadcast_conn *bc_conn, const linkaddr_t *sender)
{
  struct beacon_msg beacon;
  int16_t rssi;
  /* Get the pointer to the overall structure my_collect_conn from its field bc */
  struct my_collect_conn* conn = (struct my_collect_conn*)(((uint8_t*)bc_conn) - 
    offsetof(struct my_collect_conn, bc));

  if (packetbuf_datalen() != sizeof(struct beacon_msg)) {
    printf("my_collect: broadcast of wrong size\n");
    return;
  }
  memcpy(&beacon, packetbuf_dataptr(), sizeof(struct beacon_msg));
  rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
  printf("my_collect: recv beacon from %02x:%02x seqn %u metric %u rssi %d\n", 
      sender->u8[0], sender->u8[1], 
      beacon.seqn, beacon.metric, rssi);

  /* TO DO 3:
   * 1. Analyze the received beacon based on RSSI, seqn, and metric.
   * 2. Update (if needed) the local/node current routing info (parent, metric).
   */

  bool update = false;
  clock_time_t my_delay;

  if(rssi > RSSI_THRESHOLD){ // if enough power

    if(beacon.metric+1 < conn->metric || conn->beacon_seqn < beacon.seqn){ 
      conn->beacon_seqn = beacon.seqn;
      conn->metric = beacon.metric+1;
      linkaddr_copy(&conn->parent, sender);
      my_delay =  random_rand() % BEACON_MAX_DELAY;
      update = true;
    }
  }

  /* TO DO 4:
   * If the metric or the seqn has been updated, retransmit the beacon on to update
   * the node neighbors about the changes
   */
  if(update){
    printf("my_collect: UPDATE parent %02x seqn %u  metric %u\n", conn->parent.u8[0], conn->beacon_seqn, conn->metric);
    ctimer_set(&conn->beacon_timer, my_delay , beacon_timer_cb, conn);   
  }

}
/*---------------------------------------------------------------------------*/
/*                               Data Handling                               */
/*---------------------------------------------------------------------------*/
/* Header structure for data packets */
struct collect_header {
  linkaddr_t source;
  uint8_t hops;
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
/* Data Collection: send function */
int
my_collect_send(struct my_collect_conn *conn)
{
  struct collect_header hdr = {.source=linkaddr_node_addr, .hops=0};
  int ret;

  if (linkaddr_cmp(&conn->parent, &linkaddr_null))
    return 0; // no parent

  /* TO DO 5:
   * 1. Allocate space for the data collection header 
   * 2. Insert the header in the packet buffer
   * 3. Send the packet to the parent using unicast
   */

  ret = packetbuf_hdralloc(sizeof(hdr));

  memcpy(packetbuf_hdrptr(), &hdr, sizeof(hdr));
  unicast_send(&conn->uc, &conn->parent);

  return(ret);
}
/*---------------------------------------------------------------------------*/
/* Data receive callback */
void
uc_recv(struct unicast_conn *uc_conn, const linkaddr_t *from)
{
  /* Get the pointer to the overall structure my_collect_conn from its field uc */
  struct my_collect_conn* conn = (struct my_collect_conn*)(((uint8_t*)uc_conn) - 
    offsetof(struct my_collect_conn, uc));

  struct collect_header hdr;

  if (packetbuf_datalen() < sizeof(struct collect_header)) {
    printf("my_collect: too short unicast packet %d\n", packetbuf_datalen());
    return;
  }

  /* TO DO 6:
   * 1. Extract the header
   * 2. On the sink, remove the header and call the application callback
   * 3. On a router, update the header and forward the packet to the parent using unicast
   */

  memcpy(&hdr,packetbuf_dataptr(),sizeof(hdr));

  if(conn->metric == 0){ // if it is the sink
    packetbuf_hdrreduce(sizeof(hdr));
    conn->callbacks->recv(&hdr.source, hdr.hops+1);
  }
  else{
    hdr.hops += 1;
    memcpy(packetbuf_dataptr(), &hdr, sizeof(hdr));
    unicast_send(&conn->uc, &conn->parent);
  }

}
/*---------------------------------------------------------------------------*/

