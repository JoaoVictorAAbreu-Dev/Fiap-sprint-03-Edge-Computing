# Estratégia Edge e justificativas

## Fluxo de dados

O sensor ultrassônico coleta a distância correspondente à altura da vegetação. Cada nó ESP32 executa cinco medições, remove timeouts, valores não numéricos e valores fora de 0–80 cm e calcula a média das leituras válidas. Em seguida, classifica o lote em **NORMAL**, **ATENCAO** ou **CORTE_NECESSARIO**. O gateway recebe o evento compacto por ESP-NOW, acende o LED de alerta imediatamente e só então tenta encaminhar o evento por MQTT.

## O que é processado localmente

A filtragem, a média, a classificação e a detecção de mudança de estado acontecem no nó, sem servidor. A decisão de acionar o LED também ocorre no gateway. Isso reduz a latência de resposta e permite alertar mesmo quando o Wi-Fi ou a nuvem estão indisponíveis. O dado enviado não é o fluxo bruto de medições: é um evento contendo `tipo`, identificador do nó, média, estado e quantidade de leituras válidas. Quando não há nenhuma leitura válida, o nó transmite `FALHA_SENSOR` para diagnóstico.

## Quando transmitir

O nó não transmite quando o estado continua **NORMAL**. Ele transmite na mudança de estado e mantém a transmissão durante estados de **ATENCAO** ou **CORTE_NECESSARIO**, para que o gateway possa acompanhar um risco ativo. Eventos de falha do sensor também são reportados. Essa estratégia reduz consumo de energia e tráfego, sem esconder situações que exigem ação.

## Por que ESP-NOW + MQTT

ESP-NOW cria comunicação direta entre ESP32, sem depender de um access point e com baixo overhead. É adequado ao trecho entre pontos da rodovia e atende ao requisito de múltiplos dispositivos. MQTT é usado somente no gateway para integração opcional com a nuvem, por ser leve e orientado a eventos. Assim, a função principal não depende de internet.

## Perda de internet

Com a internet indisponível, os nós continuam medindo, filtrando e classificando. O gateway continua recebendo por ESP-NOW e acendendo o LED local. Eventos que não podem ser publicados são colocados em uma fila circular limitada a dez posições. O loop do gateway tenta reconectar ao Wi-Fi e ao MQTT sem bloquear a recepção local; quando a conexão volta, a fila é drenada em ordem FIFO e cada evento é publicado. Se o broker cair durante a drenagem, o evento que falhou retorna à frente da fila.

Na PoC, a fila é mantida em RAM e pode perder eventos se o gateway for reiniciado ou se exceder dez eventos. Em uma versão de campo, a fila deve ser persistida em NVS/Preferences ou cartão SD, além de contar com autenticação e broker privado.

## Análise de falhas

| Falha | Comportamento | Recuperação |
|---|---|---|
| Internet/MQTT indisponível | decisão local continua, LED funciona e evento vai para fila | reconexão automática e reenvio FIFO |
| Sensor com timeout/valor inválido | evento `FALHA_SENSOR`; não classifica lixo | inspeção ou substituição do sensor |
| ESP-NOW com pacote inválido | gateway ignora pacote de tamanho incorreto | próximo ciclo tenta novamente |
| Fila offline cheia | evento mais novo é descartado e a ocorrência aparece no serial | migrar para armazenamento persistente ou aumentar capacidade |
| Reinício do nó | retorna a monitorar; estado anterior é reinicializado | primeiro evento válido sincroniza o gateway |
| Reinício do gateway | nós continuam tentando transmitir; fila RAM é perdida | reinicializar gateway e usar fila persistente em campo |
