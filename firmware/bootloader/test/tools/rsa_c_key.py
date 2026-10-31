#!/usr/bin/env python3
"""
============================================================================
rsa_c_key.py — Conversion d'une clé publique RSA-2048 au format embarqué
============================================================================

Produit les champs attendus par `RSAPublicKey` (lib/crypto/rsa_light.h) :
  - n      : modulus, en mots little-endian de 32 bits
  - rr     : R^2 mod n   (R = 2^2048)       -> évite toute division sur la cible
  - n0inv  : -1 / n[0] mod 2^32              -> réduction de Montgomery

Ce module est importé par rsa_keygen.py et par les tests. Il ne dépend que de
la bibliothèque standard + `cryptography`.
============================================================================
"""

from __future__ import annotations

NUM_WORDS = 64          # RSA-2048 = 64 mots de 32 bits
WORD_BITS = 32
WORD_MASK = 0xFFFFFFFF
R = 1 << (NUM_WORDS * WORD_BITS)   # 2^2048


def _to_words(value: int) -> list[int]:
    """Entier -> liste de 64 mots 32 bits little-endian (mot 0 = poids faible)."""
    return [(value >> (WORD_BITS * i)) & WORD_MASK for i in range(NUM_WORDS)]


def c_key_fields(n: int) -> dict:
    """
    Calcule tous les champs embarqués à partir du modulus `n`.
    (L'exposant public est figé à 65537 côté firmware, donc inutile ici.)
    """
    if n.bit_length() != 2048:
        raise ValueError(f"Modulus attendu de 2048 bits, reçu {n.bit_length()}")

    n0 = n & WORD_MASK
    # -1 / n[0] mod 2^32
    n0inv = (-pow(n0, -1, 1 << WORD_BITS)) & WORD_MASK
    rr = (R * R) % n

    return {
        "n0inv": n0inv,
        "n": _to_words(n),
        "rr": _to_words(rr),
    }


def render_c_header(n: int, var_name: str = "g_firmware_public_key",
                    comment: str = "") -> str:
    """Génère le contenu d'un header C instanciant un `RSAPublicKey` constant."""
    f = c_key_fields(n)

    def fmt(words: list[int]) -> str:
        lines = []
        for i in range(0, NUM_WORDS, 4):
            chunk = ", ".join(f"0x{w:08x}" for w in words[i:i + 4])
            lines.append(f"        {chunk},")
        return "\n".join(lines)

    return f"""/**
 * ============================================================================
 * CLÉ PUBLIQUE RSA-2048 EMBARQUÉE — GÉNÉRÉ AUTOMATIQUEMENT, NE PAS ÉDITER
 * ============================================================================
 * {comment}
 * Régénérer avec :  python tools/rsa_keygen.py
 * La clé PRIVÉE correspondante ne doit JAMAIS être commitée.
 * ============================================================================
 */
#ifndef RSA_PUBLIC_KEY_H
#define RSA_PUBLIC_KEY_H

#include "rsa_light.h"

static const RSAPublicKey {var_name} = {{
    .len = {NUM_WORDS},
    .n0inv = 0x{f['n0inv']:08x},
    .n = {{
{fmt(f['n'])}
    }},
    .rr = {{
{fmt(f['rr'])}
    }},
}};

#endif /* RSA_PUBLIC_KEY_H */
"""
