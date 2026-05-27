#include <cstring>
#include <Arduino.h>
//#include <EEPROM.h>
#include <LittleFS.h>
#include <cstdlib>
//#include "mbedtls/sha256.h"
#include "hardware/structs/scb.h"

#define HIGH (uint8_t)0x1
#define LOW  (uint8_t)0x0
#define FLASH_BASE ((const uint8_t*)0x10000000)
#define RAM_BASE ((const uint8_t*)0x20000000)
#define PASSWORD "pipi"

void pinWrite(uint8_t pinN, uint8_t value) {
    digitalWrite(pinN, value);
}

uint8_t pinRead(uint8_t pinN) {
    return (uint8_t)digitalRead(pinN);
}

typedef struct {
    volatile uint32_t CPUID;
    volatile uint32_t ICSR;
    volatile uint32_t VTOR;
    volatile uint32_t AIRCR;
    volatile uint32_t SCR;
    volatile uint32_t CCR;
    volatile uint32_t SHPR[3];
    volatile uint32_t SHCSR;
    volatile uint32_t CFSR;
    volatile uint32_t HFSR;
    volatile uint32_t DFSR;
    volatile uint32_t MMFAR;
    volatile uint32_t BFAR;
} scb_full_t;

#define SCB ((scb_full_t*)0xE000ED00)

// void sha256(const uint8_t *data, size_t len, uint8_t output[32]) {
//     mbedtls_sha256_context ctx;

//     mbedtls_sha256_init(&ctx);
//     mbedtls_sha256_starts_ret(&ctx, 0); // 0 = SHA-256 (not SHA-224)
//     mbedtls_sha256_update_ret(&ctx, data, len);
//     mbedtls_sha256_finish_ret(&ctx, output);
//     mbedtls_sha256_free(&ctx);
// }

char buf[256];

