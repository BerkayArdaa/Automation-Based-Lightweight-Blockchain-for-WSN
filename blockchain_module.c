#include "blockchain_module.h"
#include "contiki.h"
#include "lib/memb.h"
#include "lib/list.h"
#include <stdio.h>
#include <string.h>
#include "random.h"
#include <stdint.h>

/* --- Liste ve bellek tanımları en üste taşındı --- */
LIST(blockchain);   // ✔ doğru
MEMB(block_mem, block_t, MAX_BLOCKS);

/* ===========================================================
   Basit SHA-256 implementasyonu (harici kütüphane gerektirmez)
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

  /* high bits */
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

void blockchain_init(void) {
  memb_init(&block_mem);
  list_init(blockchain);
  printf("[[B] Blockchain] Initialized\n");
}

void blockchain_add_block(char *data, uint32_t timestamp) {
  block_t *b = memb_alloc(&block_mem);
  if(b == NULL) return;

  snprintf(b->data, sizeof(b->data), "%s", data);
  b->timestamp = timestamp;

  block_t *last_block = list_tail(blockchain);
  if(last_block != NULL) {
    strncpy(b->prev_hash, last_block->hash, sizeof(b->prev_hash));
  } else {
    strncpy(b->prev_hash, "GENESIS", sizeof(b->prev_hash));
  }

  char combined[256];
  snprintf(combined, sizeof(combined), "%s%s%lu",
           b->data, b->prev_hash, (unsigned long)b->timestamp);

  compute_sha256(combined, b->hash);
  list_add(blockchain, b);

  printf("[[TX] Blockchain] Block added: %s | Hash: %s\n", b->data, b->hash);
}

void blockchain_verify_chain(void) {
  block_t *b = list_head(blockchain);
  block_t *prev = NULL;
  char recomputed_hash[65];
  char combined[256];

  printf("=== Blockchain Verification ===\n");
  while(b != NULL) {
    snprintf(combined, sizeof(combined), "%s%s%lu",
             b->data, b->prev_hash, (unsigned long)b->timestamp);
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
   Tamper fonksiyonu devre dışı bırakıldı (varsayılan temiz davranış).
   Eğer test amaçlı rastgele tahribat yapmak istersen, derlemeye
   ENABLE_TAMPER tanımı ile ekleyebilirsin:
     make CFLAGS="-DENABLE_TAMPER" TARGET=cooja
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
  /* Güvenlik nedeniyle devre dışı — üretim/sınama dışında kullanılmaz. */
  printf("[TAMPER] Disabled in this build.\n");
}
#endif
