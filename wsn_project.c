#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "dev/leds.h"
#include "random.h"
#include "string.h"
#include <stdio.h> /* snprintf için gerekli */

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

  /* ----------------------------------------------------------------
     1. GÜVENLİK KONTROLÜ (Proposal: Attacks Detection)
     Eğer "TAMPER" mesajı gelirse, güvenlik bayrağını kaldır ve 
     Otomasyonu acil tetikle.
     ---------------------------------------------------------------- */
  if(datalen >= 6 && strncmp((const char *)data, "TAMPER", 6) == 0) {
    LOG_WARN("[CH] ⚠️ SECURITY ALERT: TAMPER command received from ");
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");

    /* 1. Otomasyon katmanına haber ver (Bayrağı kaldır) */
    security_alert_flag = 1;

    /* 2. Otomasyonu beklemeden hemen çalıştır (Acil Durum) */
    automation_check_conditions();

    /* 3. Simülasyon görseli için: Zinciri boz (Opsiyonel, test için) */
    blockchain_tamper_random_block();
    
    return; // Saldırı paketi havuza atılmaz, fonksiyon biter.
  }

  /* ----------------------------------------------------------------
     2. NORMAL VERİ İŞLEME (Proposal: Aggregation & Energy Saving)
     Veriyi hemen bloklama! Havuza (Buffer) at.
     ---------------------------------------------------------------- */
  int temp;
  memcpy(&temp, data, sizeof(temp));
  
  /* Gönderen Node ID'sini stringe çevir (Örn: "Node-52") */
  /* IPv6 adresinin son byte'ı genelde Node ID'dir */
  char sender_id[16];
  snprintf(sender_id, sizeof(sender_id), "Node-%u", sender_addr->u8[15]);

  LOG_INFO("Received Data: %d°C from %s. Buffering for Automation...\n", temp, sender_id);

  /* --- KRİTİK DEĞİŞİKLİK --- */
  /* Eski: blockchain_add_block(...) -> Kaldırıldı (Enerji israfı) */
  /* Yeni: Veriyi havuza ekle */
  blockchain_buffer_data(temp, sender_id);

  /* Otomasyon için işlem sayacını artır */
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