import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / "src"))

from simulate import GatewaySimulation


class GatewaySimulationTests(unittest.TestCase):
    def test_offline_events_are_flushed_after_reconnection(self):
        gateway = GatewaySimulation()
        event = {"tipo": "ALERTA_VEGETACAO", "no": "PONTO_A"}

        gateway.receive(event, internet_ok=False)
        self.assertEqual(gateway.offline_queue, [event])

        gateway.flush()
        self.assertEqual(gateway.offline_queue, [])

    def test_queue_has_explicit_limit(self):
        gateway = GatewaySimulation(max_queue_size=1)
        first = {"no": "PONTO_A"}
        second = {"no": "PONTO_B"}

        gateway.receive(first, internet_ok=False)
        gateway.receive(second, internet_ok=False)

        self.assertEqual(gateway.offline_queue, [first])


if __name__ == "__main__":
    unittest.main()
