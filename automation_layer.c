#include "automation_layer.h"
#include "blockchain_module.h"
#include "sys/etimer.h"
#include "sys/log.h"
#include "random.h"
#include <stdio.h>

#define LOG_MODULE "A"
#define LOG_LEVEL LOG_LEVEL_INFO

static uint8_t pending_transactions = 0;
static uint8_t node_energy = 90; // başlangıç enerjisi

void automation_check_conditions(void) {

  // Enerjiyi yavaşça düşür (örnek olarak)
  node_energy -= 1 + (random_rand() % 3);

  if(node_energy > 20 && pending_transactions > 2) {
    LOG_INFO("[⚙️ Automation] Creating new block automatically...\n");
    blockchain_add_block("Aggregated data block", clock_seconds());
    pending_transactions = 0;
  } else {
    LOG_INFO("[⚙️ Automation] Waiting (energy=%u, tx=%u)\n",
             node_energy, pending_transactions);
  }

  if(node_energy <= 5) {
    LOG_WARN("[⚠️ Automation] Low energy, skipping block creation!\n");
  }
}

void automation_new_transaction(void) {
  pending_transactions++;
  LOG_INFO("[A] New transaction detected (total=%u)\n", pending_transactions);
}
