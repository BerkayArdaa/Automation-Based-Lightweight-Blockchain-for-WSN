# === Proje İsmi (ana hedefler) ===
CONTIKI_PROJECT = wsn_project sensor_node sink_node

# === Ek kaynak dosyalar (yardımcı modüller) ===
PROJECT_SOURCEFILES += blockchain_module.c automation_layer.c

# === Contiki kök dizini (Cooja simülasyonu için gerekli) ===
CONTIKI = ../..

# === Derleme Hedefi ===
TARGET = cooja

# === Derleme İşlemi ===
all: $(CONTIKI_PROJECT)

include $(CONTIKI)/Makefile.include

