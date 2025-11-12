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
  char hash[65];          // SHA-256 hash (64 karakter + null)
  char prev_hash[65];     // Önceki bloğun hash değeri
  uint32_t timestamp;     // Oluşturulma zamanı
} block_t;

/* Global blockchain listesi */
extern list_t blockchain;

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
 * @param data Blok verisi (ör. sensör özeti veya MerkleRoot)
 * @param timestamp Zaman damgası
 */
void blockchain_add_block(char *data, uint32_t timestamp);

/**
 * @brief Zincir bütünlüğünü doğrular.
 */
void blockchain_verify_chain(void);

/**
 * @brief (Opsiyonel) Rastgele blok bozulmasını test eder.
 *        Varsayılan derlemede devre dışıdır.
 */
void blockchain_tamper_random_block(void);

#endif /* BLOCKCHAIN_MODULE_H */
