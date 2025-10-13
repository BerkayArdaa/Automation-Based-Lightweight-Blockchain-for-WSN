#include "blockchain_module.h"
#include "contiki.h"
#include "lib/memb.h"
#include "lib/list.h"
#include <stdio.h>
#include <string.h>

MEMB(block_mem, block_t, MAX_BLOCKS);
LIST(blockchain);

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

  // basit hash
  snprintf(b->hash, sizeof(b->hash), "%lu", timestamp ^ strlen(data));
  list_add(blockchain, b);

  printf("[[TX] Blockchain] Block added: %s | Hash: %s\n", b->data, b->hash);
}
