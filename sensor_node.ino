/* Sprint 03 - Nó Sensor ESP32
 * Cada nó mede a altura, filtra ruído, classifica localmente e envia somente eventos.
 * Hardware PoC: HC-SR04 (TRIG 5, ECHO 18) + ESP-NOW.
 */
#include <WiFi.h>
#include <esp_now.h>

#define TRIG_PIN 5
#define ECHO_PIN 18
const char* NODE_ID = "PONTO_A"; // troque para PONTO_B no segundo ESP32
const float ATENCAO_CM = 25.0;
const float CORTE_CM = 30.0;
uint8_t gatewayMac[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}; // MAC do gateway
String ultimoEstado = "NORMAL";

typedef struct { char no[12]; float alturaMedia; char estado[20]; bool alerta; } Mensagem;

float lerAlturaCm() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10); digitalWrite(TRIG_PIN, LOW);
  long duracao = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duracao == 0) return -1; // timeout = leitura inválida
  return duracao * 0.0343 / 2.0;
}

String classificar(float altura) {
  if (altura < ATENCAO_CM) return "NORMAL";
  if (altura < CORTE_CM) return "ATENCAO";
  return "CORTE_NECESSARIO";
}

void setup() {
  Serial.begin(115200); pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);
  WiFi.mode(WIFI_STA); esp_now_init();
  esp_now_peer_info_t peer{}; memcpy(peer.peer_addr, gatewayMac, 6); peer.channel = 0; peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void loop() {
  float soma = 0; int validas = 0;
  for (int i = 0; i < 5; i++) { float leitura = lerAlturaCm(); if (leitura >= 0 && leitura <= 80) { soma += leitura; validas++; } delay(80); }
  if (validas == 0) { Serial.println("FALHA_SENSOR: sem leituras válidas"); delay(5000); return; }
  float media = soma / validas; String estado = classificar(media);
  bool deveEnviar = estado != ultimoEstado || estado != "NORMAL";
  if (deveEnviar) {
    Mensagem msg{}; strncpy(msg.no, NODE_ID, sizeof(msg.no)-1); msg.alturaMedia = media;
    strncpy(msg.estado, estado.c_str(), sizeof(msg.estado)-1); msg.alerta = estado != "NORMAL";
    esp_now_send(gatewayMac, (uint8_t*)&msg, sizeof(msg));
    Serial.printf("Evento enviado: %.2f cm - %s\n", media, estado.c_str());
    ultimoEstado = estado;
  } else Serial.printf("NORMAL %.2f cm - sem transmissão\n", media);
  delay(5000);
}
