/**
 * ============================================================================
 * SECURE BOOTLOADER - STM32F103 - VERSION FINALE STABLE
 * ============================================================================
 */

#include "stm32f1xx_hal.h"
#include <string.h>
#include "crypto_light.h"
#include "rsa_light.h"
#include "antirollback.h"
#include "rsa_public_key.h"   /* g_firmware_public_key, généré par rsa_keygen.py */

#define APPLICATION_ADDRESS  0x08002000
#define APPLICATION_MAX_SIZE 0xC000
#define METADATA_ADDR        0x0800E000               /* metadata (84 o) + padding */
#define SIGNATURE_ADDR       (METADATA_ADDR + 0x100)  /* signature RSA-2048 @0x0800E100 */
#define ANTIROLLBACK_ADDR    0x0800E800               /* page Flash dédiée (1 Ko)  */

#define LED_PORT GPIOC
#define LED_PIN  GPIO_PIN_13

/*
 * Codes LED (LED_Error_Loop) :
 *   1 = magic invalide          4 = signature RSA invalide
 *   2 = taille / CRC32 KO       5 = stack pointer invalide
 *   3 = SHA-256 KO              6 = anti-rollback (version refusée)
 */

typedef struct {
    uint32_t magic;
    uint32_t version;           /* version sémantique d'affichage           */
    uint32_t size;
    uint32_t crc32;
    uint8_t  sha256[32];
    uint32_t timestamp;
    uint32_t security_version;  /* compteur monotone anti-rollback          */
    uint8_t  reserved[28];
} __attribute__((packed)) FirmwareMetadata_t;

void SystemClock_Config(void);
void GPIO_Init(void);
void LED_Blink(uint32_t count, uint32_t on_ms, uint32_t off_ms);
void LED_Error_Loop(uint32_t pattern);
uint8_t Verify_Firmware(void);
void Jump_To_Application(void) __attribute__((noreturn));
uint32_t Calculate_CRC32(const uint8_t *data, uint32_t length);
static void Antirollback_Commit(uint32_t new_security_version);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    HAL_Delay(100);
    
    LED_Blink(2, 100, 100);
    HAL_Delay(500);
    
    if (Verify_Firmware()) {
        LED_Blink(3, 200, 200);
        HAL_Delay(200);
        Jump_To_Application();
    }
    
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    while(1) __WFI();
}

uint8_t Verify_Firmware(void) {
    FirmwareMetadata_t *metadata  = (FirmwareMetadata_t*)METADATA_ADDR;
    const uint8_t      *signature = (const uint8_t*)SIGNATURE_ADDR;

    /* 1. Entête valide */
    if (metadata->magic != 0xDEADBEEF) {
        LED_Error_Loop(1);
        return 0;
    }

    /* 2. Taille plausible */
    if (metadata->size == 0 || metadata->size > APPLICATION_MAX_SIZE) {
        LED_Error_Loop(2);
        return 0;
    }

    /* 3. Vecteur de pile cohérent (pré-filtre bas niveau) */
    uint32_t stack_pointer = *(__IO uint32_t*)APPLICATION_ADDRESS;
    if ((stack_pointer & 0x2FFE0000) != 0x20000000) {
        LED_Error_Loop(5);
        return 0;
    }

    /* 4. Anti-rollback : refuser une security_version < compteur stocké.
     *    Vérifié AVANT tout calcul coûteux pour couper court à un rejeu. */
    uint32_t stored_sv = *(__IO uint32_t*)ANTIROLLBACK_ADDR;
    if (!antirollback_allowed(metadata->security_version, stored_sv)) {
        LED_Error_Loop(6);
        return 0;
    }

    /* 5. CRC32 : garde-fou intégrité peu coûteux (erreurs de flash, coupures) */
    uint8_t *firmware = (uint8_t*)APPLICATION_ADDRESS;
    if (Calculate_CRC32(firmware, metadata->size) != metadata->crc32) {
        LED_Error_Loop(2);
        return 0;
    }

    /* 6. Condensé SHA-256 de l'image */
    uint8_t calculated_hash[32];
    sha256_hash(firmware, metadata->size, calculated_hash);

    /* 7. AUTHENTICITÉ : la signature RSA doit couvrir CE hash.
     *    C'est le cœur du secure boot : sans la clé privée, impossible de
     *    forger une signature valide pour une image modifiée. */
    if (!rsa_pkcs1_sha256_verify(&g_firmware_public_key,
                                 signature, RSA_NUM_BYTES, calculated_hash)) {
        LED_Error_Loop(4);
        return 0;
    }

    /* 8. Image authentique et au moins aussi récente : on scelle la nouvelle
     *    security_version si elle a progressé (le compteur ne recule jamais). */
    if (antirollback_should_commit(metadata->security_version, stored_sv)) {
        Antirollback_Commit(metadata->security_version);
    }

    return 1;
}

/*
 * Écrit la nouvelle security_version dans la page Flash dédiée.
 * Le compteur est monotone : on n'efface/reprogramme que pour le faire avancer.
 */
static void Antirollback_Commit(uint32_t new_security_version) {
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {0};
    erase.TypeErase   = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = ANTIROLLBACK_ADDR;
    erase.NbPages     = 1;
    uint32_t page_error = 0;

    if (HAL_FLASHEx_Erase(&erase, &page_error) == HAL_OK) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                          ANTIROLLBACK_ADDR, new_security_version);
    }

    HAL_FLASH_Lock();
}

void Jump_To_Application(void) {
    __disable_irq();
    
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    
    for (uint8_t i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }
    
    // NE TOUCHE PAS RCC - l'application le fera
    
    SCB->VTOR = APPLICATION_ADDRESS;
    __DSB();
    __ISB();
    
    uint32_t app_stack = *(__IO uint32_t*)APPLICATION_ADDRESS;
    uint32_t app_reset = *(__IO uint32_t*)(APPLICATION_ADDRESS + 4);
    
    __set_MSP(app_stack);
    __DSB();
    __ISB();
    
    void (*app_reset_handler)(void) = (void (*)(void))app_reset;
    app_reset_handler();
    
    while(1);
}

uint32_t Calculate_CRC32(const uint8_t *data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320) : (crc >> 1);
        }
    }
    return ~crc;
}

void LED_Blink(uint32_t count, uint32_t on_ms, uint32_t off_ms) {
    for (uint32_t i = 0; i < count; i++) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        HAL_Delay(on_ms);
        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        if (i < count - 1) HAL_Delay(off_ms);
    }
}

void LED_Error_Loop(uint32_t pattern) {
    while(1) {
        if (pattern == 1) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
            HAL_Delay(2000);
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
            HAL_Delay(500);
        } else if (pattern == 2) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
            HAL_Delay(1000);
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
            HAL_Delay(200);
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
            HAL_Delay(300);
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
            HAL_Delay(500);
        } else {
            LED_Blink(pattern, 200, 200);
            HAL_Delay(1000);
        }
    }
}

void GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
    
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

void SysTick_Handler(void) {
    HAL_IncTick();
}