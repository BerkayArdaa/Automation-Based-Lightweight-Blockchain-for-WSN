#include "blockchain_module.h"
#include "contiki.h"
#include "lib/memb.h"
#include "lib/list.h"
#include <stdio.h>
#include <string.h>
#include "random.h"
#include <stdint.h>

/* --- Liste ve Bellek Tanımları --- */
LIST(blockchain);
MEMB(block_mem, block_t, MAX_BLOCKS);
static void compute_sha256(const char *input, char *output);

static uint8_t block_count = 0;


/* --- YENİ: Buffer (Havuz) Yapısı ve Global Değişkenler --- */
/* Verileri anında hashlemek yerine burada biriktireceğiz (Energy Saving) */
typedef struct {
  int temp;
  char sender[10];
  char leaf_hash[65]; // SHA-256(temp|sender|ts)
} data_buffer_t;


static data_buffer_t buffer[MAX_BUFFER_SIZE];
static int buffer_count = 0;
static void compute_merkle_root(char out[65]) {
  if(buffer_count <= 0) {
    snprintf(out, 65, "0");
    return;
  }

  char level[MAX_BUFFER_SIZE][65];
  int n = buffer_count;

  for(int i = 0; i < n; i++) {
    snprintf(level[i], sizeof(level[i]), "%s", buffer[i].leaf_hash);
  }

  while(n > 1) {
    int next_n = (n + 1) / 2;
    char next_level[MAX_BUFFER_SIZE][65];

    for(int i = 0; i < next_n; i++) {
      char concat[130];
      const char *left  = level[i * 2];
      const char *right = (i * 2 + 1 < n) ? level[i * 2 + 1] : level[i * 2]; // tekse duplicate
      snprintf(concat, sizeof(concat), "%s%s", left, right);
      compute_sha256(concat, next_level[i]);
    }

    for(int i = 0; i < next_n; i++) {
      snprintf(level[i], sizeof(level[i]), "%s", next_level[i]);
    }
    n = next_n;
  }

  snprintf(out, 65, "%s", level[0]);
}

/* Otomasyon katmanının okuyacağı güvenlik bayrağı */
uint8_t security_alert_flag = 0;

