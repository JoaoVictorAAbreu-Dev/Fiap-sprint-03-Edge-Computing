# Especificação — Sprint 03: Sistema Edge Integrado para Monitoramento de Vegetação

## Objetivo
Evoluir a PoC para uma arquitetura distribuída capaz de monitorar dois pontos de uma rodovia, processar leituras perto da fonte, tomar decisões sem nuvem e enviar apenas eventos relevantes a um gateway. O sistema deve permanecer útil durante perda de internet.

## Arquitetura escolhida
Dois ESP32 atuam como nós sensores, cada um conectado a um sensor ultrassônico. Um terceiro ESP32 atua como gateway. Os nós usam ESP-NOW por ser comunicação direta, de baixa latência e sem exigir roteador; o gateway é o único componente que tenta usar Wi-Fi/MQTT para integração externa.

## Regras funcionais
Leituras entre 0 e 80 cm são válidas. Cada ciclo usa cinco amostras e calcula a média. A média abaixo de 25 cm é `NORMAL`; de 25 cm (inclusive) a 30 cm é `ATENCAO`; a partir de 30 cm é `CORTE_NECESSARIO`. O nó envia mudança de estado ou qualquer estado de atenção/alerta. Leitura sem amostras válidas gera `FALHA_SENSOR`.

## Comandos

```bash
python3 -m unittest discover -s tests -v
PYTHONPATH=src python3 src/simulate.py
manus-render-diagram diagrams/arquitetura.mmd docs/arquitetura.png
```

## Critérios de aceite

| Critério da rubrica | Evidência no projeto |
|---|---|
| Múltiplos dispositivos | `firmware/sensor_node.ino` + `firmware/gateway.ino`; dois nós configuráveis como PONTO_A/PONTO_B |
| Processamento Edge | média, filtragem e classificação no nó; LED de alerta no gateway |
| Comunicação | ESP-NOW entre nós/gateway e MQTT gateway/nuvem |
| Tratamento de dados | descarte de valores inválidos, média de cinco amostras, classificação e eventos |
| Estratégia | transmissão somente em mudança/alerta; justificativa em `docs/estrategia-edge.md` |
| Cenários | normal, atenção, corte e falha cobertos por testes e simulador |
| Falha | fila offline no gateway e manutenção da decisão/LED local |

## Limites e premissas
A simulação considera o sensor ultrassônico como medidor de altura já calibrado em centímetros. A fila do firmware é uma fila RAM de demonstração; em uma evolução para campo, deve ser persistida em NVS/Preferences para sobreviver a reinicialização.
