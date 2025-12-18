#include "automation_layer.h"
#include "blockchain_module.h"
#include "sys/etimer.h"
#include "sys/log.h"
#include "random.h"
#include <stdio.h>
#include "sys/energest.h"
#include "sys/rtimer.h"

#define LOG_MODULE "A"
#define LOG_LEVEL LOG_LEVEL_INFO

static uint8_t pending_transactions = 0;
static uint8_t node_energy = 90; // Başlangıç enerjisi

void automation_check_conditions(void) {
  if(security_alert_flag == 1) {
  LOG_WARN("[⚠️ Automation] SECURITY ALERT DETECTED! Forcing immediate batch commit...\n");

  blockchain_commit_batch();

  /* buffer sayacı/tx reset (sende nasıl tutuyorsan) */
  pending_transactions = 0;

  security_alert_flag = 0;
  return;
}

    /* =========================================================
   * REAL-TIME ENERGY (Energest proxy)
   * Not: Bu fiziksel batarya modeli değil, Energest sayaçlarından
   * türetilmiş göreli enerji/airtime skoru.
   * ========================================================= */
  static uint32_t last_cpu = 0, last_lpm = 0, last_tx = 0, last_rx = 0;

  energest_flush();
  uint32_t cpu = energest_type_time(ENERGEST_TYPE_CPU);
  uint32_t lpm = energest_type_time(ENERGEST_TYPE_LPM);
  uint32_t tx  = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  uint32_t rx  = energest_type_time(ENERGEST_TYPE_LISTEN);

  uint32_t d_cpu = cpu - last_cpu;
  uint32_t d_lpm = lpm - last_lpm;
  uint32_t d_tx  = tx  - last_tx;
  uint32_t d_rx  = rx  - last_rx;

  last_cpu = cpu; last_lpm = lpm; last_tx = tx; last_rx = rx;

    /* Basit maliyet: TX/RX pahalı, CPU orta, LPM ucuz */
  uint32_t cost = 0;

  cost += (d_tx + d_rx) / (RTIMER_SECOND / 8  + 1);
  cost += (d_cpu)      / (RTIMER_SECOND / 16 + 1);
  cost += (d_lpm)      / (RTIMER_SECOND / 64 + 1);

  /* 🔥 Ölçekleme: yoksa ilk ölçümde E=0 olur */
  cost = cost / 50;
  if(cost == 0) cost = 1;


    LOG_INFO("[A][EnergyRaw] d_cpu=%lu d_lpm=%lu d_tx=%lu d_rx=%lu | cost_raw=%lu\n",
           (unsigned long)d_cpu, (unsigned long)d_lpm,
           (unsigned long)d_tx, (unsigned long)d_rx,
           (unsigned long)cost);





  /* ----------------------------------------------------------------
     1. GÜVENLİK KONTROLÜ (Possible Attacks)
     Eğer blockchain modülünden veya callback'ten bir saldırı uyarısı
     geldiyse, enerjiye bakmaksızın ACİL blok oluştur.
     ---------------------------------------------------------------- */
  if(security_alert_flag == 1) {
    LOG_WARN("[⚠️ Automation] SECURITY ALERT DETECTED! Forcing immediate block creation...\n");
    
    // Havuzdaki (Buffer) tüm verileri hemen blokla ve güvene al
    blockchain_commit_batch();
    
    // Bayrağı ve sayacı sıfırla
    security_alert_flag = 0;
    pending_transactions = 0;
    return; // Acil işlem yapıldığı için normal akışa devam etme
  }

  /* ----------------------------------------------------------------
     2. NORMAL KOŞULLAR (Energy Saving & Aggregation)
     Saldırı yoksa, verilerin birikmesini bekle (Batching).
     Eşik değerini 4 yaptık ki "Aggregation" mantığı çalışsın.
     ---------------------------------------------------------------- */
  if(node_energy > 20 && pending_transactions >= 4) {
    LOG_INFO("[⚙️ Automation] Optimization Conditions Met (Tx=%u, Energy=%u). Committing Batch...\n", 
             pending_transactions, node_energy);
    
    // Havuzdaki verileri birleştirip TEK BİR BLOK yap
    blockchain_commit_batch();
    
    pending_transactions = 0;
  } 
  else {
    // Veriler havuzda (buffer) beklemeye devam ediyor -> Enerji Tasarrufu
    LOG_INFO("[⚙️ Automation] Buffering Data... (Waiting: Tx=%u, Energy=%u)\n",
             pending_transactions, node_energy);
  }

  /* ----------------------------------------------------------------
     3. DÜŞÜK ENERJİ UYARISI
     ---------------------------------------------------------------- */
  if(node_energy <= 5) {
    LOG_WARN("[⚠️ Automation] Critical Low Energy! System performance degraded.\n");
  }
}

void automation_new_transaction(void) {
  pending_transactions++;
  // Log mesajını güncelledik: Artık "Buffering" yapıldığını belirtiyor
  LOG_INFO("[A] Data buffered for automation (Buffer Count=%u)\n", pending_transactions);
}