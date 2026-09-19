import math
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / "src"))

from edge_logic import classify_height, filter_readings, build_event


class EdgeLogicTests(unittest.TestCase):
    def test_classifies_three_vegetation_levels(self):
        self.assertEqual(classify_height(18).label, "NORMAL")
        self.assertEqual(classify_height(27).label, "ATENCAO")
        self.assertEqual(classify_height(34).label, "CORTE_NECESSARIO")

    def test_thresholds_are_explicit(self):
        self.assertEqual(classify_height(24.99).label, "NORMAL")
        self.assertEqual(classify_height(25).label, "ATENCAO")
        self.assertEqual(classify_height(29.99).label, "ATENCAO")
        self.assertEqual(classify_height(30).label, "CORTE_NECESSARIO")

    def test_filters_invalid_values_and_uses_average(self):
        result = filter_readings([22, 23, -1, 22, 101, 24])
        self.assertEqual(result.valid_readings, [22, 23, 22, 24])
        self.assertEqual(result.average, 22.75)

    def test_filters_non_numeric_and_non_finite_values(self):
        result = filter_readings([20, "21", None, "n/a", math.nan, math.inf])
        self.assertEqual(result.valid_readings, [20, 21])
        self.assertEqual(result.average, 20.5)

    def test_event_is_emitted_only_for_state_change_or_alert(self):
        normal = build_event("PONTO_A", [22, 23, 22], previous_label="NORMAL")
        alert = build_event("PONTO_A", [34, 35, 34], previous_label="ATENCAO")
        self.assertIsNone(normal)
        self.assertEqual(alert["tipo"], "ALERTA_VEGETACAO")
        self.assertEqual(alert["estado"], "CORTE_NECESSARIO")

    def test_normal_state_change_is_reported_once(self):
        event = build_event("PONTO_A", [24, 24, 24], previous_label="ATENCAO")
        self.assertEqual(event["tipo"], "MUDANCA_ESTADO")
        self.assertEqual(event["estado"], "NORMAL")

    def test_invalid_sensor_reading_becomes_sensor_failure_event(self):
        event = build_event("PONTO_B", [-1, 101, None], previous_label=None)
        self.assertEqual(event["tipo"], "FALHA_SENSOR")
        self.assertEqual(event["estado"], "LEITURA_INVALIDA")
        self.assertEqual(event["leituras_validas"], 0)


if __name__ == "__main__":
    unittest.main()
