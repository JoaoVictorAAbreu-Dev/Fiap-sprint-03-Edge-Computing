# Roteiro de teste e demonstração

Execute `PYTHONPATH=src python3 src/simulate.py` para demonstrar a lógica sem hardware. No Wokwi, carregue `firmware/sensor_node.ino` em dois ESP32, alterando `NODE_ID` para `PONTO_A` e `PONTO_B`, e use um terceiro para `gateway.ino`.

| Cenário | Entrada | Resultado esperado | Requisito demonstrado |
|---|---|---|---|
| 1 — Vegetação normal | Ponto A: 18, 19, 18 cm; Ponto B: 20, 19, 21 cm | estado normal e nenhuma transmissão repetitiva | economia de energia/tráfego |
| 2 — Próxima do limite | Ponto A: 26, 27, 26 cm | evento ATENCAO | processamento e classificação Edge |
| 3 — Acima do limite | Ponto A: 33, 34, 35 cm | evento CORTE_NECESSARIO e LED aceso | alerta local de baixa latência |
| 4 — Falha | Ponto A: timeout e 101 cm; internet desligada | FALHA_SENSOR; evento do Ponto B fica na fila | tratamento de falhas e operação offline |

## Evidências para apresentação

Abra os monitores seriais dos nós e do gateway. Mostre que o nó imprime `NORMAL ... sem transmissão` no cenário 1, que o gateway recebe `ATENCAO` no cenário 2, e que o LED muda de estado no cenário 3. No cenário 4, desconecte a rede e mostre `Internet indisponível: evento guardado na fila`, mantendo o LED e a comunicação local ativos.
