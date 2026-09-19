"""Demonstração sem hardware: nós filtram/classificam e gateway enfileira offline."""

import json
from dataclasses import dataclass, field

from edge_logic import build_event


@dataclass
class GatewaySimulation:
    """Representa a fila RAM do gateway e sua drenagem após reconexão."""

    offline_queue: list[dict[str, object]] = field(default_factory=list)
    max_queue_size: int = 10

    def receive(self, event: dict[str, object], internet_ok: bool) -> None:
        if internet_ok:
            self.flush()
            print("  GATEWAY -> NUVEM:", json.dumps(event, ensure_ascii=False))
            return

        if len(self.offline_queue) >= self.max_queue_size:
            print("  GATEWAY -> FILA OFFLINE CHEIA: evento descartado")
            return

        self.offline_queue.append(event)
        print("  GATEWAY -> FILA OFFLINE:", json.dumps(event, ensure_ascii=False))

    def flush(self) -> None:
        if not self.offline_queue:
            return
        print(f"  GATEWAY -> NUVEM: reenviando {len(self.offline_queue)} evento(s)")
        for event in self.offline_queue:
            print("    ", json.dumps(event, ensure_ascii=False))
        self.offline_queue.clear()


def run() -> None:
    previous = {"PONTO_A": "NORMAL", "PONTO_B": "NORMAL"}
    gateway = GatewaySimulation()
    scenarios = [
        (
            "Cenário 1 - normal",
            {"PONTO_A": [18, 19, 18], "PONTO_B": [20, 19, 21]},
            True,
        ),
        (
            "Cenário 2 - atenção",
            {"PONTO_A": [26, 27, 26], "PONTO_B": [22, 23, 22]},
            True,
        ),
        (
            "Cenário 3 - corte necessário",
            {"PONTO_A": [33, 34, 35], "PONTO_B": [22, 23, 22]},
            False,
        ),
        (
            "Cenário 4 - falha e internet indisponível",
            {"PONTO_A": [-1, 101], "PONTO_B": [32, 33, 32]},
            False,
        ),
    ]

    for name, node_readings, internet_ok in scenarios:
        print(f"\n{name}")
        for node_id, readings in node_readings.items():
            event = build_event(node_id, readings, previous[node_id])
            if event is None:
                print(f"  {node_id}: NORMAL (sem transmissão)")
                continue

            previous[node_id] = str(event["estado"])
            gateway.receive(event, internet_ok)

    print("\nCenário 5 - reconexão")
    gateway.flush()
    print(f"Fila offline após reconexão: {len(gateway.offline_queue)} evento(s)")


if __name__ == "__main__":
    run()
