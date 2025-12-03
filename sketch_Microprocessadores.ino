#include <WiFi.h>
#include <HTTPClient.h>

#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

// ==== CONFIGURAÇÕES DE WIFI ====
// COLOCA AQUI O NOME E SENHA DA SUA REDE
const char* WIFI_SSID     = "VELLOZNET-AP203";
const char* WIFI_PASSWORD = "AtaraXia.74";

// ==== CONFIGURAÇÕES DO THINGSPEAK ====
// Vá no seu canal -> aba "API Keys" -> pegue a "Write API Key"
const char* THINGSPEAK_API_KEY = "95C3PAW5NTYGUL1X";  // <-- TROCAR AQUI
const char* THINGSPEAK_URL     = "https://api.thingspeak.com/update";

// ==== PINOS I2C NO SEU SETUP ====
#define I2C_SDA 32
#define I2C_SCL 33

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

// Intervalo entre envios (ThingSpeak recomenda >= 15 s)
const unsigned long INTERVALO_ENVIO_MS = 60000;
unsigned long ultimoEnvio = 0;

void conectaWiFi() {
  Serial.print("Conectando ao WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long inicio = millis();
  const unsigned long timeout = 15000; // 15 s

  while (WiFi.status() != WL_CONNECTED && millis() - inicio < timeout) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Falha ao conectar no WiFi.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  // I2C
  Serial.println("Inicializando I2C...");
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  // AHT20
  Serial.println("Inicializando AHT20...");
  if (!aht.begin()) {
    Serial.println("ERRO: AHT20 nao encontrado!");
    while (1) {
      delay(100);
    }
  }
  Serial.println("AHT20 OK.");

  // BMP280 (endereco 0x77, que voce ja viu no scan)
  Serial.println("Inicializando BMP280...");
  if (!bmp.begin(0x77)) {
    Serial.println("ERRO: BMP280 nao encontrado em 0x77!");
    while (1) {
      delay(100);
    }
  }
  Serial.println("BMP280 OK.");

  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,   // Temp
    Adafruit_BMP280::SAMPLING_X16,  // Pressao
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500
  );

  // WiFi
  conectaWiFi();
}

void loop() {
  // Se cair o WiFi, tenta reconectar
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, tentando reconectar...");
    conectaWiFi();
  }

  unsigned long agora = millis();
  if (agora - ultimoEnvio >= INTERVALO_ENVIO_MS) {
    ultimoEnvio = agora;

    // ===== LEITURA DOS SENSORES =====
    sensors_event_t humEvent, tempEvent;
    aht.getEvent(&humEvent, &tempEvent);

    float tempAHT = tempEvent.temperature;            // °C
    float umid    = humEvent.relative_humidity;       // %
    float tempBMP = bmp.readTemperature();            // °C
    float press   = bmp.readPressure() / 100.0F;      // hPa
    float alt     = bmp.readAltitude(1013.25);        // m (ajusta se quiser)

    Serial.println("Leituras:");
    Serial.print("Temp AHT20: ");
    Serial.print(tempAHT);
    Serial.println(" °C");

    Serial.print("Umidade: ");
    Serial.print(umid);
    Serial.println(" %");

    Serial.print("Temp BMP280: ");
    Serial.print(tempBMP);
    Serial.println(" °C");

    Serial.print("Pressao: ");
    Serial.print(press);
    Serial.println(" hPa");

    Serial.print("Altitude: ");
    Serial.print(alt);
    Serial.println(" m");
    Serial.println("-------------------------");

    // ===== ENVIO PARA THINGSPEAK =====
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      // Monta a URL com os fields
      // Field1 = tempAHT, Field2 = umid, Field3 = press, Field4 = alt
      String url = String(THINGSPEAK_URL) +
                   "?api_key=" + THINGSPEAK_API_KEY +
                   "&field1=" + String(tempAHT, 2) +
                   "&field2=" + String(umid, 2) +
                   "&field3=" + String(press, 2) +
                   "&field4=" + String(alt, 2);

      Serial.print("Enviando para ThingSpeak: ");
      Serial.println(url);

      http.begin(url);
      int httpCode = http.GET();

      Serial.print("HTTP code: ");
      Serial.println(httpCode);

      if (httpCode > 0) {
        String payload = http.getString();
        Serial.print("Resposta ThingSpeak: ");
        Serial.println(payload);  // normalmente é o número do entry criado
      } else {
        Serial.print("Falha na requisicao HTTP: ");
        Serial.println(http.errorToString(httpCode));
      }

      http.end();
    } else {
      Serial.println("Sem WiFi, nao foi possivel enviar ao ThingSpeak.");
    }
  }

  // Nada crítico aqui, só espera
  delay(100);
}
