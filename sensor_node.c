#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "random.h"
#include "dev/leds.h"
#include <stdint.h>   // <-- EKLE

#define LOG_MODULE "Sensor"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_PORT 1234

PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

static struct simple_udp_connection udp_conn;

/* 2) Payload: seq + temp */
typedef struct __attribute__((packed)) {
  uint32_t seq;
  int16_t temp;
} sensor_payload_t;

PROCESS_THREAD(sensor_node_process, ev, data)
{
  static struct etimer timer;
  static int temperature;

  /* 2) Her sensörde artan seq */
  static uint32_t g_seq = 0;

  PROCESS_BEGIN();
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, NULL);

  while(1) {
    etimer_set(&timer, CLOCK_SECOND * (5 + random_rand() % 5));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    temperature = 20 + (random_rand() % 10);

    sensor_payload_t p;
    p.seq  = ++g_seq;
    p.temp = (int16_t)temperature;

    LOG_INFO("[🌡️ Sensor] Temp = %d°C | seq=%lu\n", temperature, (unsigned long)p.seq);

    uip_ipaddr_t addr;
    uip_create_linklocal_allnodes_mcast(&addr);

    /* Artık int değil, struct yolluyoruz */
    simple_udp_sendto(&udp_conn, &p, sizeof(p), &addr);

    leds_toggle(LEDS_GREEN);
  }

  PROCESS_END();
}
