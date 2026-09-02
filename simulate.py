"""Demonstração sem hardware: dois nós filtram/classificam e gateway enfileira offline."""
import json
from edge_logic import build_event


def run():
    previous = {"PONTO_A": "NORMAL", "PONTO_B": "NORMAL"}
    scenarios = [
        ("Cenário 1 - normal", {"PONTO_A": [18, 19, 18], "PONTO_B": [20, 19, 21]}, True),
        ("Cenário 2 - atenção", {"PONTO_A": [26, 27, 26], "PONTO_B": [22, 23, 22]}, True),
        ("Cenário 3 - corte necessário", {"PONTO_A": [33, 34, 35], "PONTO_B": [22, 23, 22]}, False),
        ("Falha - sensor inválido e internet indisponível", {"PONTO_A": [-1, 101], "PONTO_B": [32, 33, 32]}, False),
    ]
    offline_queue = []
    for name, node_readings, internet_ok in scenarios:
        print(f"\n{name}")
        for node_id, readings in node_readings.items():
            event = build_event(node_id, readings, previous[node_id])
            if event is not None:
                previous[node_id] = event["estado"]
                if internet_ok:
                    print("  GATEWAY -> NUVEM:", json.dumps(event, ensure_ascii=False))
                else:
                    offline_queue.append(event)
                    print("  GATEWAY -> FILA OFFLINE:", json.dumps(event, ensure_ascii=False))
            else:
                print(f"  {node_id}: NORMAL (sem transmissão)")
    print(f"\nFila persistente aguardando reconexão: {len(offline_queue)} evento(s)")


if __name__ == "__main__":
    run()
