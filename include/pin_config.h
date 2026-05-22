// pin_config.h
// Hardware pin assignments and serial port mappings for Amidala Firmware.
// All physical I/O pin numbers and serial port aliases are defined here so
// that wiring changes require edits in exactly one place.
//
// Target: ESP32-S3 WROOM1 N16R8 — Amidala custom PCB.
//
// Note: drive-system serial symbols (DOME_DRIVE_SERIAL, RDH_SERIAL) come from
// drive_config.h and must be resolved before this header is included.

#pragma once

// ---- Servo output pins (LEDC PWM via ServoDispatchESP32) --------------------
// 8 channels on this PCB.

#define SERVO1_PIN  3
#define SERVO2_PIN  4
#define SERVO3_PIN  5
#define SERVO4_PIN  6
#define SERVO5_PIN  7
#define SERVO6_PIN  15
#define SERVO7_PIN  16
#define SERVO8_PIN  46   // Strapping pin — safe after boot

// ---- Digital output pins ----------------------------------------------------
// 6 channels available on this PCB.  DOUT7/8 (DRIVE_ACTIVE / DOME_ACTIVE)
// are not wired; setDigitalPin() treats a 0 entry as absent.

#define DOUT1_PIN   11
#define DOUT2_PIN   12
#define DOUT3_PIN   13
#define DOUT4_PIN   14
#define DOUT5_PIN   9
#define DOUT6_PIN   10

// ---- Hall-effect sensor (RoboClaw dome drive) --------------------------------
// Wired to the Digital 1 header (GPIO11).  All ESP32-S3 GPIOs support
// external interrupts, so any digital pin would work here.

#define DOME_HALL_PIN  11

// ---- PPM RC input -----------------------------------------------------------

#define PPMIN_PIN   47

// ---- Analog inputs (ADC1, WiFi-safe) ----------------------------------------

#define ANALOG1_PIN  1   // ADC1_0
#define ANALOG2_PIN  2   // ADC1_1

// ---- SD card chip-select pin ------------------------------------------------

#define SD_CS_PIN   39

// ---- Mode select pins -------------------------------------------------------
// RCSEL: disabled by default — GPIO12/13 serve as DOUT2/3.
//   Define ESP32_RCSEL_ENABLE to reclaim them as RCSEL A/B (loses DOUT2/3).
// SEL2 A: available on GPIO21 unless AUX serial is enabled.
//   Define ESP32_AUX_SERIAL to use GPIO21/38 as UART2; SEL2 is then unavailable.

#ifdef ESP32_RCSEL_ENABLE
#define RCSEL_PIN   12   // RCSEL A, shared with DOUT2
#endif

#ifndef ESP32_AUX_SERIAL
#define SEL2_PIN    21   // SEL2 A; SEL2 B (GPIO38) reserved for future 2-bit expansion
#endif

// ---- SPI bus pins -----------------------------------------------------------

#define SPI_MOSI_PIN      35
#define SPI_MISO_PIN      36
#define SPI_SCK_PIN       37
#define XBEE_CS_PIN       40
#define SPI_SPARE_CS_PIN  41
#define XBEE_ATTN_PIN     42

// ---- I2C bus pins -----------------------------------------------------------
// Both are strapping pins but safe to use after boot.

#define I2C_SDA_PIN  45
#define I2C_SCL_PIN  48

// ---- AUX serial (UART2 on SW-UART header) -----------------------------------
// Mutually exclusive with SEL2 A/B.  Enable with -DESP32_AUX_SERIAL.

#ifdef ESP32_AUX_SERIAL
#define AUX_SERIAL         Serial2
#define AUX_SERIAL_TX_PIN  21
#define AUX_SERIAL_RX_PIN  38
#endif

// ---- Serial port assignments ------------------------------------------------
// Serial  = USB-CDC console  (ARDUINO_USB_CDC_ON_BOOT=1)
// Serial0 = UART0 GPIO43/44  WCB / body controller out  (was Serial3 on Mega)
// Serial1 = UART1 GPIO17/18  RoboClaw dome drive         (see drive_config.h)

#define CONSOLE_SERIAL  Serial   // USB-CDC

// SERIAL (downstream WCB output) — ESP32 Arduino.h defines SERIAL as 0x0
// (a UART index constant).  Undefine it so we can use SERIAL as a Stream alias.
#if !defined(DOME_DRIVE_SERIAL) && !defined(RDH_SERIAL)
#undef  SERIAL
#define SERIAL  Serial0          // UART0, GPIO43/44 — WCB serial out
#endif
