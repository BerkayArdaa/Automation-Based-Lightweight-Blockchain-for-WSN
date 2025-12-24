#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "random.h"
#include "dev/leds.h"
#include <stdint.h>

#define LOG_MODULE "Sensor"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_PORT 1234

PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

static struct simple_udp_connection udp_conn;

typedef struct __attribute__((packed)) {
  uint32_t seq;
  int16_t temp;
} sensor_payload_t;

PROCESS_THREAD(sensor_node_process, ev, data)
{
  static struct etimer timer;
  static int temperature;

  static uint32_t g_seq = 0;

  // ✅ replay için son paketi sakla
  static sensor_payload_t last_p;
  static uint8_t has_last = 0;

  PROCESS_BEGIN();
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, NULL);

  while(1) {
    etimer_set(&timer, CLOCK_SECOND * (5 + random_rand() % 5));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    uip_ipaddr_t addr;
    uip_create_linklocal_allnodes_mcast(&addr);

    /* ✅ Her 6 pakette 1 replay yap */
    if(has_last && (g_seq % 6 == 0)) {
      LOG_WARN("[REPLAY] Re-sending old packet! Temp=%d°C | seq=%lu\n",
               (int)last_p.temp, (unsigned long)last_p.seq);

      simple_udp_sendto(&udp_conn, &last_p, sizeof(last_p), &addr);
      leds_toggle(LEDS_RED);
      continue; // bu turda yeni paket üretme
    }

    /* Normal paket */
    temperature = 20 + (random_rand() % 10);

    sensor_payload_t p;
    p.seq  = ++g_seq;
    p.temp = (int16_t)temperature;

    LOG_INFO("[🌡️ Sensor] Temp = %d°C | seq=%lu\n", temperature, (unsigned long)p.seq);

    simple_udp_sendto(&udp_conn, &p, sizeof(p), &addr);
    leds_toggle(LEDS_GREEN);

    // ✅ son paketi sakla
    last_p = p;
    has_last = 1;
  }

  PROCESS_END();
}
