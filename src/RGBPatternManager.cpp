#include <freertos/FreeRTOS.h>

#include "RGBPatternManager.h"

const char* const TAG = "RGBPatternManager";

#include "Chipset.h"
#include "Logging.h"
#include "util/TaskUtils.h"

#include <array>

using namespace OpenShock;

// Currently this assumes a single WS2812B LED
// TODO: Support multiple LEDs ?
// TODO: Support other LED types ?

// WS2812B timing requires 100ns tick resolution
const float kWS2812BTickrateNs = 100;

RGBPatternManager::RGBPatternManager(gpio_num_t gpioPin)
  : m_gpioPin(GPIO_NUM_NC)
  , m_brightness(255)
  , m_pattern()
  , m_taskHandle(nullptr)
  , m_taskMutex()
{
  if (gpioPin == GPIO_NUM_NC) {
    OS_LOGE(TAG, "Pin is not set");
    return;
  }

  if (!OpenShock::IsValidOutputPin(gpioPin)) {
    OS_LOGE(TAG, "Pin %hhi is not a valid output pin", gpioPin);
    return;
  }

  // Initialize RMT with frequency derived from tick rate (100ns tick = 10MHz)
  if (!rmtInit(gpioPin, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, static_cast<uint32_t>(1000000000.0f / kWS2812BTickrateNs))) {
    OS_LOGE(TAG, "Failed to initialize RMT for pin %hhi", gpioPin);
    return;
  }

  OS_LOGD(TAG, "RMT initialized for pin %hhi", gpioPin);

  SetBrightness(20);

  m_gpioPin = gpioPin;
}

RGBPatternManager::~RGBPatternManager()
{
  ClearPattern();

  if (m_gpioPin != GPIO_NUM_NC) {
    rmtDeinit(m_gpioPin);
  }
}

void RGBPatternManager::SetPattern(const RGBState* pattern, std::size_t patternLength)
{
  m_taskMutex.lock(portMAX_DELAY);

  ClearPatternInternal();

  // Set new values
  m_pattern.resize(patternLength);
  std::copy(pattern, pattern + patternLength, m_pattern.begin());

  // Start the task
  BaseType_t result = TaskUtils::TaskCreateExpensive(RunPattern, TAG, 4096, this, 1, &m_taskHandle);  // PROFILED: 1.7KB stack usage
  if (result != pdPASS) {
    OS_LOGE(TAG, "[pin-%hhi] Failed to create task: %d", m_gpioPin, result);

    m_taskHandle = nullptr;
    m_pattern.clear();
  }

  m_taskMutex.unlock();
}

void RGBPatternManager::ClearPattern()
{
  m_taskMutex.lock(portMAX_DELAY);

  ClearPatternInternal();

  m_taskMutex.unlock();
}

void RGBPatternManager::ClearPatternInternal()
{
  if (m_taskHandle != nullptr) {
    vTaskDelete(m_taskHandle);
    m_taskHandle = nullptr;
  }

  m_pattern.clear();
}

// Range: 0-255
void RGBPatternManager::SetBrightness(uint8_t brightness)
{
  m_brightness = brightness;
}

void RGBPatternManager::RunPattern(void* arg)
{
  RGBPatternManager* thisPtr = reinterpret_cast<RGBPatternManager*>(arg);

  gpio_num_t gpioPin             = thisPtr->m_gpioPin;
  uint8_t brightness             = thisPtr->m_brightness;
  std::vector<RGBState>& pattern = thisPtr->m_pattern;

  std::array<rmt_data_t, 24> led_data;  // 24 bits per LED (8 bits per color * 3 colors)

  while (true) {
    for (const auto& state : pattern) {
      // WS2812B usually takes commands in GRB order
      // https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf - Page 5
      // But some actually expect RGB!

      uint8_t r = static_cast<uint8_t>(static_cast<uint16_t>(state.red) * brightness / 255);
      uint8_t g = static_cast<uint8_t>(static_cast<uint16_t>(state.green) * brightness / 255);
      uint8_t b = static_cast<uint8_t>(static_cast<uint16_t>(state.blue) * brightness / 255);
#if OPENSHOCK_LED_FLIP_RG_CHANNELS
      std::swap(r, g);
#endif

      const uint32_t colors = (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(r) << 8) | static_cast<uint32_t>(b);

      // Encode the data
      for (std::size_t bit = 0; bit < 24; bit++) {
        if (colors & (1 << (23 - bit))) {
          led_data[bit].level0    = 1;
          led_data[bit].duration0 = 8;
          led_data[bit].level1    = 0;
          led_data[bit].duration1 = 4;
        } else {
          led_data[bit].level0    = 1;
          led_data[bit].duration0 = 4;
          led_data[bit].level1    = 0;
          led_data[bit].duration1 = 8;
        }
      }

      // Send the data
      rmtWrite(gpioPin, led_data.data(), led_data.size(), RMT_WAIT_FOR_EVER);
      vTaskDelay(pdMS_TO_TICKS(state.duration));
    }
  }
}
