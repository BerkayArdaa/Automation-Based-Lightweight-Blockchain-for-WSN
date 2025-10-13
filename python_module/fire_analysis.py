import re
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime
from blockchain_module import Blockchain

# 🔹 Log dosyası yolu (Cooja simülasyonunda kaydettiğin)
LOG_FILE = "../cooja_logs/wsn_output.log"

# 🔹 Eşik değer (örnek: 28°C üzeri = yangın riski)
FIRE_THRESHOLD = 28

# 🔹 Verileri tutmak için liste
data = []

# --- Logları oku ve sıcaklık verilerini ayıkla ---
with open(LOG_FILE, "r", encoding="utf-8") as f:
    for line in f:
        match = re.search(r"Node (\d+).*temperature: (\d+)", line)
        if match:
            node_id = int(match.group(1))
            temp = int(match.group(2))
            data.append({
                "Node": node_id,
                "Temperature": temp,
                "Timestamp": datetime.now()
            })

# --- DataFrame'e çevir ---
df = pd.DataFrame(data)
if df.empty:
    print("⚠️  Log dosyasında sıcaklık verisi bulunamadı.")
    exit()

# --- Yangın riski kontrolü ---
df["FireAlert"] = df["Temperature"] > FIRE_THRESHOLD
fire_nodes = df[df["FireAlert"] == True]
# Blockchain başlat
blockchain = Blockchain()

# Her veri kaydını zincire ekle
for _, row in df.iterrows():
    blockchain.add_block({
        "Node": int(row["Node"]),
        "Temperature": int(row["Temperature"]),
        "Timestamp": str(row["Timestamp"]),
        "FireAlert": bool(row["FireAlert"])
    })

print(f"\n📦 Blockchain contains {len(blockchain.chain)} blocks.")
print("🧩 Chain integrity:", "✅ Valid" if blockchain.is_chain_valid() else "❌ Corrupted")

# --- Konsola özet yazdır ---
print("\n🔥 FIRE DETECTION REPORT 🔥")
print("-" * 40)
print(df.groupby("Node")["Temperature"].mean().round(1).to_string())
if not fire_nodes.empty:
    print("\n🚨 ALERT: High temperature detected at nodes:")
    for node in fire_nodes["Node"].unique():
        print(f" - Node {node}")
else:
    print("\n✅ No fire detected — all temperatures within safe range.")

# --- Grafik oluştur ---
plt.figure(figsize=(6,4))
plt.title("Temperature Readings by Node")
for node, group in df.groupby("Node"):
    plt.plot(group["Timestamp"], group["Temperature"], label=f"Node {node}")

plt.axhline(FIRE_THRESHOLD, color="r", linestyle="--", label="Fire Threshold")
plt.xlabel("Time")
plt.ylabel("Temperature (°C)")
plt.legend()
plt.tight_layout()
plt.savefig("../results/fire_detection_plot.png")
plt.show()
# --- Blockchain'i JSON olarak kaydet ---
chain_data = []
for block in blockchain.chain:
    chain_data.append({
        "index": block.index,
        "timestamp": block.timestamp,
        "data": block.data,
        "previous_hash": block.previous_hash,
        "hash": block.hash
    })

with open("../results/chain_log.json", "w", encoding="utf-8") as f:
    json.dump(chain_data, f, indent=4)

print("💾 Blockchain saved to ../results/chain_log.json")
