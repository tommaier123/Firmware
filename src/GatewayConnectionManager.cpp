#include "GatewayConnectionManager.h"

#include "CaptivePortal.h"
#include "CommandHandler.h"
#include "Config.h"
#include "Constants.h"
#include "fbs/DeviceToServerMessage_generated.h"
#include "fbs/ServerToDeviceMessage_generated.h"
#include "ShockerCommandType.h"
#include "ShockerModelType.h"
#include "Time.h"

#include <esp_log.h>

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <WiFiClientSecure.h>

#include <memory>
#include <unordered_map>

static const char* const TAG             = "GatewayConnectionManager";
static const char* const AUTH_TOKEN_FILE = "/authToken";

extern const std::uint8_t* const rootca_crt_bundle_start asm("_binary_data_cert_x509_crt_bundle_start");

static std::unordered_map<std::uint64_t, OpenShock::GatewayConnectionManager::ConnectedChangedHandler> s_connectedChangedHandlers;

struct GatewayClient {
  GatewayClient(const std::string& authToken, const std::string& fwVersionStr) : m_webSocket(), m_lastKeepAlive(0), m_state(State::Disconnected) {
    ESP_LOGD(TAG, "Creating GatewayClient");
    std::string headers;
    headers.reserve(512);

    headers += "Firmware-Version: ";
    headers += fwVersionStr;
    headers += "\r\n";
    headers += "Device-Token: ";
    headers += authToken;

    m_webSocket.setExtraHeaders(headers.c_str());
    m_webSocket.onEvent(std::bind(&GatewayClient::_handleEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  }
  ~GatewayClient() {
    ESP_LOGD(TAG, "Destroying GatewayClient");
    m_webSocket.disconnect();
  }

  enum class State {
    Disconnected,
    Disconnecting,
    Connecting,
    Connected,
  };

  State state() { return m_state; }

  void connect(const char* lcgFqdn) {
    if (m_state != State::Disconnected) {
      return;
    }
    m_state = State::Connecting;
    m_webSocket.beginSSL(lcgFqdn, 443, "/1/ws/device");
  }

  void disconnect() {
    if (m_state != State::Connected) {
      return;
    }
    m_state = State::Disconnecting;
    m_webSocket.disconnect();
  }

  bool loop() {
    if (m_state == State::Disconnected) {
      return false;
    }

    m_webSocket.loop();

    // We are still in the process of connecting or disconnecting
    if (m_state != State::Connected) {
      // return true to indicate that we are still busy
      return true;
    }

    std::uint64_t msNow = OpenShock::Millis();

    std::uint64_t timeSinceLastKA = msNow - m_lastKeepAlive;

    if (timeSinceLastKA >= 15'000) {
      _sendKeepAlive();
      m_lastKeepAlive = msNow;
    }

    return true;
  }

  WebSocketsClient m_webSocket;
  std::uint64_t m_lastKeepAlive;
  State m_state;

private:
  void _sendKeepAlive() {
    ESP_LOGV(TAG, "Sending keep alive message");

    OpenShock::Serialization::KeepAlive keepAlive(OpenShock::Millis());

    flatbuffers::FlatBufferBuilder builder(64);

    auto keepAliveOffset = builder.CreateStruct(keepAlive);

    auto msg = OpenShock::Serialization::CreateDeviceToServerMessage(builder, OpenShock::Serialization::DeviceToServerMessagePayload::KeepAlive, keepAliveOffset.Union());

    builder.Finish(msg);

    m_webSocket.sendBIN(builder.GetBufferPointer(), builder.GetSize());
  }

  void _handleShockerCommandListMessage(const OpenShock::Serialization::ShockerCommandList* shockerCommandList) {
    auto commands = shockerCommandList->commands();
    if (commands == nullptr) {
      ESP_LOGE(TAG, "Received invalid command list from API");
      return;
    }

    ESP_LOGV(TAG, "Received command list from API (%d commands)", commands->size());

    for (auto command : *commands) {
      OpenShock::ShockerModelType model;
      switch (command->model()) {
        case OpenShock::Serialization::Types::ShockerModelType::CaiXianlin:
          model = OpenShock::ShockerModelType::CaiXianlin;
          break;
        case OpenShock::Serialization::Types::ShockerModelType::PetTrainer:
          model = OpenShock::ShockerModelType::PetTrainer;
          break;
        default:
          ESP_LOGE(TAG, "Received unknown model type from API (%d)", command->model());
          continue;
      }
      ESP_LOGV(TAG, "   Model %s", OpenShock::Serialization::Types::EnumNameShockerModelType(command->model()));

      OpenShock::ShockerCommandType type;
      switch (command->type()) {
        case OpenShock::Serialization::Types::ShockerCommandType::Stop:

          type = OpenShock::ShockerCommandType::Stop;
          break;
        case OpenShock::Serialization::Types::ShockerCommandType::Shock:
          type = OpenShock::ShockerCommandType::Shock;
          break;
        case OpenShock::Serialization::Types::ShockerCommandType::Vibrate:
          type = OpenShock::ShockerCommandType::Vibrate;
          break;
        case OpenShock::Serialization::Types::ShockerCommandType::Sound:
          type = OpenShock::ShockerCommandType::Sound;
          break;
        default:
          ESP_LOGE(TAG, "Received unknown command type from API (%d)", command->type());
          continue;
      }
      ESP_LOGV(TAG, "   CommandTpe %s", OpenShock::Serialization::Types::EnumNameShockerCommandType(command->type()));

      std::uint16_t id       = command->id();
      std::uint8_t intensity = command->intensity();
      std::uint32_t duration = command->duration();

      ESP_LOGV(TAG, "   ID %u, Intensity %u, Duration %u", id, intensity, duration);

      if (!OpenShock::CommandHandler::HandleCommand(model, id, type, intensity, duration)) {
        ESP_LOGE(TAG, "Remote command failed/rejected!");
      }
    }
  }

  void _handleCaptivePortalConfigMessage(const OpenShock::Serialization::CaptivePortalConfig* captivePortalConfig) {
    bool enabled = captivePortalConfig->enabled();

    ESP_LOGD(TAG, "Captive portal is %s", enabled ? "force enabled" : "normal");

    OpenShock::CaptivePortal::SetAlwaysEnabled(enabled);
  }

  void _parseMessage(std::uint8_t* payload, std::size_t length) {
    ESP_LOGV(TAG, "Received binary message from API (%d bytes)", length);

    if (payload == nullptr || length == 0) {
      ESP_LOGE(TAG, "Received invalid message from API");
      return;
    }

    auto msg = flatbuffers::GetRoot<OpenShock::Serialization::ServerToDeviceMessage>(payload);
    if (msg == nullptr) {
      ESP_LOGE(TAG, "Received invalid message from API");
      return;
    }

    flatbuffers::Verifier::Options verifierOptions {
      .max_depth  = 32,   // Half of the default value
      .max_tables = 64,   // Way less than the default value
      .max_size   = 512,  // Don't see why we would need more than this at the moment
    };
    flatbuffers::Verifier verifier(payload, length, verifierOptions);
    if (!msg->Verify(verifier)) {
      ESP_LOGE(TAG, "Received invalid message from API");
      return;
    }

    ESP_LOGV(TAG, "Message validated");

    switch (msg->payload_type()) {
      [[likely]] case OpenShock::Serialization::ServerToDeviceMessagePayload::ShockerCommandList:
        _handleShockerCommandListMessage(msg->payload_as_ShockerCommandList());
        break;
      case OpenShock::Serialization::ServerToDeviceMessagePayload::CaptivePortalConfig:
        _handleCaptivePortalConfigMessage(msg->payload_as_CaptivePortalConfig());
        break;
      [[unlikely]] default:
        ESP_LOGE(TAG, "Received unknown message type from API (%d)", msg->payload_type());
        break;
    }
  }

  void _handleEvent(WStype_t type, std::uint8_t* payload, std::size_t length) {
    switch (type) {
      case WStype_DISCONNECTED:
        ESP_LOGI(TAG, "Disconnected from API");
        m_state = State::Disconnected;
        for (auto& handler : s_connectedChangedHandlers) {
          handler.second(false);
        }
        break;
      case WStype_CONNECTED:
        ESP_LOGI(TAG, "Connected to API");
        m_state = State::Connected;
        for (auto& handler : s_connectedChangedHandlers) {
          handler.second(true);
        }
        _sendKeepAlive();
        break;
      case WStype_TEXT:
        ESP_LOGW(TAG, "Received text from API, JSON parsing is not supported anymore :D");
        break;
      case WStype_ERROR:
        ESP_LOGE(TAG, "Received error from API");
        break;
      case WStype_FRAGMENT_TEXT_START:
        ESP_LOGD(TAG, "Received fragment text start from API");
        break;
      case WStype_FRAGMENT:
        ESP_LOGD(TAG, "Received fragment from API");
        break;
      case WStype_FRAGMENT_FIN:
        ESP_LOGD(TAG, "Received fragment fin from API");
        break;
      case WStype_PING:
        ESP_LOGD(TAG, "Received ping from API");
        break;
      case WStype_PONG:
        ESP_LOGD(TAG, "Received pong from API");
        break;
      case WStype_BIN:
        _parseMessage(payload, length);
        break;
      case WStype_FRAGMENT_BIN_START:
        ESP_LOGE(TAG, "Received binary fragment start from API, this is not supported!");
        break;
      default:
        ESP_LOGE(TAG, "Received unknown event from API");
        break;
    }
  }
};

constexpr std::uint8_t FLAG_NONE          = 0;
constexpr std::uint8_t FLAG_HAS_IP        = 1 << 0;
constexpr std::uint8_t FLAG_AUTHENTICATED = 1 << 1;

static std::uint8_t s_flags = 0;
// static WiFiClientSecure* s_wifiClient            = nullptr;
static std::unique_ptr<GatewayClient> s_wsClient = nullptr;

void _evGotIPHandler(arduino_event_t* event) {
  s_flags |= FLAG_HAS_IP;
  ESP_LOGD(TAG, "Got IP address");
}

void _evWiFiDisconnectedHandler(arduino_event_t* event) {
  s_flags    = FLAG_NONE;
  s_wsClient = nullptr;
  ESP_LOGD(TAG, "Lost IP address");
}

using namespace OpenShock;

bool GatewayConnectionManager::Init() {
  //
  //  ######  ########  ######  ##     ## ########  #### ######## ##    ##    ########  ####  ######  ##    ##
  // ##    ## ##       ##    ## ##     ## ##     ##  ##     ##     ##  ##     ##     ##  ##  ##    ## ##   ##
  // ##       ##       ##       ##     ## ##     ##  ##     ##      ####      ##     ##  ##  ##       ##  ##
  //  ######  ######   ##       ##     ## ########   ##     ##       ##       ########   ##   ######  #####
  //       ## ##       ##       ##     ## ##   ##    ##     ##       ##       ##   ##    ##        ## ##  ##
  // ##    ## ##       ##    ## ##     ## ##    ##   ##     ##       ##       ##    ##   ##  ##    ## ##   ##
  //  ######  ########  ######   #######  ##     ## ####    ##       ##       ##     ## ####  ######  ##    ##
  //
  // WARNING: Skipping SSL Verification!
  //
  // Fix loading CA Certificate bundles, currently fails with "[esp_crt_bundle.c:161] esp_crt_bundle_init(): Unable to allocate memory for bundle"
  // This is probably due to the fact that the bundle is too large for the ESP32's heap or the bundle is incorrectly packed
  //
  //
  // s_wifiClient = new WiFiClientSecure();
  // s_wifiClient->setCACertBundle(rootca_crt_bundle_start);
  //
  //
  //  ######  ########  ######  ##     ## ########  #### ######## ##    ##    ########  ####  ######  ##    ##
  // ##    ## ##       ##    ## ##     ## ##     ##  ##     ##     ##  ##     ##     ##  ##  ##    ## ##   ##
  // ##       ##       ##       ##     ## ##     ##  ##     ##      ####      ##     ##  ##  ##       ##  ##
  //  ######  ######   ##       ##     ## ########   ##     ##       ##       ########   ##   ######  #####
  //       ## ##       ##       ##     ## ##   ##    ##     ##       ##       ##   ##    ##        ## ##  ##
  // ##    ## ##       ##    ## ##     ## ##    ##   ##     ##       ##       ##    ##   ##  ##    ## ##   ##
  //  ######  ########  ######   #######  ##     ## ####    ##       ##       ##     ## ####  ######  ##    ##
  //
  WiFi.onEvent(_evGotIPHandler, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(_evGotIPHandler, ARDUINO_EVENT_WIFI_STA_GOT_IP6);
  WiFi.onEvent(_evWiFiDisconnectedHandler, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  return true;
}

bool GatewayConnectionManager::IsConnected() {
  if (s_wsClient == nullptr) {
    return false;
  }

  return s_wsClient->state() == GatewayClient::State::Connected;
}

void GetDeviceInfoFromJsonResponse(HTTPClient& http) {
  ArduinoJson::DynamicJsonDocument doc(1024);  // TODO: profile the normal message size and adjust this accordingly
  deserializeJson(doc, http.getString());

  auto data   = doc["data"];
  String id   = data["id"];
  String name = data["name"];

  ESP_LOGD(TAG, "Device ID:   %s", id.c_str());
  ESP_LOGD(TAG, "Device name: %s", name.c_str());

  auto shockers = data["shockers"];
  for (int i = 0; i < shockers.size(); i++) {
    auto shocker              = shockers[i];
    String shockerId          = shocker["id"];
    std::uint16_t shockerRfId = shocker["rfId"];
    std::uint8_t shockerModel = shocker["model"];

    ESP_LOGD(TAG, "Found shocker %s with RF ID %u and model %u", shockerId.c_str(), shockerRfId, shockerModel);
  }
}

bool GatewayConnectionManager::IsPaired() {
  return (s_flags & FLAG_AUTHENTICATED) != 0;
}

// This method is here to heap usage
std::string GetAuthTokenFromJsonResponse(HTTPClient& http) {
  ArduinoJson::DynamicJsonDocument doc(1024);  // TODO: profile the normal message size and adjust this accordingly
  deserializeJson(doc, http.getString());

  String str = doc["data"];

  return std::string(str.c_str(), str.length());
}

bool GatewayConnectionManager::Pair(unsigned int pairCode) {
  if ((s_flags & FLAG_HAS_IP) == 0) {
    return false;
  }
  s_wsClient = nullptr;

  ESP_LOGD(TAG, "Attempting to pair with pair code %u", pairCode);

  HTTPClient http;

  char uri[256];
  sprintf(uri, OPENSHOCK_API_URL("/1/device/pair/%u"), pairCode);

  http.begin(uri);  // TODO: http.begin(*s_wifiClient, uri);

  int responseCode = http.GET();

  if (responseCode != 200) {
    ESP_LOGE(TAG, "Error while getting auth token: [%d] %s", responseCode, http.getString().c_str());
    return false;
  }

  std::string authToken = GetAuthTokenFromJsonResponse(http);

  http.end();

  if (authToken.empty()) {
    ESP_LOGE(TAG, "Received empty auth token");
    return false;
  }

  Config::SetBackendAuthToken(authToken);

  s_flags |= FLAG_AUTHENTICATED;
  ESP_LOGD(TAG, "Successfully paired with pair code %u", pairCode);

  return true;
}
void GatewayConnectionManager::UnPair() {
  s_flags &= FLAG_HAS_IP;
  s_wsClient = nullptr;
  Config::ClearBackendAuthToken();
}

std::uint64_t GatewayConnectionManager::RegisterConnectedChangedHandler(ConnectedChangedHandler handler) {
  static std::uint64_t nextHandleId    = 0;
  std::uint64_t handleId               = nextHandleId++;
  s_connectedChangedHandlers[handleId] = handler;
  return handleId;
}
void GatewayConnectionManager::UnRegisterConnectedChangedHandler(std::uint64_t handlerId) {
  auto it = s_connectedChangedHandlers.find(handlerId);

  if (it != s_connectedChangedHandlers.end()) {
    s_connectedChangedHandlers.erase(it);
  }
}

bool FetchDeviceInfo(const std::string& authToken) {
  // TODO: this function is very slow, should be optimized!
  if ((s_flags & FLAG_HAS_IP) == 0) {
    return false;
  }

  HTTPClient http;

  http.begin(OPENSHOCK_API_URL("/1/device/self"));  // TODO: http.begin(*s_wifiClient, uri);
  http.addHeader("DeviceToken", authToken.c_str());

  int responseCode = http.GET();

  if (responseCode == 401) {
    ESP_LOGD(TAG, "Auth token is invalid, clearing it");
    Config::ClearBackendAuthToken();
    return false;
  }

  if (responseCode != 200) {
    ESP_LOGE(TAG, "Error while verifying auth token: [%d] %s", responseCode, http.getString().c_str());
    return false;
  }

  GetDeviceInfoFromJsonResponse(http);

  http.end();

  s_flags |= FLAG_AUTHENTICATED;

  return true;
}

static std::uint64_t _lastConnectionAttempt = 0;
bool ConnectToLCG() {
  // TODO: this function is very slow, should be optimized!
  if (s_wsClient == nullptr) {  // If wsClient is already initialized, we are already paired or connected
    ESP_LOGD(TAG, "wsClient is null");
    return false;
  }

  if (s_wsClient->state() != GatewayClient::State::Disconnected) {
    ESP_LOGD(TAG, "WebSocketClient is not disconnected, waiting...");
    s_wsClient->disconnect();
    return false;
  }

  std::uint64_t msNow = Millis();
  if (_lastConnectionAttempt != 0 && (msNow - _lastConnectionAttempt) < 20'000) {  // Only try to connect every 20 seconds
    return false;
  }

  _lastConnectionAttempt = msNow;

  if (!Config::HasBackendAuthToken()) {
    ESP_LOGD(TAG, "No auth token, can't connect to LCG");
    return false;
  }

  std::string authToken = Config::GetBackendAuthToken();

  HTTPClient http;

  http.begin(OPENSHOCK_API_URL("/1/device/assignLCG"));  // TODO: http.begin(*s_wifiClient, uri);
  http.addHeader("DeviceToken", authToken.c_str());

  int responseCode = http.GET();

  if (responseCode != 200) {
    ESP_LOGE(TAG, "Error while fetching LCG endpoint: [%d] %s", responseCode, http.getString().c_str());
    return false;
  }

  ArduinoJson::DynamicJsonDocument doc(1024);  // TODO: profile the normal message size and adjust this accordingly
  deserializeJson(doc, http.getString());

  auto data           = doc["data"];
  const char* fqdn    = data["fqdn"];
  const char* country = data["country"];

  http.end();

  if (fqdn == nullptr || country == nullptr) {
    ESP_LOGE(TAG, "Received invalid response from LCG endpoint");
    return false;
  }

  ESP_LOGD(TAG, "Connecting to LCG endpoint %s in country %s", fqdn, country);
  s_wsClient->connect(fqdn);

  return true;
}

void GatewayConnectionManager::Update() {
  if (s_wsClient == nullptr) {
    // Can't connect to the API without WiFi or an auth token
    if ((s_flags & FLAG_HAS_IP) == 0 || !Config::HasBackendAuthToken()) {
      return;
    }

    std::string authToken = Config::GetBackendAuthToken();

    // Test if the auth token is valid
    if (!FetchDeviceInfo(authToken)) {
      ESP_LOGD(TAG, "Auth token is invalid, clearing it");
      Config::ClearBackendAuthToken();
      return;
    }

    s_flags |= FLAG_AUTHENTICATED;
    ESP_LOGD(TAG, "Successfully verified auth token");

    s_wsClient = std::make_unique<GatewayClient>(authToken, OpenShock::Constants::Version);
  }

  if (s_wsClient->loop()) {
    return;
  }

  if (ConnectToLCG()) {
    ESP_LOGD(TAG, "Successfully connected to LCG");
    return;
  }
}
