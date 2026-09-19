/* Sprint 03 - Nó Sensor ESP32
 * Cada nó mede a altura, filtra ruído, classifica localmente e envia somente eventos.
 * Hardware PoC: HC-SR04 (TRIG 5, ECHO 18) + ESP-NOW.
 */
#include <WiFi.h>
#include <esp_now.h>

#define TRIG_PIN 5
#define ECHO_PIN 18

const char* NODE_ID = "PONTO_A"; // Troque para PONTO_B no segundo ESP32.
const float ATENCAO_CM = 25.0;
const float CORTE_CM = 30.0;
uint8_t gatewayMac[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}; // MAC do gateway.
String ultimoEstado = "NORMAL";

// O layout precisa ser igual no sensor e no gateway.
typedef struct {
  char no[12];
  float alturaMedia;
  char estado[20];
  uint8_t alerta;
  uint8_t leiturasValidas;
} Mensagem;

float lerAlturaCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duracao == 0) return -1; // Timeout: leitura inválida.
  return duracao * 0.0343 / 2.0;
}

String classificar(float altura) {
  if (altura < ATENCAO_CM) return "NORMAL";
  if (altura < CORTE_CM) return "ATENCAO";
  return "CORTE_NECESSARIO";
}

void preencherMensagem(Mensagem& msg, float media, const char* estado, uint8_t validas) {
  memset(&msg, 0, sizeof(msg));
  strncpy(msg.no, NODE_ID, sizeof(msg.no) - 1);
  msg.alturaMedia = media;
  strncpy(msg.estado, estado, sizeof(msg.estado) - 1);
  msg.alerta = strcmp(estado, "NORMAL") != 0;
  msg.leiturasValidas = validas;
}

void enviarMensagem(Mensagem& msg) {
  esp_err_t resultado = esp_now_send(gatewayMac, (uint8_t*)&msg, sizeof(msg));
  if (resultado != ESP_OK) {
    Serial.printf("Falha ao enviar evento ESP-NOW: %d\n", resultado);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Falha ao inicializar ESP-NOW");
    return;
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, gatewayMac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Falha ao cadastrar gateway ESP-NOW");
  }
}

void loop() {
  float soma = 0;
  uint8_t validas = 0;

  // Tratamento local: cinco amostras e descarte de timeout/valores fora de 0-80 cm.
  for (int i = 0; i < 5; i++) {
    float leitura = lerAlturaCm();
    if (leitura >= 0 && leitura <= 80) {
      soma += leitura;
      validas++;
    }
    delay(80);
  }

  if (validas == 0) {
    Mensagem falha;
    preencherMensagem(falha, 0, "FALHA_SENSOR", 0);
    enviarMensagem(falha);
    Serial.println("FALHA_SENSOR: sem leituras válidas");
    delay(5000);
    return;
  }

  float media = soma / validas;
  String estado = classificar(media);
  bool deveEnviar = estado != ultimoEstado || estado != "NORMAL";

  if (deveEnviar) {
    Mensagem msg;
    preencherMensagem(msg, media, estado.c_str(), validas);
    enviarMensagem(msg);
    Serial.printf("Evento enviado: %.2f cm - %s (%d leituras válidas)\n",
                  media, estado.c_str(), validas);
    ultimoEstado = estado;
  } else {
    Serial.printf("NORMAL %.2f cm - sem transmissão\n", media);
  }

  delay(5000);
}
