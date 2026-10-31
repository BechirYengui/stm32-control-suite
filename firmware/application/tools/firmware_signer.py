#!/usr/bin/env python3
"""
============================================================================
FIRMWARE SIGNER — Signe et package un firmware STM32 pour le Secure Boot
============================================================================

Signature RÉELLE RSA-2048 PKCS#1 v1.5 sur le SHA-256 de l'image, avec la clé
privée générée par rsa_keygen.py. Le bootloader vérifie cette signature avec la
clé publique embarquée : l'intégrité ET l'authenticité sont garanties.

Produit, pour une image `app.bin` :
  - app_signed.bin           : [metadata 84o | pad | signature 256o]
                               -> à flasher à 0x0800E000 (bloc de boot)
  - app_signed_metadata.json : métadonnées lisibles

Usage:
    python tools/firmware_signer.py app.bin -o app_signed.bin \\
        --version 1.2.3 --security-version 1
    python tools/firmware_signer.py app_signed.bin --verify --firmware app.bin
============================================================================
"""

import argparse
import hashlib
import json
import struct
import time
from pathlib import Path

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding

HERE = Path(__file__).resolve().parent
PROJECT_ROOT = HERE.parent
DEFAULT_PRIVATE_KEY = PROJECT_ROOT / "keys" / "signing_key.pem"

FIRMWARE_MAGIC = 0xDEADBEEF
MAX_FIRMWARE_SIZE = 48 * 1024          # 48 KB (doit matcher APPLICATION_MAX_SIZE)
METADATA_FORMAT = "<I I I I 32s I I 28s"  # magic,version,size,crc32,sha256,ts,secver,reserved
METADATA_SIZE = struct.calcsize(METADATA_FORMAT)   # 84 octets
METADATA_PAD = 256                     # le bloc métadonnées est paddé à 256 o
SIGNATURE_SIZE = 256                   # RSA-2048


# ---------------------------------------------------------------------------
# Primitives
# ---------------------------------------------------------------------------

def calculate_crc32(data: bytes) -> int:
    """CRC32 IEEE 802.3, identique à Calculate_CRC32() du bootloader."""
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0xEDB88320 if crc & 1 else crc >> 1
    return (~crc) & 0xFFFFFFFF


def calculate_sha256(data: bytes) -> bytes:
    return hashlib.sha256(data).digest()


def parse_version(version: str) -> int:
    """'1.2.3' -> 0x00010203"""
    major, minor, patch = (int(x) for x in version.split("."))
    return (major << 16) | (minor << 8) | patch


def load_private_key(path: Path):
    if not path.exists():
        raise FileNotFoundError(
            f"Clé privée introuvable : {path}\n"
            f"    Générez-la d'abord :  python tools/rsa_keygen.py"
        )
    return serialization.load_pem_private_key(path.read_bytes(), password=None)


# ---------------------------------------------------------------------------
# Metadata + signature
# ---------------------------------------------------------------------------

def build_metadata(firmware: bytes, version: str, security_version: int) -> bytes:
    metadata = struct.pack(
        METADATA_FORMAT,
        FIRMWARE_MAGIC,
        parse_version(version),
        len(firmware),
        calculate_crc32(firmware),
        calculate_sha256(firmware),
        int(time.time()),
        security_version,
        b"\x00" * 28,
    )
    assert len(metadata) == METADATA_SIZE
    return metadata


def sign_firmware(private_key, firmware: bytes) -> bytes:
    """Signature RSA-2048 PKCS#1 v1.5 sur SHA-256(firmware). 256 octets big-endian."""
    signature = private_key.sign(firmware, padding.PKCS1v15(), hashes.SHA256())
    assert len(signature) == SIGNATURE_SIZE, f"taille signature inattendue: {len(signature)}"
    return signature


