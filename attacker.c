#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "dev/leds.h"
#include "dev/button-hal.h"

#include <string.h>
#include <stdint.h>

#define LOG_MODULE "Attacker"
#define LOG_LEVEL LOG_LEVEL_INFO
#define UDP_PORT 1234

PROCESS(attacker_button_process, "Attacker (Button-Triggered TAMPER)");
AUTOSTART_PROCESSES(&attacker_button_process);

static struct simple_udp_connection udp_conn;

PROCESS_THREAD(attacker_button_process, ev, data)
{
  PROCESS_BEGIN();

  /* UDP hazırla */
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, NULL);

  LOG_INFO("Attacker ready.\n");
  LOG_INFO("Press BUTTON to send 'TAMPER' to ALL-NODES (link-local multicast)\n");

  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == button_hal_press_event) {
      const char msg[] = "TAMPER";

      /* ✅ Değişiklik: all-nodes multicast hedef */
      uip_ipaddr_t mcast_addr;
      uip_create_linklocal_allnodes_mcast(&mcast_addr);

      simple_udp_sendto(&udp_conn, msg, sizeof(msg) - 1, &mcast_addr);

      LOG_WARN("[MANUAL ATTACK] Sent '%s' to ", msg);
      LOG_INFO_6ADDR(&mcast_addr);
      LOG_INFO_("\n");

      leds_toggle(LEDS_RED);
    }
  }

  PROCESS_END();
}
