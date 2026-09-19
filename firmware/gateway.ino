/* Sprint 03 - Gateway ESP32
 * Recebe eventos por ESP-NOW, aciona LED local e tenta publicar via MQTT.
 * Se a internet falhar, mantém fila RAM e reenvia após a reconexão.
 */
#include <WiFi.h>
#include <esp_now.h>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif
#include <PubSubClient.h>

#define LED_ALERTA 2
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";
const char* MQTT_BROKER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_TOPIC = "rodovia/vegetacao/eventos";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// O layout precisa ser igual no sensor e no gateway.
typedef struct {
  char no[12];
  float alturaMedia;
  char estado[20];
  uint8_t alerta;
  uint8_t leiturasValidas;
} Mensagem;

const uint8_t TAMANHO_FILA = 10;
Mensagem fila[TAMANHO_FILA];
uint8_t filaInicio = 0;
uint8_t filaQuantidade = 0;
unsigned long ultimaTentativaWifi = 0;
unsigned long ultimaTentativaMqtt = 0;

// O callback apenas copia o pacote; operações de rede ficam no loop principal.
volatile bool novaMensagem = false;
Mensagem mensagemRecebida;

bool filaVazia() {
  return filaQuantidade == 0;
}

bool enfileirar(const Mensagem& msg) {
  if (filaQuantidade >= TAMANHO_FILA) {
    Serial.println("Fila offline cheia: evento descartado");
    return false;
  }

  uint8_t posicao = (filaInicio + filaQuantidade) % TAMANHO_FILA;
  fila[posicao] = msg;
  filaQuantidade++;
  return true;
}

bool retirarDaFila(Mensagem& msg) {
  if (filaVazia()) return false;
  msg = fila[filaInicio];
  filaInicio = (filaInicio + 1) % TAMANHO_FILA;
  filaQuantidade--;
  return true;
}

String payloadDaMensagem(const Mensagem& msg) {
  const char* tipo = strcmp(msg.estado, "FALHA_SENSOR") == 0
                       ? "FALHA_SENSOR"
                       : (msg.alerta ? "ALERTA_VEGETACAO" : "MUDANCA_ESTADO");
  return String("{\"tipo\":\"") + tipo +
         "\",\"no\":\"" + msg.no +
         "\",\"altura_media_cm\":" + String(msg.alturaMedia, 2) +
         ",\"estado\":\"" + msg.estado +
         "\",\"leituras_validas\":" + String(msg.leiturasValidas) + "}";
}

bool publicar(const Mensagem& msg) {
  if (WiFi.status() != WL_CONNECTED || !mqtt.connected()) return false;
  String payload = payloadDaMensagem(msg);
  bool publicado = mqtt.publish(MQTT_TOPIC, payload.c_str());
  if (publicado) {
    Serial.printf("Publicado na nuvem: %s\n", payload.c_str());
  } else {
    Serial.println("MQTT recusou o evento; mantendo na fila");
  }
  return publicado;
}

void publicarOuGuardar(const Mensagem& msg) {
  if (!publicar(msg)) {
    if (enfileirar(msg)) {
      Serial.println("Internet/MQTT indisponível: evento guardado na fila");
    }
  }
}

void drenarFila() {
  if (WiFi.status() != WL_CONNECTED || !mqtt.connected()) return;

  Mensagem pendente;
  while (retirarDaFila(pendente)) {
    if (!publicar(pendente)) {
      // Recoloca na frente lógica da fila. Não perde o evento se o broker cair.
      filaInicio = (filaInicio + TAMANHO_FILA - 1) % TAMANHO_FILA;
      fila[filaInicio] = pendente;
      filaQuantidade++;
      return;
    }
    Serial.printf("Evento offline reenviado; restam %d\n", filaQuantidade);
  }
}

void processarMensagem(const Mensagem& msg) {
  digitalWrite(LED_ALERTA, msg.alerta ? HIGH : LOW);
  Serial.printf("Recebido %s: %.2f cm - %s (%d leituras válidas)\n",
                msg.no, msg.alturaMedia, msg.estado, msg.leiturasValidas);
  publicarOuGuardar(msg);
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void recebeu(const esp_now_recv_info_t*, const uint8_t* dados, int tamanho) {
#else
void recebeu(const uint8_t*, const uint8_t* dados, int tamanho) {
#endif
  if (tamanho != sizeof(Mensagem)) {
    Serial.println("Pacote ESP-NOW ignorado: tamanho inválido");
    return;
  }
  memcpy((void*)&mensagemRecebida, dados, sizeof(Mensagem));
  novaMensagem = true;
}

void manterConectividade() {
  unsigned long agora = millis();

  if (WiFi.status() != WL_CONNECTED && agora - ultimaTentativaWifi >= 10000) {
    ultimaTentativaWifi = agora;
    Serial.println("Tentando reconectar ao Wi-Fi...");
    WiFi.reconnect();
  }

  if (WiFi.status() == WL_CONNECTED && !mqtt.connected() &&
      agora - ultimaTentativaMqtt >= 5000) {
    ultimaTentativaMqtt = agora;
    Serial.println("Tentando reconectar ao MQTT...");
    if (mqtt.connect("gateway-vegetacao-sprint03")) {
      Serial.println("MQTT conectado");
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_ALERTA, OUTPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Falha ao inicializar ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(recebeu);
}

void loop() {
  if (novaMensagem) {
    Mensagem local;
    noInterrupts();
    memcpy(&local, (const void*)&mensagemRecebida, sizeof(Mensagem));
    novaMensagem = false;
    interrupts();
    processarMensagem(local);
  }

  manterConectividade();
  mqtt.loop();
  drenarFila();
  delay(100);
}
