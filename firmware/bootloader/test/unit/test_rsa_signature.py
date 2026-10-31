"""
Tests Unitaires - Vérification de Signature RSA-2048
Teste le VRAI code C de lib/crypto/rsa_light.c (rsa_pkcs1_sha256_verify).

Le modèle de menaces exige que le bootloader n'accepte QUE des images signées
par le détenteur de la clé privée. On prouve ici :
  - une signature valide est acceptée
  - une image modifiée (hash différent) est rejetée
  - une signature corrompue est rejetée
  - une signature produite par une AUTRE clé est rejetée
  - une signature de mauvaise taille est rejetée
"""

import ctypes
import hashlib

import pytest

RSA_NUM_BYTES = 256


def _verify(lib, c_public_key, signature: bytes, digest: bytes) -> int:
    sig = (ctypes.c_uint8 * len(signature))(*signature)
    dig = (ctypes.c_uint8 * len(digest))(*digest)
    return lib.rsa_pkcs1_sha256_verify(
        ctypes.byref(c_public_key), sig, len(signature), dig
    )


@pytest.fixture
def firmware():
    # Image de test déterministe et non triviale
    return b"STM32 SECURE FIRMWARE\n" + bytes(range(256)) * 8


@pytest.mark.unit
@pytest.mark.crypto
class TestRSASignatureVerify:
    """Vérification RSA-2048 PKCS#1 v1.5 sur SHA-256."""

    def test_valid_signature_accepted(self, bootloader_lib, c_public_key,
                                      sign_firmware, firmware):
        sig = sign_firmware(firmware)
        digest = hashlib.sha256(firmware).digest()
        assert _verify(bootloader_lib, c_public_key, sig, digest) == 1

    def test_tampered_firmware_rejected(self, bootloader_lib, c_public_key,
                                        sign_firmware, firmware):
        """Signature authentique, mais image modifiée -> hash ne correspond plus."""
        sig = sign_firmware(firmware)
        tampered = bytearray(firmware)
        tampered[1000] ^= 0xFF
        digest = hashlib.sha256(bytes(tampered)).digest()
        assert _verify(bootloader_lib, c_public_key, sig, digest) == 0

    def test_corrupted_signature_rejected(self, bootloader_lib, c_public_key,
                                          sign_firmware, firmware):
        sig = bytearray(sign_firmware(firmware))
        sig[128] ^= 0x01  # un seul bit
        digest = hashlib.sha256(firmware).digest()
        assert _verify(bootloader_lib, c_public_key, bytes(sig), digest) == 0

    def test_wrong_key_rejected(self, bootloader_lib, c_public_key, firmware):
        """Image signée par une clé différente de celle embarquée."""
        from cryptography.hazmat.primitives import hashes
        from cryptography.hazmat.primitives.asymmetric import padding, rsa

        attacker = rsa.generate_private_key(public_exponent=65537, key_size=2048)
        forged = attacker.sign(firmware, padding.PKCS1v15(), hashes.SHA256())
        digest = hashlib.sha256(firmware).digest()
        assert _verify(bootloader_lib, c_public_key, forged, digest) == 0

    def test_bad_length_rejected(self, bootloader_lib, c_public_key,
                                 sign_firmware, firmware):
        sig = sign_firmware(firmware)[:RSA_NUM_BYTES - 1]
        digest = hashlib.sha256(firmware).digest()
        assert _verify(bootloader_lib, c_public_key, sig, digest) == 0

    def test_zero_signature_rejected(self, bootloader_lib, c_public_key, firmware):
        digest = hashlib.sha256(firmware).digest()
        assert _verify(bootloader_lib, c_public_key, b"\x00" * RSA_NUM_BYTES, digest) == 0


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
