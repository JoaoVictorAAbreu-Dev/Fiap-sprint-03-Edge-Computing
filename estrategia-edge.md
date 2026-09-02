# Estratégia Edge e justificativas

## Fluxo de dados

O sensor ultrassônico coleta a distância correspondente à altura da vegetação. O nó ESP32 executa cinco medições, remove timeout e valores fora de 0–80 cm e calcula a média. Em seguida, classifica o lote em **NORMAL**, **ATENÇÃO** ou **CORTE NECESSÁRIO**. O gateway recebe o evento compacto por ESP-NOW, acende o LED de alerta imediatamente e só então tenta encaminhar o evento por MQTT.

## O que é processado localmente

A filtragem, a média, a classificação e a detecção de mudança de estado acontecem no nó, sem servidor. A decisão de acionar o LED também ocorre no gateway. Isso reduz a latência de resposta e permite alertar mesmo quando o Wi-Fi ou a nuvem estão indisponíveis. O dado enviado não é o fluxo bruto de medições: é um evento contendo nó, média, estado e quantidade de leituras válidas.

## Quando transmitir

O nó não transmite quando o estado continua **NORMAL**. Ele transmite na mudança de estado e mantém a transmissão durante estados de **ATENÇÃO** ou **CORTE NECESSÁRIO**, para que o gateway possa acompanhar um risco ativo. Eventos de falha do sensor também são reportados. Essa estratégia reduz consumo de energia e tráfego, sem esconder situações que exigem ação.

## Por que ESP-NOW + MQTT

ESP-NOW cria comunicação direta entre ESP32, sem depender de um access point e com baixo overhead. É adequado ao trecho entre pontos da rodovia e ao requisito de múltiplos dispositivos. MQTT é usado somente no gateway para integração opcional com a nuvem, por ser leve e orientado a eventos. Assim, a função principal não depende de internet.

## Perda de internet

Com a internet indisponível, os nós continuam medindo, filtrando e classificando. O gateway continua recebendo por ESP-NOW e acendendo o LED local. Eventos que não podem ser publicados são colocados em fila local; na reconexão, a fila deve ser drenada. Na PoC, a fila é limitada a dez eventos em RAM. O comportamento de descarte por fila cheia deve ser substituído por NVS ou cartão SD na versão de campo.

## Análise de falhas

| Falha | Comportamento | Recuperação |
|---|---|---|
| Internet/MQTT indisponível | decisão local continua; evento vai para fila | publicação após reconexão |
| Sensor com timeout/valor inválido | evento `FALHA_SENSOR`; não classifica lixo | inspeção ou substituição do sensor |
| ESP-NOW com pacote inválido | gateway ignora pacote de tamanho incorreto | próximo ciclo tenta novamente |
| Reinício do nó | retorna a monitorar; estado anterior é reinicializado | primeiro evento válido sincroniza o gateway |
