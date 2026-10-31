# Modèle de menaces — Secure Boot STM32F103

> Écrit pour cadrer ce que le secure boot doit réellement empêcher, **avant** le
> code de vérification. Une protection qu'on n'a pas tenté de casser n'est qu'une
> intention : chaque menace listée ici est éprouvée par un test automatisé
> (voir la colonne « Preuve »).
>
> **Modèle technique (niveau produit).** Pour l'analyse de risque au niveau
> système/organisationnel selon la méthode **EBIOS Risk Manager (ANSSI)**, voir
> [`EBIOS_RM.md`](EBIOS_RM.md) (les menaces T1–T5 ci-dessous y alimentent les
> scénarios opérationnels, cf. son annexe A).

## 1. Périmètre et biens à protéger

| Bien | Pourquoi |
|------|----------|
| **Intégrité de l'application** (`0x08002000`) | Le code exécuté doit être exactement celui publié par le propriétaire. |
| **Authenticité de l'application** | Seul le détenteur de la clé privée peut autoriser un firmware. |
| **Clé privée de signature** | Sa compromission casse toute la chaîne. Elle vit hors de la carte (idéalement HSM), jamais dans le dépôt. |
| **Monotonie de la version de sécurité** | Empêcher le retour à une version vulnérable déjà corrigée. |

Le **bootloader** (`0x08000000`, 8 Ko) est la racine de confiance : on le suppose
intègre (flashé en usine, idéalement protégé en écriture par les *option bytes*).

## 2. Modèle d'adversaire

- **Capacités** : peut reflasher la zone application et la zone métadonnées via
  SWD/ST-Link ou via un canal de mise à jour ; peut lire le contenu de la Flash ;
  peut rejouer d'anciennes images légitimes.
- **Hors périmètre** (hypothèses explicites) :
  - Attaques physiques invasives (decap, *fault injection*, *glitching*).
  - Canaux auxiliaires (temps/puissance) — hors le soin pris de comparer le hash
    en temps constant.
  - Compromission de la clé privée ou du poste de signature.
  - Verrouillage RDP/WRP des *option bytes* (recommandé en production, non couvert
    par le code ici).

## 3. Menaces, mitigations et preuves

| # | Menace | Scénario | Mitigation | Preuve (test) |
|---|--------|----------|------------|---------------|
| T1 | **Altération de l'image** | L'attaquant modifie quelques octets du firmware. | SHA-256 recalculé puis **signature RSA-2048** vérifiée sur ce hash : le hash change, la signature ne correspond plus. | `test_rsa_signature.py::test_tampered_firmware_rejected` |
| T2 | **Substitution par une image forgée** | L'attaquant flashe son propre firmware et génère métadonnées + « signature ». | Sans la clé privée, impossible de produire une signature valide pour la clé publique embarquée. | `test_rsa_signature.py::test_wrong_key_rejected`, `test_zero_signature_rejected` |
| T3 | **Rejeu d'une version antérieure** | L'attaquant reflashe une ancienne image **authentique** dont la faille est corrigée depuis. | Compteur `security_version` **monotone** stocké en Flash dédiée ; le bootloader refuse toute version strictement inférieure et ne le fait jamais reculer. | `test_antirollback.py` (tout le fichier) |
| T4 | **Corruption non malveillante** | Coupure pendant le flash, usure Flash. | Pré-filtres : magic, taille, vecteur de pile, **CRC32** — échec rapide avant la crypto. | `test_verification.py`, `test_rsa_signature.py::test_corrupted_signature_rejected` |
| T5 | **Saut vers une image invalide** | Vecteur de reset/pile incohérent. | Contrôle `SP & 0x2FFE0000 == 0x20000000` avant tout saut. | `test_verification.py::TestStackPointerValidation` |

## 4. Chaîne de vérification (ordre voulu)

```
magic → taille → stack pointer → anti-rollback → CRC32 → SHA-256 → signature RSA → (commit security_version) → saut
```

L'anti-rollback est placé **avant** les calculs coûteux (hash/RSA) pour couper
court à un rejeu au plus tôt. La signature RSA est le **dernier verrou** : c'est
elle qui apporte l'authenticité ; les étapes précédentes sont des garde-fous.

## 5. Détails cryptographiques

- **Signature** : RSA-2048, PKCS#1 v1.5, condensé **SHA-256**. Exposant public
  figé à F4 = 65537.
- **Vérification embarquée** : arithmétique de Montgomery (base 2³², 64 mots),
  sans division ni `malloc` ; `n0inv` et `R² mod n` pré-calculés côté PC et
  embarqués dans la clé publique (`include/rsa_public_key.h`).
- **Comparaison du hash** en temps constant (pas de court-circuit sur le premier
  octet divergent).

## 6. Risques résiduels / pistes de durcissement

- **Clé privée de démo** : le dépôt fournit une clé générée localement pour que
  la chaîne soit reproductible. En production : clé en HSM, rotation, révocation.
- **Verrouillage Flash** : activer RDP niveau 1+ et WRP sur la page bootloader via
  les *option bytes* pour empêcher lecture/réécriture de la racine de confiance.
- **Anti-rollback & *option bytes*** : le compteur peut aussi être ancré dans une
  zone OTP/BKP pour résister à un effacement de masse.
- **PKCS#1 v1.5** : correct ici car exposant/padding fixes ; RSA-PSS ou Ed25519
  seraient un cran au-dessus pour un nouveau design.
- **Attaques physiques / fault injection** : non traitées (hors périmètre).
