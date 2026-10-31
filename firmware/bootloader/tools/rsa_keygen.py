#!/usr/bin/env python3
"""
============================================================================
rsa_keygen.py — Génère la paire de clés RSA-2048 du Secure Boot
============================================================================

- Crée une clé PRIVÉE RSA-2048 (exposant public 65537) -> keys/signing_key.pem
- Dérive le header C de la clé PUBLIQUE                 -> include/rsa_public_key.h

La clé privée sert à signer les firmwares (tools/firmware_signer.py).
Le bootloader n'embarque QUE la clé publique.

⚠️  keys/ est ignoré par git (.gitignore). Ne jamais committer la clé privée.
    En production, cette clé vit dans un HSM, pas sur disque.

Usage:
    python tools/rsa_keygen.py                 # génère si absente
    python tools/rsa_keygen.py --force         # régénère (nouvelle clé)
============================================================================
"""

import argparse
from pathlib import Path

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa

from rsa_c_key import render_c_header

HERE = Path(__file__).resolve().parent
PROJECT_ROOT = HERE.parent
KEY_DIR = PROJECT_ROOT / "keys"
PRIVATE_KEY_PATH = KEY_DIR / "signing_key.pem"
PUBLIC_HEADER_PATH = PROJECT_ROOT / "include" / "rsa_public_key.h"

PUBLIC_EXPONENT = 65537
KEY_SIZE = 2048


def generate_private_key() -> rsa.RSAPrivateKey:
    return rsa.generate_private_key(
        public_exponent=PUBLIC_EXPONENT,
        key_size=KEY_SIZE,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Génère la paire de clés Secure Boot")
    parser.add_argument("--force", action="store_true",
                        help="Régénère même si la clé existe déjà")
    args = parser.parse_args()

    KEY_DIR.mkdir(exist_ok=True)

    if PRIVATE_KEY_PATH.exists() and not args.force:
        print(f"[=] Clé privée déjà présente : {PRIVATE_KEY_PATH}")
        print("    (utiliser --force pour en générer une nouvelle)")
        private_key = serialization.load_pem_private_key(
            PRIVATE_KEY_PATH.read_bytes(), password=None
        )
    else:
        print(f"[+] Génération d'une clé RSA-{KEY_SIZE} (e={PUBLIC_EXPONENT})...")
        private_key = generate_private_key()
        pem = private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.NoEncryption(),
        )
        PRIVATE_KEY_PATH.write_bytes(pem)
        PRIVATE_KEY_PATH.chmod(0o600)
        print(f"[+] Clé privée écrite : {PRIVATE_KEY_PATH}")

    n = private_key.public_key().public_numbers().n
    header = render_c_header(
        n, comment=f"RSA-{KEY_SIZE}, e={PUBLIC_EXPONENT}"
    )
    PUBLIC_HEADER_PATH.write_text(header)
    print(f"[+] Header public écrit : {PUBLIC_HEADER_PATH}")
    print("\n[✓] Clés prêtes. Signer un firmware :")
    print("    python tools/firmware_signer.py build/app.bin -o app_signed.bin "
          "--version 1.0.0 --security-version 1")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
