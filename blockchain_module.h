#ifndef BLOCKCHAIN_MODULE_H
#define BLOCKCHAIN_MODULE_H

#include "contiki.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "sys/log.h"
#include <stdint.h>

#define MAX_BLOCKS 50

/* ---------------------------------------
   Basit Blockchain Blok Yapısı
   --------------------------------------- */
typedef struct block {
  struct block *next;     // Zincirdeki bir sonraki blok
  char data[64];          // Sensör veya özetlenmiş veri
  char hash[33];          // SHA-256 veya basitleştirilmiş hash string
  uint32_t timestamp;     // Oluşturulma zamanı (clock_seconds)
} block_t;

/* ---------------------------------------
   Fonksiyon Prototipleri
   --------------------------------------- */

/**
 * @brief Blockchain sistemini başlatır.
 *        Bellek havuzu ve zincir listesini sıfırlar.
 */
void blockchain_init(void);

/**
 * @brief Yeni bir blok oluşturur ve zincire ekler.
 * @param data Blok verisi (ör. MerkleRoot veya sensör özeti)
 * @param timestamp Zaman damgası
 */
void blockchain_add_block(char *data, uint32_t timestamp);

/**
 * @brief Zinciri ekrana/loga basar (debug için).
 */
void blockchain_print_chain(void);

/**
 * @brief Yeni bir işlem geldiğinde çağrılır.
 *        Automation layer ile birlikte çalışır.
 */
void blockchain_add_transaction(int value);

#endif /* BLOCKCHAIN_MODULE_H */
