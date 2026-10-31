/**
 * ============================================================================
 * ANTI-ROLLBACK — IMPLÉMENTATION (voir antirollback.h)
 * ============================================================================
 */

#include "antirollback.h"

uint32_t antirollback_normalize(uint32_t stored_flash_word) {
    /* Flash vierge => aucune version enregistrée => 0 */
    if (stored_flash_word == ANTIROLLBACK_ERASED) {
        return 0u;
    }
    return stored_flash_word;
}

uint8_t antirollback_allowed(uint32_t fw_security_version,
                             uint32_t stored_flash_word) {
    uint32_t stored = antirollback_normalize(stored_flash_word);
    return (fw_security_version >= stored) ? 1u : 0u;
}

uint8_t antirollback_should_commit(uint32_t fw_security_version,
                                   uint32_t stored_flash_word) {
    uint32_t stored = antirollback_normalize(stored_flash_word);
    return (fw_security_version > stored) ? 1u : 0u;
}
