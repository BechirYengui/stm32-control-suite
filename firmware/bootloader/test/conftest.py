"""
Configuration pytest globale pour tests bootloader
Fixtures réutilisables pour tous les tests
"""

import sys
import pytest
import ctypes
from pathlib import Path

# Rend les outils (rsa_c_key.py) importables depuis les tests
TOOLS_DIR = Path(__file__).parent.parent / 'tools'
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))


# ============================================================================
# Structure ctypes miroir de RSAPublicKey (lib/crypto/rsa_light.h)
# ============================================================================

RSA_NUM_WORDS = 64


class RSAPublicKey(ctypes.Structure):
    _fields_ = [
        ("len", ctypes.c_int),
        ("n0inv", ctypes.c_uint32),
        ("n", ctypes.c_uint32 * RSA_NUM_WORDS),
        ("rr", ctypes.c_uint32 * RSA_NUM_WORDS),
    ]


# ============================================================================
# Fixture: Bibliothèque Bootloader Compilée
# ============================================================================

@pytest.fixture(scope="session")
def bootloader_lib():
    """
    Charge la bibliothèque bootloader compilée (.so)
    
    Usage:
        def test_crc32(bootloader_lib):
            result = bootloader_lib.Calculate_CRC32(data, len(data))
    """
    lib_path = Path(__file__).parent / 'bindings' / 'libbootloader.so'
    
    if not lib_path.exists():
        pytest.skip(f"Bibliothèque non trouvée: {lib_path}")
    
    lib = ctypes.CDLL(str(lib_path))
    
    # Configure CRC32
    lib.Calculate_CRC32.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint32]
    lib.Calculate_CRC32.restype = ctypes.c_uint32
    
    # Configure SHA-256
    lib.sha256_hash.argtypes = [
        ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint8)
    ]
    lib.sha256_hash.restype = None

    # Configure la vérification de signature RSA-2048 PKCS#1 v1.5
    lib.rsa_pkcs1_sha256_verify.argtypes = [
        ctypes.POINTER(RSAPublicKey),
        ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint8),
    ]
    lib.rsa_pkcs1_sha256_verify.restype = ctypes.c_int

    # Configure l'anti-rollback
    lib.antirollback_normalize.argtypes = [ctypes.c_uint32]
    lib.antirollback_normalize.restype = ctypes.c_uint32
    lib.antirollback_allowed.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.antirollback_allowed.restype = ctypes.c_uint8
    lib.antirollback_should_commit.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    lib.antirollback_should_commit.restype = ctypes.c_uint8

    return lib


# ============================================================================
# Fixtures: Clé RSA de test + helpers de signature
# ============================================================================

@pytest.fixture(scope="session")
def rsa_private_key():
    """Paire de clés RSA-2048 éphémère (jamais écrite sur disque)."""
    crypto = pytest.importorskip("cryptography")  # skip propre si absent
    from cryptography.hazmat.primitives.asymmetric import rsa
    return rsa.generate_private_key(public_exponent=65537, key_size=2048)


@pytest.fixture(scope="session")
def c_public_key(rsa_private_key):
    """Construit un RSAPublicKey (ctypes) à partir de la clé de test."""
    from rsa_c_key import c_key_fields
    n = rsa_private_key.public_key().public_numbers().n
    f = c_key_fields(n)
    return RSAPublicKey(
        RSA_NUM_WORDS,
        f["n0inv"],
        (ctypes.c_uint32 * RSA_NUM_WORDS)(*f["n"]),
        (ctypes.c_uint32 * RSA_NUM_WORDS)(*f["rr"]),
    )


@pytest.fixture
def sign_firmware(rsa_private_key):
    """Retourne une fonction qui signe des octets (RSA-2048 PKCS#1 v1.5 / SHA-256)."""
    from cryptography.hazmat.primitives import hashes
    from cryptography.hazmat.primitives.asymmetric import padding

    def _sign(data: bytes) -> bytes:
        return rsa_private_key.sign(data, padding.PKCS1v15(), hashes.SHA256())

    return _sign


# ============================================================================
# Fixture: Firmware de Test
# ============================================================================

@pytest.fixture
def test_firmware_valid():
    """
    Retourne un firmware de test valide
    
    Usage:
        def test_verify(test_firmware_valid):
            assert verify_firmware(test_firmware_valid)
    """
    import struct
    
    # Crée un firmware valide
    sp = 0x20005000  # Stack pointer en RAM
    reset = 0x08002001  # Reset vector avec Thumb bit
    
    firmware = struct.pack('<II', sp, reset)
    firmware += b'\x00' * 1000  # Code simulé
    
    return firmware


@pytest.fixture
def test_firmware_corrupted(test_firmware_valid):
    """
    Retourne un firmware corrompu
    
    Usage:
        def test_detect_corruption(test_firmware_corrupted):
            assert not verify_firmware(test_firmware_corrupted)
    """
    firmware = bytearray(test_firmware_valid)
    firmware[500] ^= 0xFF  # Corrompt 1 byte
    return bytes(firmware)


# ============================================================================
# Fixture: Métadonnées
# ============================================================================

@pytest.fixture
def valid_metadata():
    """
    Retourne des métadonnées valides
    
    Usage:
        def test_metadata(valid_metadata):
            assert valid_metadata['magic'] == 0xDEADBEEF
    """
    return {
        'magic': 0xDEADBEEF,
        'version': 0x00010203,  # 1.2.3
        'size': 1024,
        'crc32': 0x12345678,
        'sha256': b'\x00' * 32,
    }


# ============================================================================
# Fixture: Constantes Bootloader
# ============================================================================

@pytest.fixture
def bootloader_constants():
    """
    Constantes du bootloader
    
    Usage:
        def test_address(bootloader_constants):
            assert address == bootloader_constants['BOOTLOADER_START']
    """
    return {
        'BOOTLOADER_START': 0x08000000,
        'BOOTLOADER_SIZE': 8 * 1024,
        'APPLICATION_START': 0x08002000,
        'APPLICATION_MAX_SIZE': 48 * 1024,
        'METADATA_ADDR': 0x0800E000,
        'FIRMWARE_MAGIC': 0xDEADBEEF,
        'RAM_START': 0x20000000,
        'RAM_END': 0x20005000,
    }


# ============================================================================
# Helper Functions
# ============================================================================

def bytes_to_c_array(data: bytes):
    """Convertit bytes Python en tableau C uint8_t[]"""
    return (ctypes.c_uint8 * len(data))(*data)
