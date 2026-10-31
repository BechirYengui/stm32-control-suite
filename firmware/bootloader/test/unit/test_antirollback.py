"""
Tests Unitaires - Anti-Rollback
Teste le VRAI code C de lib/boot/antirollback.c.

Menace couverte : rejeu d'une image ancienne, correctement signée à l'époque,
mais depuis corrigée. Le compteur monotone `security_version` ne doit jamais
reculer.
"""

import pytest

ERASED = 0xFFFFFFFF


@pytest.mark.unit
@pytest.mark.verification
class TestAntirollbackNormalize:
    """Une Flash vierge (0xFFFFFFFF) = aucune version enregistrée = 0."""

    def test_erased_flash_is_zero(self, bootloader_lib):
        assert bootloader_lib.antirollback_normalize(ERASED) == 0

    def test_written_value_unchanged(self, bootloader_lib):
        for v in (0, 1, 42, 1000, 0x7FFFFFFF):
            assert bootloader_lib.antirollback_normalize(v) == v


@pytest.mark.unit
@pytest.mark.verification
class TestAntirollbackAllowed:
    """fw_security_version >= compteur stocké."""

    def test_newer_version_allowed(self, bootloader_lib):
        assert bootloader_lib.antirollback_allowed(5, 3) == 1

    def test_same_version_allowed(self, bootloader_lib):
        assert bootloader_lib.antirollback_allowed(3, 3) == 1

    def test_older_version_rejected(self, bootloader_lib):
        assert bootloader_lib.antirollback_allowed(2, 3) == 0

    def test_first_boot_on_blank_flash_allowed(self, bootloader_lib):
        # Carte neuve : stored = erased -> toute version >= 0 passe
        assert bootloader_lib.antirollback_allowed(0, ERASED) == 1
        assert bootloader_lib.antirollback_allowed(7, ERASED) == 1


@pytest.mark.unit
@pytest.mark.verification
class TestAntirollbackCommit:
    """On ne fait avancer le compteur que si la version progresse strictement."""

    def test_commit_only_when_strictly_greater(self, bootloader_lib):
        assert bootloader_lib.antirollback_should_commit(5, 3) == 1
        assert bootloader_lib.antirollback_should_commit(3, 3) == 0
        assert bootloader_lib.antirollback_should_commit(2, 3) == 0

    def test_commit_on_blank_flash(self, bootloader_lib):
        # stored = erased (=0) ; commit seulement si fw > 0
        assert bootloader_lib.antirollback_should_commit(1, ERASED) == 1
        assert bootloader_lib.antirollback_should_commit(0, ERASED) == 0


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
