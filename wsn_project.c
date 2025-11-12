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

  /* 1️⃣ Öncelikle gelen verinin "TAMPER" olup olmadığını kontrol et */
  if(datalen >= 6 && strncmp((const char *)data, "TAMPER", 6) == 0) {
    LOG_WARN("[CH] Received TAMPER command from ");
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");

    /* Simüle edilmiş saldırı: zinciri boz */
    blockchain_tamper_random_block();
    return; // saldırı tespit edildi, normal veri işleme yapılmaz
  }

  /* 2️⃣ Eğer normal sıcaklık verisiyse işleme devam et */
  int temp;
  memcpy(&temp, data, sizeof(temp));
  LOG_INFO("Received temperature: %d°C from ", temp);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");

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
  static struct etimer verify_timer; // doğrulama için ikinci zamanlayıcı

  PROCESS_BEGIN();

  LOG_INFO("[[B] Blockchain] Initializing blockchain module...\n");
  blockchain_init();

  /* UDP bağlantısını başlat */
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, udp_rx_callback);

  /* Başlangıç zamanlayıcıları */
  etimer_set(&timer, CLOCK_SECOND * 10);
  etimer_set(&verify_timer, CLOCK_SECOND * 30);

  while(1) {
    PROCESS_WAIT_EVENT();

    /* 10 saniyede bir otomasyon kontrolü */
    if(etimer_expired(&timer)) {
      LOG_INFO("[[A] Automation] Checking conditions...\n");
      automation_check_conditions();
      etimer_reset(&timer);
    }

    /* 30 saniyede bir blockchain doğrulama */
    if(etimer_expired(&verify_timer)) {
      LOG_INFO("[[B] Blockchain] Periodic verification running...\n");
      blockchain_verify_chain();
      etimer_reset(&verify_timer);
    }
  }

  PROCESS_END();
}
