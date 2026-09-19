# Roteiro de teste e demonstração

Execute `python3 -m unittest discover -s tests -v` para validar a lógica sem hardware e `PYTHONPATH=src python3 src/simulate.py` para apresentar o fluxo completo. No Wokwi ou no hardware, carregue `firmware/sensor_node.ino` em dois ESP32, alterando `NODE_ID` para `PONTO_A` e `PONTO_B`, e use um terceiro ESP32 para `firmware/gateway.ino`.

| Cenário | Entrada | Resultado esperado | Requisito demonstrado |
|---|---|---|---|
| 1 — Vegetação normal | Ponto A: 18, 19, 18 cm; Ponto B: 20, 19, 21 cm | estado normal e nenhuma transmissão repetitiva | economia de energia/tráfego |
| 2 — Próxima do limite | Ponto A: 26, 27, 26 cm | evento `ATENCAO` | processamento e classificação Edge |
| 3 — Acima do limite | Ponto A: 33, 34, 35 cm | evento `CORTE_NECESSARIO` e LED aceso | alerta local de baixa latência |
| 4 — Falha | Ponto A: timeout e 101 cm; internet desligada | `FALHA_SENSOR`; evento do Ponto B fica na fila; LED continua local | tratamento de falhas e operação offline |
| 5 — Reconexão | restabelecer Wi-Fi/MQTT | gateway drena a fila em ordem e publica os eventos pendentes | recuperação de comunicação |

## Evidências para apresentação

Abra os monitores seriais dos nós e do gateway. Mostre que o nó imprime `NORMAL ... sem transmissão` no cenário 1, que o gateway recebe `ATENCAO` no cenário 2 e que o LED muda de estado no cenário 3. No cenário 4, desconecte a rede e mostre `Internet/MQTT indisponível: evento guardado na fila`, mantendo o LED e a comunicação local ativos. No cenário 5, reconecte a rede e mostre `Evento offline reenviado` até a fila ficar vazia.

## Critérios de aceite observáveis

1. Existem dois nós sensores independentes e um gateway.
2. A média, a filtragem e a classificação são feitas nos nós, antes da comunicação externa.
3. O payload transmitido contém evento tratado, e não as cinco amostras brutas.
4. O gateway aciona o alerta local antes de depender do MQTT.
5. A função principal continua durante a perda de internet.
6. Os testes automatizados e o simulador cobrem normal, atenção, corte, falha e reconexão.