void serial_ReadLine(bool password = false) {
    char c;
    uint8_t idx = 0;
    while (true) {
        while (!Serial.available()) delay(1);
        c = (char)Serial.read();

        if (c == '\r' || c == '\n') {
            buf[idx] = '\0';
            Serial.println();
            break;
        } else if (c == '\b' || c == 127) {
            buf[idx--] = '\0';
            Serial.print("\b \b");
        } else {
            buf[idx++] = c;
            if (password) Serial.print("*");
            else Serial.print(c);
        }
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(1);

    while (true) {
        Serial.print(F("Enter PI-DOS password: "));
        serial_ReadLine(true);
        if (!strcmp(buf, PASSWORD)) {
            Serial.println(F("Password correct, logging in."));
            break;
        } else {
            Serial.println(F("Password incorrect, try again."));
        }
    }

    LittleFS.begin();
}

void loop() {
    Serial.print("> ");

    serial_ReadLine();

    char *cmd = strtok(buf, "|");
    if (!strcmp(cmd, "echo")) {
        char *arg1 = strtok(NULL, "|");
        Serial.print("< ");
        Serial.println(arg1);
        
    } else if (!strcmp(cmd, "ping")) {
        Serial.println("< pong! :3");

    } else if (!strcmp(cmd, "memdump")) {
        char *arg1 = strtok(NULL, "|");

        char *arg2 = strtok(NULL, "|");
        long start = strtol(arg2, nullptr, 0);

        char *arg3 = strtok(NULL, "|");
        long size = strtol(arg3, nullptr, 0);
        if (!strcmp(arg1, "flash")) {
            for (long i = 0; i < size; i++) {
                Serial.print(*(FLASH_BASE + start + i), HEX); Serial.print(" ");
            }
            Serial.println();

        } else if (!strcmp(arg1, "ram")) {
            for (long i = 0; i < size; i++) {
                Serial.print(*(RAM_BASE + start + i), HEX); Serial.print(" ");
            }
            Serial.println();

        } else {
            Serial.println(F("< name is not mapped in memory space"));
            Serial.println(F("< go read https://en.wikipedia.org/wiki/Memory_management_unit to understand why"));

        }
    } else if (!strcmp(cmd, "pin")) {
        char *arg1 = strtok(NULL, "|");
        uint8_t pin = (uint8_t)atoi(arg1);

        char *arg2 = strtok(NULL, "|");
        char *arg3 = strtok(NULL, "|");

        if (!strcmp(arg2, "set")) {
            if (!strcmp(arg3, "in")) {
                pinMode(pin, INPUT_PULLDOWN);
            } else if (!strcmp(arg3, "out")) {
                pinMode(pin, OUTPUT);
            } else {
                Serial.println(F("< pin mode not found, go to https://my.clevelandclinic.org/health/diseases/6005-dyslexia to know why."));
            }

        } else if (!strcmp(arg2, "read")) {
            Serial.println(digitalRead(pin), HEX);

        } else if (!strcmp(arg2, "write")) {
            if (!strcmp(arg3, "high")) {
                digitalWrite(pin, HIGH);
            } else if (!strcmp(arg3, "low")) {
                digitalWrite(pin, LOW);
            }
        } else if (!strcmp(arg2, "oscill")) {
            long freq = strtol(arg3, nullptr, 0);
            if (freq == 0) noTone(pin);
            else tone(pin, freq);
        }

    } else if (!strcmp(cmd, "picofetch")) {
        unsigned long t = millis() / 1000;

        unsigned int sec = t % 60;
        unsigned int min = (t / 60) % 60;
        unsigned int hr  = (t / 3600) % 24;
        unsigned int day = (t / 86400);

        Serial.println(F("\x1b[92m       .~~.   .~~.       \x1b[0mOS     | PI-DOS 1.0"));
        Serial.println(F("\x1b[92m      '. \\ ' ' / .'      \x1b[0mRAM    | 520KB"));
        Serial.println(F("\x1b[91m       .~ .~~~..~.       \x1b[0mFlash  | 4MB"));
        Serial.println(F("\x1b[91m      : .~.'~'.~. :      \x1b[0mCPU1   | ARM Cortex-M33 (Dual-core)"));
        Serial.println(F("\x1b[91m     ~ (   ) (   ) ~     \x1b[0mCPU2   | RISC-V Hazard3 (Dual-core)"));
        Serial.println(F("\x1b[91m    ( : '~'.~.'~' : )    \x1b[0mDevice | RP2350"));
        Serial.println(F("\x1b[91m     ~ .~ (   ) ~. ~     \x1b[0mFS     | LittleFS"));
          Serial.print(F("\x1b[91m      (  : '~' :  )      \x1b[0mUptime | ")); Serial.print(day); Serial.print("d "); Serial.print(hr); Serial.print("h "); Serial.print(min); Serial.print("m "); Serial.print(sec); Serial.println("s");
        Serial.println(F("\x1b[91m       '~ .~~~. ~'"));
        Serial.println(F("           '~'\x1b[0m"));
    } else if (!strcmp(cmd, "cat")) {
        Serial.println(F(" /ᐠ｡ꞈ｡ᐟ\\"));

    } else if (!strcmp(cmd, "clear")) {
        Serial.print(F("\x1b[2J"));
        Serial.print("\x1b[H");

    } else if (!strcmp(cmd, "filewr")) {
        char *arg1 = strtok(NULL, "|");

        File f = LittleFS.open(arg1, "w");
        while (true) {
            serial_ReadLine();
            if (!strcmp(buf, ".q")) break;
            f.println(buf);
        }
        f.close();

    } else if (!strcmp(cmd, "filerd")) {
        char *arg1 = strtok(NULL, "|");

        File f = LittleFS.open(arg1, "r");

        while (f.available()) {
            String line = f.readStringUntil('\n');
            Serial.println(line);
        }

        f.close();
    } else if (!strcmp(cmd, "ringtone")) {
        tone(3, 1000);
        delay(150);
        tone(3, 900);
        delay(150);
        tone(3, 700);
        delay(250);
        noTone(3);

        delay(100);

        tone(3, 1000);
        delay(150);
        tone(3, 900);
        delay(150);
        tone(3, 700);
        delay(250);
        noTone(3);
    } else {
        Serial.print(F("< command '")); 
        Serial.print(cmd);
        Serial.println(F("' not found, go to https://my.clevelandclinic.org/health/diseases/6005-dyslexia to know why."));
    }
}