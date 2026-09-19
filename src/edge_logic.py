"""Regras de processamento local do sistema de monitoramento de vegetação."""

from dataclasses import dataclass
from math import isfinite
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


def filter_readings(readings: Iterable[object]) -> FilterResult:
    """Remove leituras inválidas e calcula a média do lote no Edge.

    Além dos limites físicos do sensor, valores não numéricos e ``NaN`` são
    descartados para que uma amostra corrompida não interrompa o ciclo.
    """
    valid: list[float] = []
    for raw_value in readings:
        try:
            value = float(raw_value)
        except (TypeError, ValueError):
            continue
        if isfinite(value) and MIN_VALID_HEIGHT <= value <= MAX_VALID_HEIGHT:
            valid.append(value)

    return FilterResult(valid, round(fmean(valid), 2) if valid else None)


def classify_height(height: float) -> Classification:
    """Classifica a altura média da vegetação em três níveis de ação."""
    if height < ATTENTION_HEIGHT:
        return Classification("NORMAL", 0)
    if height < CUT_HEIGHT:
        return Classification("ATENCAO", 1)
    return Classification("CORTE_NECESSARIO", 2)


def build_event(
    node_id: str, readings: Iterable[object], previous_label: str | None
) -> dict[str, object] | None:
    """Converte leituras em evento compacto.

    O nó transmite uma falha, uma mudança de estado ou qualquer estado de
    atenção/alerta. Leituras normais repetidas permanecem somente no Edge.
    """
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
