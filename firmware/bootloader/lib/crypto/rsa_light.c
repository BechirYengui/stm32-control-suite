/**
 * ============================================================================
 * RSA-2048 PKCS#1 v1.5 — IMPLÉMENTATION (voir rsa_light.h)
 * ============================================================================
 */

#include "rsa_light.h"
#include <string.h>

/*
 * Préfixe ASN.1 DigestInfo pour SHA-256 (RFC 8017, EMSA-PKCS1-v1_5).
 *   SEQUENCE { SEQUENCE { OID sha256, NULL }, OCTET STRING (32) }
 */
static const uint8_t kSha256Asn1Prefix[] = {
    0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
    0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
    0x00, 0x04, 0x20
};
#define ASN1_PREFIX_LEN (sizeof(kSha256Asn1Prefix))
#define DIGEST_INFO_LEN (ASN1_PREFIX_LEN + 32)   /* 19 + 32 = 51 */

/* a[] -= n[] (soustraction du modulus, propagation d'emprunt) */
static void subM(const RSAPublicKey *key, uint32_t *a) {
    int64_t A = 0;
    int i;
    for (i = 0; i < key->len; ++i) {
        A += (uint64_t)a[i] - key->n[i];
        a[i] = (uint32_t)A;
        A >>= 32;
    }
}

/* Retourne 1 si a[] >= n[] */
static int geM(const RSAPublicKey *key, const uint32_t *a) {
    int i;
    for (i = key->len - 1; i >= 0; --i) {
        if (a[i] < key->n[i]) return 0;
        if (a[i] > key->n[i]) return 1;
    }
    return 1; /* égalité */
}

/* Montgomery : c[] += a * b[], puis réduction d'un mot */
static void montMulAdd(const RSAPublicKey *key,
                       uint32_t *c, const uint32_t a, const uint32_t *b) {
    uint64_t A = (uint64_t)a * b[0] + c[0];
    uint32_t d0 = (uint32_t)A * key->n0inv;
    uint64_t B = (uint64_t)d0 * key->n[0] + (uint32_t)A;
    int i;

    for (i = 1; i < key->len; ++i) {
        A = (A >> 32) + (uint64_t)a * b[i] + c[i];
        B = (B >> 32) + (uint64_t)d0 * key->n[i] + (uint32_t)A;
        c[i - 1] = (uint32_t)B;
    }

    A = (A >> 32) + (B >> 32);
    c[i - 1] = (uint32_t)A;

    if (A >> 32) {
        subM(key, c);
    }
}

/* Montgomery : c[] = a[] * b[] / R mod n */
static void montMul(const RSAPublicKey *key,
                    uint32_t *c, const uint32_t *a, const uint32_t *b) {
    int i;
    for (i = 0; i < key->len; ++i) {
        c[i] = 0;
    }
    for (i = 0; i < key->len; ++i) {
        montMulAdd(key, c, a[i], b);
    }
}

/*
 * Exponentiation modulaire publique avec e = F4 = 65537.
 * Entrée/sortie : inout[] = signature big-endian (RSA_NUM_BYTES octets),
 * remplacée in-place par s^65537 mod n (big-endian).
 */
static void modpowF4(const RSAPublicKey *key, uint8_t *inout) {
    uint32_t a[RSA_NUM_WORDS];
    uint32_t aR[RSA_NUM_WORDS];
    uint32_t aaR[RSA_NUM_WORDS];
    uint32_t *aaa = aaR; /* réutilise le tampon pour le résultat final */
    int i;

    /* big-endian octets -> little-endian mots */
    for (i = 0; i < key->len; ++i) {
        uint32_t tmp =
            ((uint32_t)inout[((key->len - 1 - i) * 4) + 0] << 24) |
            ((uint32_t)inout[((key->len - 1 - i) * 4) + 1] << 16) |
            ((uint32_t)inout[((key->len - 1 - i) * 4) + 2] <<  8) |
            ((uint32_t)inout[((key->len - 1 - i) * 4) + 3] <<  0);
        a[i] = tmp;
    }

    montMul(key, aR, a, key->rr);          /* aR = a * R mod n              */
    for (i = 0; i < 16; i += 2) {          /* 16 carrés -> aR = a^(2^16) R  */
        montMul(key, aaR, aR, aR);
        montMul(key, aR, aaR, aaR);
    }
    montMul(key, aaa, aR, a);              /* aaa = a^65537 mod n           */

    /* aaa peut dépasser n d'au plus 1 fois */
    if (geM(key, aaa)) {
        subM(key, aaa);
    }

    /* little-endian mots -> big-endian octets */
    for (i = key->len - 1; i >= 0; --i) {
        uint32_t tmp = aaa[i];
        *inout++ = (uint8_t)(tmp >> 24);
        *inout++ = (uint8_t)(tmp >> 16);
        *inout++ = (uint8_t)(tmp >>  8);
        *inout++ = (uint8_t)(tmp >>  0);
    }
}

int rsa_pkcs1_sha256_verify(const RSAPublicKey *key,
                            const uint8_t *signature, size_t sig_len,
                            const uint8_t sha256[32]) {
    uint8_t buf[RSA_NUM_BYTES];
    size_t i;
    size_t ps_end;
    uint8_t diff;

    if (key == NULL || signature == NULL || sha256 == NULL) return 0;
    if (key->len != RSA_NUM_WORDS) return 0;
    if (sig_len != RSA_NUM_BYTES)  return 0;

    memcpy(buf, signature, RSA_NUM_BYTES);
    modpowF4(key, buf);

    /*
     * buf doit désormais être le bloc encodé EMSA-PKCS1-v1_5 :
     *   00 01 FF..FF 00 <DigestInfo SHA-256> <hash 32o>
     */
    if (buf[0] != 0x00 || buf[1] != 0x01) return 0;

    /* Zone de padding 0xFF : indices 2 .. (fin - DIGEST_INFO_LEN - 1) exclus */
    ps_end = RSA_NUM_BYTES - DIGEST_INFO_LEN - 1; /* position du 0x00 séparateur */
    for (i = 2; i < ps_end; ++i) {
        if (buf[i] != 0xFF) return 0;
    }
    if (buf[ps_end] != 0x00) return 0;

    /* Préfixe ASN.1 */
    if (memcmp(&buf[ps_end + 1], kSha256Asn1Prefix, ASN1_PREFIX_LEN) != 0) return 0;

    /* Comparaison du hash en temps constant */
    diff = 0;
    for (i = 0; i < 32; ++i) {
        diff |= buf[ps_end + 1 + ASN1_PREFIX_LEN + i] ^ sha256[i];
    }

    return diff == 0;
}
