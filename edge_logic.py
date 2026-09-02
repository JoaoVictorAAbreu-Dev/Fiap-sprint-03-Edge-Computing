"""Regras de processamento local do sistema de monitoramento de vegetação."""

from dataclasses import dataclass
from statistics import fmean
from typing import Iterable

MIN_VALID_HEIGHT = 0.0
MAX_VALID_HEIGHT = 80.0
ATTENTION_HEIGHT = 25.0
CUT_HEIGHT = 30.0


@dataclass(frozen=True)
class Classification:
    label: str
    severity: int


@dataclass(frozen=True)
class FilterResult:
    valid_readings: list[float]
    average: float | None


def filter_readings(readings: Iterable[float]) -> FilterResult:
    """Remove leituras impossíveis e calcula a média do lote localmente."""
    valid = [float(value) for value in readings if MIN_VALID_HEIGHT <= value <= MAX_VALID_HEIGHT]
    return FilterResult(valid, round(fmean(valid), 2) if valid else None)


def classify_height(height: float) -> Classification:
    """Classifica a altura média da vegetação em três níveis de ação."""
    if height < ATTENTION_HEIGHT:
        return Classification("NORMAL", 0)
    if height < CUT_HEIGHT:
        return Classification("ATENCAO", 1)
    return Classification("CORTE_NECESSARIO", 2)


def build_event(node_id: str, readings: Iterable[float], previous_label: str | None) -> dict | None:
    """Converte leituras em evento compacto; retorna None quando não há mudança relevante."""
    filtered = filter_readings(readings)
    if filtered.average is None:
        return {
            "tipo": "FALHA_SENSOR",
            "no": node_id,
            "estado": "LEITURA_INVALIDA",
            "leituras_validas": 0,
        }

    classification = classify_height(filtered.average)
    is_alert = classification.severity > 0
    changed = classification.label != previous_label
    if not is_alert and not changed:
        return None

    return {
        "tipo": "ALERTA_VEGETACAO" if is_alert else "MUDANCA_ESTADO",
        "no": node_id,
        "altura_media_cm": filtered.average,
        "estado": classification.label,
        "leituras_validas": len(filtered.valid_readings),
    }
