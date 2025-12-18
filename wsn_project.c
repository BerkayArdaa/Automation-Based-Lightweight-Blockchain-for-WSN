#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "dev/leds.h"
#include "random.h"
#include "string.h"
#include <stdio.h> /* snprintf için gerekli */

#include "blockchain_module.h"
#include "automation_layer.h"
#include <stdint.h>

typedef struct __attribute__((packed)) {
  uint32_t seq;
  int16_t temp;
} sensor_payload_t;

/* Node başına son seq (0..255) */
static uint32_t last_seq[256];


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
                            uint16_t datalen)
{
  /* 1) Manual saldırı testi (TAMPER komutu) - sende vardı, kalsın */
  if(datalen >= 6 && strncmp((const char *)data, "TAMPER", 6) == 0) {
    LOG_WARN("[CH] ⚠️ SECURITY ALERT: TAMPER command received from ");
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");

    security_alert_flag = 1;
    automation_check_conditions();
    blockchain_tamper_random_block();   // test amaçlı

    return;
  }

  /* 2) Normal sensör paketi: seq + temp bekliyoruz */
  if(datalen < sizeof(sensor_payload_t)) {
    LOG_WARN("[CH] Invalid payload size=%u (expected=%u) from ",
             datalen, (unsigned)sizeof(sensor_payload_t));
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
    return;
  }

  sensor_payload_t p;
  memcpy(&p, data, sizeof(p));

  /* Node ID: IPv6 son byte (sen de bunu kullanıyorsun) */
  uint8_t nid = sender_addr->u8[15];

  /* 3) REPLAY DETECTION */
  if(last_seq[nid] != 0 && p.seq <= last_seq[nid]) {
    LOG_WARN("[A][SECURITY] Replay detected from Node-%u (seq=%lu last=%lu). Forcing security alert.\n",
             nid, (unsigned long)p.seq, (unsigned long)last_seq[nid]);

    security_alert_flag = 1;

    /* acil commit istiyorsan: */
    automation_check_conditions();

    /* replay paketi buffer’a girmez */
    return;
  }

  /* geçerli paket → son seq güncelle */
  last_seq[nid] = p.seq;

  /* 4) Buffer + automation (mevcut akışın) */
  char sender_id[16];
  snprintf(sender_id, sizeof(sender_id), "Node-%u", nid);

  int temp = (int)p.temp;

  LOG_INFO("Received Data: %d°C (seq=%lu) from %s. Buffering for Automation...\n",
           temp, (unsigned long)p.seq, sender_id);

  blockchain_buffer_data(temp, sender_id);
  automation_new_transaction();

  leds_toggle(LEDS_GREEN);
}

/* ==========================================================
 *  Ana süreç (Cluster Head)
 * ==========================================================*/
PROCESS_THREAD(wsn_project_process, ev, data)
{
  static struct etimer timer;
  static struct etimer verify_timer; // Doğrulama için ikinci zamanlayıcı

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

    /* 10 saniyede bir otomasyon kontrolü (Real-time conditions check) */
    if(etimer_expired(&timer)) {
      /* Log kirliliğini önlemek için sadece işlem varsa log basılabilir,
         ama şimdilik akışı görmek için bırakıyoruz. */
      // LOG_INFO("[[A] Automation] Checking conditions...\n");
      automation_check_conditions();
      etimer_reset(&timer);
    }

    /* 30 saniyede bir blockchain doğrulama */
    if(etimer_expired(&verify_timer)) {
      /* Doğrulama sadece zincirde blok varsa anlamlıdır */
      // LOG_INFO("[[B] Blockchain] Periodic verification running...\n");
      blockchain_verify_chain();
      etimer_reset(&verify_timer);
    }
  }

  PROCESS_END();
}