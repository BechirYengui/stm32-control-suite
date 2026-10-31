/**
 * ============================================================================
 * RSA-2048 PKCS#1 v1.5 — VÉRIFICATION DE SIGNATURE POUR STM32 BARE METAL
 * ============================================================================
 *
 * Vérifie l'AUTHENTICITÉ du firmware (et non seulement son intégrité) :
 * le hash SHA-256 de l'image est signé hors-ligne avec la clé PRIVÉE, et le
 * bootloader ne connaît que la clé PUBLIQUE. Un attaquant qui modifie l'image
 * ne peut plus produire de signature valide sans la clé privée.
 *
 * - Exposant public figé à F4 = 65537 (standard, 2 bits à 1 → 16 carrés + 1 mul)
 * - Arithmétique de Montgomery, base 2^32, 64 mots → modulo 2048 bits
 * - Pas de malloc, pas de division : n0inv et R^2 mod n sont pré-calculés
 *   côté PC (voir tools/rsa_keygen.py) et embarqués dans la clé publique.
 *
 * Coût : ~2 KB Flash, ~1 KB pile. Portage direct de la technique « mincrypt ».
 * ============================================================================
 */

#ifndef RSA_LIGHT_H
#define RSA_LIGHT_H

#include <stdint.h>
#include <stddef.h>

#define RSA_NUM_BYTES 256                         /* RSA-2048 = 256 octets */
#define RSA_NUM_WORDS (RSA_NUM_BYTES / 4)         /* 64 mots de 32 bits    */

/**
 * Clé publique RSA-2048 au format Montgomery.
 * Tous les tableaux sont en little-endian de mots 32 bits (n[0] = poids faible).
 * Généré par tools/rsa_keygen.py à partir d'une paire de clés OpenSSL/cryptography.
 */
typedef struct {
    int      len;                 /* Nombre de mots de n[] (doit valoir 64)   */
    uint32_t n0inv;               /* -1 / n[0] mod 2^32                        */
    uint32_t n[RSA_NUM_WORDS];    /* Modulus n                                 */
    uint32_t rr[RSA_NUM_WORDS];   /* R^2 mod n, avec R = 2^2048                */
} RSAPublicKey;

/**
 * Vérifie une signature RSA-2048 PKCS#1 v1.5 sur un condensé SHA-256.
 *
 * @param key       Clé publique embarquée.
 * @param signature Signature brute, big-endian, exactement RSA_NUM_BYTES octets.
 * @param sig_len   Longueur de la signature (doit valoir RSA_NUM_BYTES).
 * @param sha256    Condensé SHA-256 (32 octets) de l'image à authentifier.
 * @return 1 si la signature est valide, 0 sinon. Comparaison finale en temps
 *         constant pour ne pas fuir d'information par la durée.
 */
int rsa_pkcs1_sha256_verify(const RSAPublicKey *key,
                            const uint8_t *signature, size_t sig_len,
                            const uint8_t sha256[32]);

#endif /* RSA_LIGHT_H */
