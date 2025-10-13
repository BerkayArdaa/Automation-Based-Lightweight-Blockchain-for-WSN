#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "dev/leds.h"

#define LOG_MODULE "Sink"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_PORT 1234

PROCESS(sink_node_process, "Sink Node");
AUTOSTART_PROCESSES(&sink_node_process);

static struct simple_udp_connection udp_conn;

/* ==========================================================
 *  Gelen veriyi dinleyen callback
 * ==========================================================*/
static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen)
{
  int temp;
  memcpy(&temp, data, sizeof(temp));

  LOG_INFO("Received temperature: %d°C from ", temp);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");

  leds_toggle(LEDS_RED);
}

/* ==========================================================
 *  Sink süreci
 * ==========================================================*/
PROCESS_THREAD(sink_node_process, ev, data)
{
  PROCESS_BEGIN();

  LOG_INFO("Sink node initialized — waiting for data...\n");

  /* UDP alıcıyı başlat */
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
