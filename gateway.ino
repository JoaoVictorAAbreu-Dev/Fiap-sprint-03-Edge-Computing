/* Sprint 03 - Gateway ESP32
 * Recebe eventos por ESP-NOW, aciona LED local e tenta publicar via MQTT.
 * Se a internet falhar, mantém fila RAM para reenvio quando reconectar.
 */
#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>

#define LED_ALERTA 2
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";
WiFiClient wifiClient; PubSubClient mqtt(wifiClient);

typedef struct { char no[12]; float alturaMedia; char estado[20]; bool alerta; } Mensagem;
struct EventoPendente { Mensagem msg; };
EventoPendente fila[10]; int filaInicio = 0, filaFim = 0;

void enfileirar(Mensagem msg) { if (filaFim - filaInicio < 10) fila[filaFim++ % 10] = {msg}; }
void publicarOuGuardar(Mensagem msg) {
  String payload = String("{\"no\":\"") + msg.no + "\",\"altura_cm\":" + msg.alturaMedia + ",\"estado\":\"" + msg.estado + "\"}";
  if (WiFi.status() == WL_CONNECTED && mqtt.connected()) { mqtt.publish("rodovia/vegetacao/eventos", payload.c_str()); Serial.println("Publicado na nuvem"); }
  else { enfileirar(msg); Serial.println("Internet indisponível: evento guardado na fila"); }
}
void recebeu(const uint8_t*, const uint8_t* dados, int tamanho) {
  if (tamanho != sizeof(Mensagem)) return;
  Mensagem msg; memcpy(&msg, dados, sizeof(msg));
  digitalWrite(LED_ALERTA, msg.alerta ? HIGH : LOW); // decisão local no gateway
  Serial.printf("Recebido %s: %.2f cm - %s\n", msg.no, msg.alturaMedia, msg.estado);
  publicarOuGuardar(msg);
}
void setup() {
  Serial.begin(115200); pinMode(LED_ALERTA, OUTPUT); WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS); mqtt.setServer("broker.hivemq.com", 1883);
  esp_now_init(); esp_now_register_recv_cb(recebeu);
}
void loop() {
  if (WiFi.status() == WL_CONNECTED && !mqtt.connected()) mqtt.connect("gateway-vegetacao-sprint03");
  mqtt.loop(); delay(100);
}