def package(firmware_path: Path, output_path: Path, key_path: Path,
            version: str, security_version: int) -> bool:
    firmware = firmware_path.read_bytes()
    if len(firmware) == 0:
        print("[!] Firmware vide")
        return False
    if len(firmware) > MAX_FIRMWARE_SIZE:
        print(f"[!] Firmware trop gros ({len(firmware)} > {MAX_FIRMWARE_SIZE} o)")
        return False

    print(f"[+] Firmware        : {firmware_path} ({len(firmware)} o)")
    private_key = load_private_key(key_path)

    metadata = build_metadata(firmware, version, security_version)
    signature = sign_firmware(private_key, firmware)

    # Bloc de boot = metadata paddée à 256 o + signature 256 o
    boot_block = metadata + b"\x00" * (METADATA_PAD - len(metadata)) + signature
    output_path.write_bytes(boot_block)

    sha = calculate_sha256(firmware)
    print(f"[+] Version         : {version}  (security_version={security_version})")
    print(f"[+] CRC32           : 0x{calculate_crc32(firmware):08X}")
    print(f"[+] SHA-256         : {sha.hex()}")
    print(f"[+] Signature RSA   : {len(signature)} o (PKCS#1 v1.5 / SHA-256)")
    print(f"[+] Bloc de boot    : {output_path} ({len(boot_block)} o) -> flash @ 0x0800E000")

    meta_json = {
        "magic": f"0x{FIRMWARE_MAGIC:08X}",
        "version": version,
        "security_version": security_version,
        "size": len(firmware),
        "crc32": f"0x{calculate_crc32(firmware):08X}",
        "sha256": sha.hex(),
        "signature_type": "RSA-2048 PKCS#1 v1.5 (SHA-256)",
        "signature": signature.hex(),
    }
    json_path = output_path.with_name(output_path.stem + "_metadata.json")
    json_path.write_text(json.dumps(meta_json, indent=4))
    print(f"[+] Métadonnées     : {json_path}")
    print("\n[✓] Firmware signé avec succès.")
    return True


def verify(boot_block_path: Path, firmware_path: Path, key_path: Path) -> bool:
    """Vérifie un bloc de boot contre le firmware et la clé publique (côté PC)."""
    data = boot_block_path.read_bytes()
    metadata = data[:METADATA_SIZE]
    signature = data[METADATA_PAD:METADATA_PAD + SIGNATURE_SIZE]

    (magic, version, size, crc32_stored, sha_stored,
     _ts, secver, _res) = struct.unpack(METADATA_FORMAT, metadata)

    firmware = firmware_path.read_bytes()
    ok = True

    if magic != FIRMWARE_MAGIC:
        print(f"[!] Magic invalide : 0x{magic:08X}"); ok = False
    else:
        print("[✓] Magic OK")

    if size != len(firmware):
        print(f"[!] Taille {size} != firmware {len(firmware)}"); ok = False
    if calculate_crc32(firmware) != crc32_stored:
        print("[!] CRC32 mismatch"); ok = False
    else:
        print("[✓] CRC32 OK")
    if calculate_sha256(firmware) != sha_stored:
        print("[!] SHA-256 mismatch"); ok = False
    else:
        print("[✓] SHA-256 OK")

    public_key = load_private_key(key_path).public_key()
    try:
        public_key.verify(signature, firmware, padding.PKCS1v15(), hashes.SHA256())
        print("[✓] Signature RSA OK")
    except InvalidSignature:
        print("[!] Signature RSA INVALIDE"); ok = False

    print(f"\n{'[✓✓✓] VÉRIFICATION RÉUSSIE' if ok else '[✗] VÉRIFICATION ÉCHOUÉE'}"
          f"  (v{version >> 16 & 0xFF}.{version >> 8 & 0xFF}.{version & 0xFF}, "
          f"secver={secver})")
    return ok


def main() -> int:
    p = argparse.ArgumentParser(description="Signe/package un firmware STM32 (RSA-2048)")
    p.add_argument("input", help="Firmware .bin (ou bloc de boot en mode --verify)")
    p.add_argument("-o", "--output", default="app_signed.bin", help="Bloc de boot de sortie")
    p.add_argument("-v", "--version", default="1.0.0", help="Version sémantique (ex: 1.2.3)")
    p.add_argument("-s", "--security-version", type=int, default=1,
                   help="Compteur anti-rollback monotone (défaut: 1)")
    p.add_argument("-k", "--key", default=str(DEFAULT_PRIVATE_KEY), help="Clé privée PEM")
    p.add_argument("--verify", action="store_true", help="Vérifie un bloc de boot déjà signé")
    p.add_argument("--firmware", help="Firmware original (requis avec --verify)")
    args = p.parse_args()

    if args.verify:
        if not args.firmware:
            p.error("--verify nécessite --firmware <app.bin>")
        ok = verify(Path(args.input), Path(args.firmware), Path(args.key))
    else:
        ok = package(Path(args.input), Path(args.output), Path(args.key),
                     args.version, args.security_version)
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
