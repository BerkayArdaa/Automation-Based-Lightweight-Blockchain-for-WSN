#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "dev/leds.h"
#include "random.h"
#include "string.h"

#include "blockchain_module.h"
#include "automation_layer.h"

#define LOG_MODULE "WSN"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_PORT 1234

PROCESS(wsn_project_process, "WSN Project (Blockchain Cluster Head)");
AUTOSTART_PROCESSES(&wsn_project_process);

static struct simple_udp_connection udp_conn;

/* ==========================================================
 *  Gelen veriyi işleyen UDP callback fonksiyonu
 * ==========================================================*/
static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port,
                            const uint8_t *data,
                            uint16_t datalen) {

  int temp;
  memcpy(&temp, data, sizeof(temp));
  LOG_INFO("Received temperature: %d°C from ", temp);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");

  /* Veriyi stringe dönüştür */
  char data_str[32];
  snprintf(data_str, sizeof(data_str), "%d", temp);

  /* Blockchain'e yeni blok ekle */
  blockchain_add_block(data_str, clock_seconds());

  leds_toggle(LEDS_GREEN);
}

/* ==========================================================
 *  Ana süreç (Cluster Head)
 * ==========================================================*/
PROCESS_THREAD(wsn_project_process, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  LOG_INFO("[[B] Blockchain] Initializing blockchain module...\n");
  blockchain_init();

  /* UDP bağlantısını başlat */
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

  while(1) {
    /* Her 10 saniyede bir otomasyon kontrolü yap */
    etimer_set(&timer, CLOCK_SECOND * 10);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    LOG_INFO("[[A] Automation] Checking conditions...\n");
    automation_check_conditions();
  }

  PROCESS_END();
}
