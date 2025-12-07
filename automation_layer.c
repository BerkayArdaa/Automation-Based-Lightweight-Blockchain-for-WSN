#include "automation_layer.h"
#include "blockchain_module.h"
#include "sys/etimer.h"
#include "sys/log.h"
#include "random.h"
#include <stdio.h>

#define LOG_MODULE "A"
#define LOG_LEVEL LOG_LEVEL_INFO

static uint8_t pending_transactions = 0;
static uint8_t node_energy = 90; // Başlangıç enerjisi

void automation_check_conditions(void) {

  // Enerjiyi yavaşça düşür (Simülasyon)
  if(node_energy > 0) {
      node_energy -= 1 + (random_rand() % 3);
  }

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