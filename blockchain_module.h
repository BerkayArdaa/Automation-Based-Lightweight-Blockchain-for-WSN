#ifndef BLOCKCHAIN_MODULE_H
#define BLOCKCHAIN_MODULE_H

#include "contiki.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "sys/log.h"
#include <stdint.h>

#define MAX_BLOCKS 50
#define MAX_BUFFER_SIZE 20

/* ---------------------------------------
   Basit Blockchain Blok Yapısı
   --------------------------------------- */
typedef struct block {
  struct block *next;     // Zincirdeki bir sonraki blok
  char data[64];          // Sensör veya özetlenmiş (aggregated) veri
  char hash[65];          // SHA-256 hash
  char prev_hash[65];     // Önceki bloğun hash değeri
  uint32_t timestamp;     // Oluşturulma zamanı
} block_t;

/* 
   HATA DÜZELTİLDİ: 
   'extern struct list blockchain;' satırı kaldırıldı.
   Liste, module.c içinde 'LIST(blockchain)' ile tanımlanıyor 
   ve dışarıdan doğrudan erişilmesine gerek yok.
*/

/* ---------------------------------------
   Global Değişkenler
   --------------------------------------- */
/* Otomasyon katmanının saldırıyı görmesi için bayrak */
extern uint8_t security_alert_flag;

/* ---------------------------------------
   Fonksiyon Prototipleri
   --------------------------------------- */

/**
 * @brief Blockchain sistemini başlatır.
 */
void blockchain_init(void);

/**
 * @brief (DÜŞÜK SEVİYE) Doğrudan blok oluşturur.
 *        Genellikle 'commit_batch' tarafından dahili kullanılır.
 */
void blockchain_add_block(char *data, uint32_t timestamp);

/**
 * @brief (YENİ) Veriyi hemen bloklamaz, havuza atar.
 *        Enerji tasarrufu ve veri birleştirme (Aggregation) sağlar.
 * @param temperature Sensör sıcaklık verisi
 * @param sender_id Gönderen düğüm bilgisi (ID veya IP string)
 */
void blockchain_buffer_data(int temperature, const char *sender_id);

/**
 * @brief (YENİ) Havuzdaki verileri birleştirir ve TEK BİR BLOK yapar.
 *        Merkle Root mantığının lightweight simülasyonudur.
 */
void blockchain_commit_batch(void);

/**
 * @brief Zincir bütünlüğünü doğrular.
 */
void blockchain_verify_chain(void);

/**
 * @brief Rastgele blok bozulmasını test eder (Saldırı simülasyonu).
 */
void blockchain_tamper_random_block(void);

#endif /* BLOCKCHAIN_MODULE_H */