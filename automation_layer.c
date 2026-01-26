#include "automation_layer.h"
#include "blockchain_module.h"
#include "sys/etimer.h"
#include "sys/log.h"
#include "random.h"
#include <stdio.h>
#include <stdint.h>
#include "sys/energest.h"
#include "sys/rtimer.h"

#define LOG_MODULE "A"
#define LOG_LEVEL LOG_LEVEL_INFO

static uint8_t pending_transactions = 0;
static uint8_t node_energy = 90; // Başlangıç enerjisi

/* ===================== TRUST TABLES (Attacker Identification) ===================== */
static uint8_t trust_score[256];
static uint8_t bad_streak[256];
static uint8_t blacklisted[256];
static uint8_t trust_inited = 0;

void automation_trust_init(void) {
  if(trust_inited) return;
  for(int i = 0; i < 256; i++) {
    trust_score[i] = TRUST_SCALE;  /* start trusted */
    bad_streak[i] = 0;
    blacklisted[i] = 0;
  }
  trust_inited = 1;
  LOG_INFO("[Trust] initialized (score=100 for all nodes)\n");
}

uint8_t automation_is_blacklisted(uint8_t node_id) {
  return blacklisted[node_id] ? 1 : 0;
}

uint8_t automation_get_trust(uint8_t node_id) {
  return trust_score[node_id];
}

static uint8_t penalty_from_event(trust_event_t ev) {
  switch(ev) {
    case TRUST_EVENT_REPLAY:  return TRUST_PENALTY_REPLAY;
    case TRUST_EVENT_TAMPER:  return TRUST_PENALTY_TAMPER;
    case TRUST_EVENT_INVALID: return TRUST_PENALTY_INVALID;
    case TRUST_EVENT_GOOD:
    default: return 0;
  }
}

uint8_t automation_trust_event(uint8_t node_id, trust_event_t ev) {
  if(!trust_inited) automation_trust_init();

  if(blacklisted[node_id]) {
    return 0; /* already eliminated */
  }

  uint8_t pen = penalty_from_event(ev);
  uint8_t behavior = (pen >= TRUST_SCALE) ? 0 : (uint8_t)(TRUST_SCALE - pen);

  uint16_t updated = (uint16_t)TRUST_ALPHA * (uint16_t)trust_score[node_id]
                   + (uint16_t)(TRUST_SCALE - TRUST_ALPHA) * (uint16_t)behavior;
  trust_score[node_id] = (uint8_t)(updated / TRUST_SCALE);

  if(ev == TRUST_EVENT_GOOD) {
    bad_streak[node_id] = 0;
  } else {
    if(trust_score[node_id] <= TRUST_ELIMINATE_THRESHOLD) {
      if(bad_streak[node_id] < 255) bad_streak[node_id]++;
    } else {
      if(bad_streak[node_id] > 0) bad_streak[node_id]--;
    }

    if(trust_score[node_id] <= TRUST_SUSPICIOUS_THRESHOLD) {
      security_alert_flag = 1;
    }
  }

  if(bad_streak[node_id] >= TRUST_ELIMINATE_STREAK) {
    blacklisted[node_id] = 1;
    security_alert_flag = 1;
    LOG_WARN("[Trust] Node-%u BLACKLISTED (trust=%u, streak=%u)\n",
             node_id, trust_score[node_id], bad_streak[node_id]);
    return 1;
  }

  LOG_INFO("[Trust] Node-%u ev=%u → trust=%u (streak=%u)\n",
           node_id, (unsigned)ev, trust_score[node_id], bad_streak[node_id]);
  return 0;
}
/* ================================================================================ */


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