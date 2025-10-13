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

/**
 * @brief Called when a new transaction arrives.
 *        Increases the transaction counter.
 */
void automation_new_transaction(void);

#endif /* AUTOMATION_LAYER_H */