/* ===========================================================
   Basit SHA-256 implementasyonu (Aynen korundu)
   =========================================================== */
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static const uint32_t k[64] = {
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
  0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
  0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
  0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
  0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
  0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
  0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
  0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void compute_sha256(const char *input, char *output) {
  uint32_t h[8] = {
    0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
    0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
  };

  uint8_t data[64];
  uint32_t datalen = 0;
  uint64_t bitlen = 0;
  size_t i, j;

  size_t len = strlen(input);
  for(i = 0; i < len; ++i) {
    data[datalen++] = (uint8_t)input[i];
    if(datalen == 64) {
      uint32_t m[64];
      for(j=0;j<16;++j)
        m[j]=(data[j*4]<<24)|(data[j*4+1]<<16)|(data[j*4+2]<<8)|(data[j*4+3]);
      for(;j<64;++j)
        m[j]=SIG1(m[j-2])+m[j-7]+SIG0(m[j-15])+m[j-16];
      uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hv=h[7];
      for(j=0;j<64;++j){
        uint32_t t1=hv+EP1(e)+CH(e,f,g)+k[j]+m[j];
        uint32_t t2=EP0(a)+MAJ(a,b,c);
        hv=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
      }
      h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hv;
      bitlen+=512; datalen=0;
    }
  }

  data[datalen++] = 0x80;
  while(datalen < 56) data[datalen++] = 0x00;
  bitlen += (uint64_t)len * 8ULL;

  data[56] = (uint8_t)((bitlen >> 56) & 0xFF);
  data[57] = (uint8_t)((bitlen >> 48) & 0xFF);
  data[58] = (uint8_t)((bitlen >> 40) & 0xFF);
  data[59] = (uint8_t)((bitlen >> 32) & 0xFF);
  data[60] = (uint8_t)((bitlen >> 24) & 0xFF);
  data[61] = (uint8_t)((bitlen >> 16) & 0xFF);
  data[62] = (uint8_t)((bitlen >> 8) & 0xFF);
  data[63] = (uint8_t)(bitlen & 0xFF);

  uint32_t m[64];
  for(j=0;j<16;++j)
    m[j]=(data[j*4]<<24)|(data[j*4+1]<<16)|(data[j*4+2]<<8)|(data[j*4+3]);
  for(;j<64;++j)
    m[j]=SIG1(m[j-2])+m[j-7]+SIG0(m[j-15])+m[j-16];
  uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hv=h[7];
  for(j=0;j<64;++j){
    uint32_t t1=hv+EP1(e)+CH(e,f,g)+k[j]+m[j];
    uint32_t t2=EP0(a)+MAJ(a,b,c);
    hv=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
  }
  h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hv;

  for(i = 0; i < 8; ++i)
    sprintf(output + (i * 8), "%08x", h[i]);
  output[64] = '\0';
}

/* ===========================================================
   Blockchain Fonksiyonları
   =========================================================== */

void blockchain_init(void) {
  memb_init(&block_mem);
  list_init(blockchain);
  
  // Buffer ve güvenlik bayrağını sıfırla
  buffer_count = 0;
  security_alert_flag = 0;
  
  printf("[[B] Blockchain] Initialized (Aggregation Mode)\n");
}

/* --- YENİ: Veriyi havuza atma fonksiyonu (Energy Saving) --- */
void blockchain_buffer_data(int temperature, const char *sender_id) {
  if(buffer_count < MAX_BUFFER_SIZE) {
    buffer[buffer_count].temp = temperature;

    strncpy(buffer[buffer_count].sender, sender_id, 9);
    buffer[buffer_count].sender[9] = '\0';

    char leaf_in[64];
    snprintf(leaf_in, sizeof(leaf_in), "%d|%s|%lu",
             temperature, buffer[buffer_count].sender, (unsigned long)clock_seconds());
    compute_sha256(leaf_in, buffer[buffer_count].leaf_hash);

    buffer_count++;
  } else {
    printf("[[B] Buffer Full!] Dropping data to save memory.\n");
  }
}


/* --- YENİ: Toplu Bloklama (Aggregation / Merkle Root Simulation) --- */
void blockchain_commit_batch() {
  if(buffer_count == 0) {
    printf("No data to commit.\n");
    return;
  }

  // 1) Aggregated data hazırla
  char aggregated_data[64] = {0};

  for(int i = 0; i < buffer_count; i++) {
    char temp_str[8];
    snprintf(temp_str, sizeof(temp_str), "%d|", buffer[i].temp);
    strncat(aggregated_data, temp_str,
            sizeof(aggregated_data) - strlen(aggregated_data) - 1);
  }

  // 2) Merkle root'u HESAPLA (buffer'ı temizlemeden önce!)
  char merkle_root[65];
  compute_merkle_root(merkle_root);

  // 3) Bu batch'te kaç tx var? (buffer_count reset olmadan yakala)
  uint8_t tx_count = (uint8_t)buffer_count;

  // 4) Yeni imzaya göre block ekle
  blockchain_add_block(aggregated_data, merkle_root, tx_count, clock_seconds());

  // 5) Buffer'ı temizle
  buffer_count = 0;
}


void blockchain_add_block(char *data, const char *merkle_root, uint8_t tx_count, uint32_t timestamp)
 {
 if(block_count >= MAX_BLOCKS) {
  block_t *old = list_head(blockchain);
  if(old != NULL) {
    list_remove(blockchain, old);
    memb_free(&block_mem, old);
    block_count--;
    printf("[[B] GC] Oldest block freed to keep memory bounded.\n");
  }
}


  block_t *b = memb_alloc(&block_mem);
  
  
  // Bellek doluysa en eski bloğu (genesis hariç) silip yer açmayı deneyebiliriz
  // Şimdilik sadece hata verip çıkıyoruz.
  if(b == NULL) {
    printf("[[B] Error] Memory full, cannot add block!\n");
    return;
  }

  /* --------------------------------------------------------
   * FIX: Initialize block fields before hashing.
   * Previously, b->data and b->timestamp were uninitialized
   * but were used in the hash input, causing non-deterministic
   * hashes and false "tamper" detections during verification.
   * -------------------------------------------------------- */
  snprintf(b->data, sizeof(b->data), "%s", (data != NULL) ? data : "");
  b->timestamp = timestamp;

  snprintf(b->merkle_root, sizeof(b->merkle_root), "%s",
           (merkle_root != NULL) ? merkle_root : "0");
  b->tx_count = tx_count;


  block_t *last_block = list_tail(blockchain);
  if(last_block != NULL) {
    strncpy(b->prev_hash, last_block->hash, sizeof(b->prev_hash));
  } else {
    strncpy(b->prev_hash, "GENESIS", sizeof(b->prev_hash));
  }

  char combined[256];
  snprintf(combined, sizeof(combined), "%s%s%s%u%lu",
           b->data,
           b->merkle_root,
           b->prev_hash,
           (unsigned)b->tx_count,
           (unsigned long)b->timestamp);

  compute_sha256(combined, b->hash);

  list_add(blockchain, b);
 block_count++;
  printf("[[TX] Blockchain] Block added: %s | ts=%lu | tx=%u | merkle=%s | hash=%s\n",
         b->data, (unsigned long)b->timestamp, b->tx_count, b->merkle_root, b->hash);

}

void blockchain_verify_chain(void) {
  block_t *b = list_head(blockchain);
  block_t *prev = NULL;
  char recomputed_hash[65];
  char combined[256];

  printf("=== Blockchain Verification ===\n");
  while(b != NULL) {
    snprintf(combined, sizeof(combined), "%s%s%s%u%lu",
         b->data,
         b->merkle_root,
         b->prev_hash,
         (unsigned)b->tx_count,
         (unsigned long)b->timestamp);

    compute_sha256(combined, recomputed_hash);

    if(strcmp(b->hash, recomputed_hash) != 0) {
      printf("[ERROR] Block tampered! Data: %s | Hash: %s | Recomputed: %s\n",
             b->data, b->hash, recomputed_hash);
      return;
    }
    if(prev != NULL && strcmp(b->prev_hash, prev->hash) != 0) {
      printf("[ERROR] Chain broken between blocks! prev_hash=%s expected=%s\n",
             b->prev_hash, prev->hash);
      return;
    }
    prev = b;
    b = b->next;
  }
  printf("[OK] Blockchain verified successfully.\n");
}

/* -----------------------------------------------------------
   Tamper (Saldırı Simülasyonu) Fonksiyonu
   ----------------------------------------------------------- */
#ifdef ENABLE_TAMPER
void blockchain_tamper_random_block(void) {
  int count = 0;
  block_t *b;
  for(b = list_head(blockchain); b != NULL; b = b->next) count++;
  if(count == 0) {
    printf("[TAMPER] No blocks to tamper.\n");
    return;
  }

  int idx = random_rand() % count;
  int i = 0;
  for(b = list_head(blockchain); b != NULL; b = b->next) {
    if(i == idx) {
      size_t len = strlen(b->data);
      if(len == 0) {
        strncpy(b->data, "TAMPERED", sizeof(b->data)-1);
        b->data[sizeof(b->data)-1] = '\0';
      } else {
        int pos = random_rand() % len;
        b->data[pos] = b->data[pos] ^ 0x55;
      }
      printf("[TAMPER] Tampered block index=%d data=%s\n", idx, b->data);
      return;
    }
    i++;
  }
}
#else
void blockchain_tamper_random_block(void) {
  /* Güvenlik nedeniyle devre dışı */
  printf("[TAMPER] Disabled in this build.\n");
}
#endif