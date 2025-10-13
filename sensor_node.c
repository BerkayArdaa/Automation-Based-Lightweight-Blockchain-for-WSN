#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "random.h"
#include "dev/leds.h"
#include "automation_layer.h"

#define LOG_MODULE "Sensor"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_PORT 1234

PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

static struct simple_udp_connection udp_conn;

PROCESS_THREAD(sensor_node_process, ev, data)
{
  static struct etimer timer;
  static int temperature;

  PROCESS_BEGIN();
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, NULL);

  while(1) {
    etimer_set(&timer, CLOCK_SECOND * (5 + random_rand() % 5));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    temperature = 20 + (random_rand() % 10);
    LOG_INFO("[🌡️ Sensor] Temp = %d°C\n", temperature);

    uip_ipaddr_t addr;
    uip_create_linklocal_allnodes_mcast(&addr);
    simple_udp_sendto(&udp_conn, &temperature, sizeof(temperature), &addr);

    leds_toggle(LEDS_GREEN);
    automation_new_transaction(); // Blockchain sistemine gönderim bilgisi
  }

  PROCESS_END();
}
