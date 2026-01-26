#ifndef AUTOMATION_LAYER_H
#define AUTOMATION_LAYER_H


#include "contiki.h"

/* 
 * Automation Layer — Lightweight decision logic
 * ------------------------------------------------
 * This module monitors WSN node status (energy, pending transactions)
 * and decides when to automatically trigger blockchain commits.
 */

/**
 * @brief Checks the network and energy conditions.
 *        Called periodically (e.g., every 10 seconds).
 */
void automation_check_conditions(void);

/* Automation layer ve CH tarafından okunacak güvenlik bayrağı */
extern uint8_t security_alert_flag;

/**
 * @brief Called when a new transaction arrives.
 *        Increases the transaction counter.
 */
void automation_new_transaction(void);

/* ===================== TRUST / ATTACKER IDENTIFICATION =====================
 * We keep a lightweight trust score per node (0..100).
 * - Trust decreases on suspicious events (replay / tamper / malformed packet)
 * - If trust stays very low for K consecutive bad rounds, node is blacklisted
 * ==========================================================================*/
#define TRUST_SCALE 100
#define TRUST_ALPHA 70  /* EMA: higher = more weight to history */

#define TRUST_PENALTY_REPLAY  40
#define TRUST_PENALTY_TAMPER  70
#define TRUST_PENALTY_INVALID 10

#define TRUST_SUSPICIOUS_THRESHOLD 40
#define TRUST_ELIMINATE_THRESHOLD 50
#define TRUST_ELIMINATE_STREAK    1

typedef enum {
  TRUST_EVENT_GOOD = 0,
  TRUST_EVENT_REPLAY = 1,
  TRUST_EVENT_TAMPER = 2,
  TRUST_EVENT_INVALID = 3
} trust_event_t;

void automation_trust_init(void);
uint8_t automation_trust_event(uint8_t node_id, trust_event_t ev);
uint8_t automation_is_blacklisted(uint8_t node_id);
uint8_t automation_get_trust(uint8_t node_id);

#endif /* AUTOMATION_LAYER_H */
