# === Proje ismi ===
CONTIKI_PROJECT = wsn_project sensor_node sink_node

# === Ek kaynak dosyalar (proje modülleri) ===
PROJECT_SOURCEFILES += blockchain_module.c automation_layer.c

# === Contiki kök dizini (Cooja için önemli) ===
CONTIKI = ../contiki-ng

# === Derleme hedefi (simülasyon ortamı) ===
TARGET ?= cooja

# === Derleme ===
all: $(CONTIKI_PROJECT)
include $(CONTIKI)/Makefile.include
