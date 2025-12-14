#pragma once

/**
 * @file ArduinoCompat.h
 * @brief ESP-IDF and Dependency Compatibility Layer for ESP32
 * 
 * Bridges API differences between ESP-IDF 4.4 and 5.x, along with their bundled dependencies.
 * Uses Arduino Core version as the detection mechanism:
 *   - Arduino Core 2.x bundles ESP-IDF 4.4
 *   - Arduino Core 3.x bundles ESP-IDF 5.x (required for ESP32-C6/H2/P4)
 * 
 * This compatibility layer handles:
 *   - ESP-IDF API changes (task watchdog, etc.)
 *   - mbedtls library API changes (2.x vs 3.x)
 *   - Arduino framework API changes (WiFiClient, Serial)
 * 
 * Include this BEFORE any Arduino or ESP-IDF headers that have API changes.
 */

#include <Arduino.h>

// Detect Arduino Core version (used as proxy for ESP-IDF version)
#if __has_include(<esp_arduino_version.h>)
  #include <esp_arduino_version.h>
#endif

#ifndef ESP_ARDUINO_VERSION_MAJOR
  #define ESP_ARDUINO_VERSION_MAJOR 2
#endif

// ============================================================================
// mbedtls Library Compatibility (2.x vs 3.x)
// ============================================================================
// ESP-IDF 5.x bundles mbedtls 3.x which removed the _ret suffix from hash functions
// These macros allow code written for mbedtls 2.x to work with mbedtls 3.x

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  // Map old mbedtls 2.x API names to new mbedtls 3.x names (without _ret suffix)
  #define mbedtls_md5_starts_ret(ctx) mbedtls_md5_starts(ctx)
  #define mbedtls_md5_update_ret(ctx, input, ilen) mbedtls_md5_update(ctx, input, ilen)
  #define mbedtls_md5_finish_ret(ctx, output) mbedtls_md5_finish(ctx, output)
  
  #define mbedtls_sha1_starts_ret(ctx) mbedtls_sha1_starts(ctx)
  #define mbedtls_sha1_update_ret(ctx, input, ilen) mbedtls_sha1_update(ctx, input, ilen)
  #define mbedtls_sha1_finish_ret(ctx, output) mbedtls_sha1_finish(ctx, output)
  
  #define mbedtls_sha256_starts_ret(ctx, is224) mbedtls_sha256_starts(ctx, is224)
  #define mbedtls_sha256_update_ret(ctx, input, ilen) mbedtls_sha256_update(ctx, input, ilen)
  #define mbedtls_sha256_finish_ret(ctx, output) mbedtls_sha256_finish(ctx, output)
#endif

// ============================================================================
// Arduino Framework WiFi API Compatibility
// ============================================================================
// Arduino Core 3.x renamed WiFiClient to NetworkClient as part of a networking refactor

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #include <NetworkClient.h>
  #include <WiFi.h>
  using WiFiClient = NetworkClient;
  
  // IPv6Address type alias for consistency
  using IPv6Address = esp_ip6_addr_t;
  
  // Resolve namespace conflict: Serial is a macro in Arduino Core 3.x
  // This can collide with OpenShock::Serial namespace
  // Capture the macro's expansion, undefine it, then redefine as a reference
  #ifdef Serial
    static auto& _OpenShockArduinoSerial = Serial;
    #undef Serial
    #define Serial _OpenShockArduinoSerial
  #endif
#else
  #include <WiFi.h>
  // IPv6Address already defined by Arduino Core 2.x WiFi library
#endif

// ============================================================================
// ESP-IDF Task Watchdog API Compatibility
// ============================================================================
// ESP-IDF 5.x changed esp_task_wdt_init to accept a config struct instead of parameters
// Wrap the new API to provide backward compatibility with the old two-parameter signature

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  #include <esp_task_wdt.h>
  
  // Wrapper function that converts old API (timeout_sec, panic) to new config-based API
  inline esp_err_t esp_task_wdt_init(uint32_t timeout, bool panic_on_timeout) {
    esp_task_wdt_config_t config = {
      .timeout_ms = timeout * 1000,  // Old API used seconds, new API uses milliseconds
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,  // Monitor all cores
      .trigger_panic = panic_on_timeout
    };
    return esp_task_wdt_init(&config);
  }
#endif
