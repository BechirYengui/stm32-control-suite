/**
 * ============================================================================
 * ANTI-ROLLBACK — PROTECTION CONTRE LE REJEU D'UNE VERSION ANTÉRIEURE
 * ============================================================================
 *
 * Menace couverte : un attaquant reflashe une ancienne image, correctement
 * signée à l'époque, dont une faille a depuis été corrigée. La signature RSA
 * seule l'accepterait (elle est authentique). On ajoute donc un compteur de
 * "security version" MONOTONE, stocké en Flash, que le bootloader n'accepte
 * jamais de faire reculer.
 *
 * Deux champs distincts :
 *   - version          : version sémantique d'affichage (1.2.3), libre.
 *   - security_version : compteur monotone, incrémenté UNIQUEMENT quand une
 *                        image corrige une faille de sécurité. C'est lui qui
 *                        est comparé au compteur stocké.
 *
 * Fonctions pures (aucun accès matériel) pour être testables sur PC.
 * ============================================================================
 */

#ifndef ANTIROLLBACK_H
#define ANTIROLLBACK_H

#include <stdint.h>

/* Valeur d'un mot de Flash effacé (STM32 : 0xFFFFFFFF). */
#define ANTIROLLBACK_ERASED 0xFFFFFFFFu

/**
 * Normalise le mot lu en Flash : une Flash vierge (0xFFFFFFFF) est interprétée
 * comme "aucune version encore enregistrée" = 0, de sorte qu'une carte neuve
 * accepte sa première image.
 */
uint32_t antirollback_normalize(uint32_t stored_flash_word);

/**
 * Décide si une image est autorisée vis-à-vis de l'anti-rollback.
 * @return 1 si fw_security_version >= compteur stocké (normalisé), 0 sinon.
 */
uint8_t antirollback_allowed(uint32_t fw_security_version,
                             uint32_t stored_flash_word);

/**
 * Indique s'il faut faire avancer le compteur stocké (strictement supérieur).
 * @return 1 si fw_security_version > compteur stocké (normalisé), 0 sinon.
 */
uint8_t antirollback_should_commit(uint32_t fw_security_version,
                                   uint32_t stored_flash_word);

#endif /* ANTIROLLBACK_H */
