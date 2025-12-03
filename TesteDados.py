
import requests
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

# ====== CONFIGURAÇÕES ======
CHANNEL_ID = "3183235"          # ex: "1234567"
READ_API_KEY = "S6RV5KFA0618S08Z"      # se o canal for privado
RESULTS = 60                           # últimos 60 pontos (≈ última hora)

# Se o canal for público, você pode tirar o api_key=...
url = f"https://api.thingspeak.com/channels/{CHANNEL_ID}/feeds.json"
params = {
    "api_key": READ_API_KEY,  # se for público, pode comentar essa linha
    "results": RESULTS
}

# ====== REQUISIÇÃO ======
resp = requests.get(url, params=params)
resp.raise_for_status()
data = resp.json()

feeds = data["feeds"]
df = pd.DataFrame(feeds)

# ====== TRATAMENTO DOS DADOS ======
# Converte tempo
df["created_at"] = pd.to_datetime(df["created_at"])

# Se quiser converter pra horário de Brasília:
df["created_at"] = df["created_at"].dt.tz_convert("America/Sao_Paulo")

# Converte fields para float (podem vir como string)
for col in ["field1", "field2", "field3", "field4"]:
    df[col] = pd.to_numeric(df[col], errors="coerce")

# Renomeia colunas pra ficar mais legível
df = df.rename(columns={
    "created_at": "time",
    "field1": "temp",
    "field2": "umid",
    "field3": "press",
    "field4": "alt"
})

# Remove linhas totalmente vazias, se tiver
df = df.dropna(subset=["temp", "umid", "press", "alt"])

# ====== PLOT ======
plt.figure(figsize=(12, 8))

# Formatador de horário no eixo x
time_fmt = mdates.DateFormatter("%H:%M")

# --- Temperatura ---
ax1 = plt.subplot(2, 2, 1)
ax1.plot(df["time"], df["temp"], marker="o")
ax1.set_title("Temperatura (°C)")
ax1.set_xlabel("Horário")
ax1.set_ylabel("°C")
ax1.xaxis.set_major_formatter(time_fmt)
plt.xticks(rotation=45)

# --- Umidade ---
ax2 = plt.subplot(2, 2, 2)
ax2.plot(df["time"], df["umid"], marker="o")
ax2.set_title("Umidade Relativa (%)")
ax2.set_xlabel("Horário")
ax2.set_ylabel("%")
ax2.xaxis.set_major_formatter(time_fmt)
plt.xticks(rotation=45)

# --- Pressão ---
ax3 = plt.subplot(2, 2, 3)
ax3.plot(df["time"], df["press"], marker="o")
ax3.set_title("Pressão (hPa)")
ax3.set_xlabel("Horário")
ax3.set_ylabel("hPa")
ax3.xaxis.set_major_formatter(time_fmt)
plt.xticks(rotation=45)

# --- Altitude ---
ax4 = plt.subplot(2, 2, 4)
ax4.plot(df["time"], df["alt"], marker="o")
ax4.set_title("Altitude (m)")
ax4.set_xlabel("Horário")
ax4.set_ylabel("m")
ax4.xaxis.set_major_formatter(time_fmt)
plt.xticks(rotation=45)

plt.tight_layout()
plt.show()
