#include <stdbool.h>
#include "contiki.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include "leds.h"
#include "net/netstack.h"
#include <stdio.h>
#include "core/net/linkaddr.h"
#include "node-id.h"
#include "sched_collect.h"
#include "sched_beacon.h"
/*---------------------------------------------------------------------------*/
void bc_recv(struct broadcast_conn *conn, const linkaddr_t *sender);
void uc_recv(struct unicast_conn *c, const linkaddr_t *from);

struct broadcast_callbacks bc_cb = {
  .recv = bc_recv,
  .sent = NULL
};
struct unicast_callbacks uc_cb = {
  .recv = uc_recv,
  .sent = NULL
};
/*---------------------------------------------------------------------------*/
/* Buffer */
static uint16_t bufferlen, bufferptr;
static uint16_t buffer_a[80];
static uint8_t *buffer = (uint8_t *)buffer_a;

void buffer_clear(void)
{
  bufferlen = bufferptr = 0;
}

void buffer_set_datalen(uint16_t len)
{
  bufferlen = len;
}

uint16_t buffer_datalen(void)
{
  return bufferlen;
}

void * buffer_dataptr(void)
{
  return buffer;
}

/*---------------------------------------------------------------------------*/
PROCESS(sink_process, "Sink process");
PROCESS(node_process, "Node process"); 
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sink_process, ev, data)
{
  PROCESS_BEGIN();
  // periodically transmit the synchronization beacon
  struct sched_collect_conn *conn = data;
  conn->metric = 0;

  ctimer_set(&conn->beacon_timer, 0, beacon_timer_cb, conn);

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  PROCESS_BEGIN();

  // manage the phases of each epoch
  static struct etimer et;
  static struct sched_collect_conn *conn;
  static bool met_guard = false;
  static bool bootstrap = true;
  BEACON_ARRIVED = process_alloc_event();
  
  // turn off radio for the first period
  NETSTACK_MAC.on();

  while(1){
    // synch phase
    leds_on(LEDS_RED);


    // if the beacon didn't arrive in the guard phase
    // and if is not the first epoch, 
    // then expect to wait for the entire SYNCH_DURATION
    if(!met_guard){
      if(!bootstrap){
        etimer_set(&et, SYNCH_DURATION);
      }
      PROCESS_WAIT_EVENT();
    }
    if(ev == BEACON_ARRIVED || met_guard){
      conn = data;
      met_guard = false;
      etimer_set(&et, conn->epoch_begin-conn->rx+SYNCH_DURATION-conn->metric*27);
      // PROCESS_WAIT_EVENT();
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
      leds_off(LEDS_RED);
    }
    if(ev == PROCESS_EVENT_TIMER && etimer_expired(&et)){
      leds_off(LEDS_RED);
    }

    bootstrap = false;

    //collect phase
    etimer_set(&et, SCHED_COLLECT);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et)); // wait until it's node's time to collect
    leds_on(LEDS_BLUE); 

    if(buffer_datalen > 0){
      send_msg(conn); // collect
      buffer_clear();
    }

    etimer_set(&et, COLLECT_DURATION-SCHED_COLLECT);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et)); // wait for the rest of the time
    leds_off(LEDS_BLUE);
    
    // radio-off phase
    NETSTACK_MAC.off(false);
    etimer_set(&et, RADIO_OFF);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    NETSTACK_MAC.on();

    // guard phase
    etimer_set(&et, GUARD_DURATION);
    leds_on(LEDS_GREEN);
    PROCESS_WAIT_EVENT();
    if(ev == PROCESS_EVENT_TIMER && etimer_expired(&et)){
      leds_off(LEDS_GREEN);
    }
    else if(ev == BEACON_ARRIVED){
      met_guard = true;
      leds_off(LEDS_GREEN);
    }
    
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
sched_collect_open(struct sched_collect_conn* conn, uint16_t channels,
  bool is_sink, const struct sched_collect_callbacks *callbacks)
{
  /* Create 2 Rime connections: broadcast (for beacons) and unicast (for collection)
   * Start the appropriate process to perform the necessary epoch operations or 
   * use ctimers and callbacks as necessary to schedule these operations.
   */

  /* Initialize the connector structure */
  linkaddr_copy(&conn->parent, &linkaddr_null);
  conn->metric = 65535; /* The MAX metric (the node is not connected yet) */
  conn->beacon_seqn = 0;
  conn->callbacks = callbacks;

  /* Open the underlying Rime primitives */
  broadcast_open(&conn->bc, channels,     &bc_cb);
  unicast_open  (&conn->uc, channels + 1, &uc_cb);

  if(is_sink){
    process_start(&sink_process, conn);
  }
  else{
    process_start(&node_process, conn);
  }

}
/*---------------------------------------------------------------------------*/
int
sched_collect_send(struct sched_collect_conn *conn, uint8_t *data, uint8_t len)
{
  /* Store packet in a local buffer to be send during the data collection 
   * time window. If the packet cannot be stored, e.g., because there is
   * a pending packet to be sent, return zero. Otherwise, return non-zero
   * to report operation success. */
  if(buffer_datalen()==0){
    buffer_clear();
    memcpy(buffer_dataptr(),data,len);
    buffer_set_datalen(len);
    printf("buffer set len :%d\n", buffer_datalen());
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
send_msg(struct sched_collect_conn *conn)
{
  /* Send data collection packet to sink */

  // copy the data from the buffer
  packetbuf_clear();
  memcpy(packetbuf_dataptr(), buffer_dataptr(), buffer_datalen());
  packetbuf_set_datalen(buffer_datalen());

  /* Header */
  struct collect_header hdr = {.source=linkaddr_node_addr, .hops=0};

  if (linkaddr_cmp(&conn->parent, &linkaddr_null))
    return; // no parent

  packetbuf_hdralloc(sizeof(hdr));

  memcpy(packetbuf_hdrptr(), &hdr, sizeof(hdr));
  printf("Send to %02x\n", conn->parent.u8[0]);
  unicast_send(&conn->uc, &conn->parent);
}
/*---------------------------------------------------------------------------*/
void 
uc_recv(struct unicast_conn *uc_conn, const linkaddr_t *from)
{
  /* Get the pointer to the overall structure sched_collect_conn from its field uc */
  struct sched_collect_conn* conn = (struct sched_collect_conn*)(((uint8_t*)uc_conn) - 
    offsetof(struct sched_collect_conn, uc));

  struct collect_header hdr;

  if (packetbuf_datalen() < sizeof(struct collect_header)) {
    printf("my_collect: too short unicast packet %d\n", packetbuf_datalen());
    return;
  }

  memcpy(&hdr,packetbuf_dataptr(),sizeof(hdr));
  printf("Received packet from: %02x, Origin: %02x\n", from->u8[0], hdr.source.u8[0]);

  if(conn->metric == 0){ // if it is the sink
    packetbuf_hdrreduce(sizeof(hdr));
    conn->callbacks->recv(&hdr.source, hdr.hops+1);
  }
  else{
    hdr.hops += 1;
    printf("Source %02x hop %d Send to %02x\n", hdr.source.u8[0], hdr.hops, conn->parent.u8[0]);
    memcpy(packetbuf_dataptr(), &hdr, sizeof(hdr));
    unicast_send(&conn->uc, &conn->parent);
  }
}
/*---------------------------------------------------------------------------*/

