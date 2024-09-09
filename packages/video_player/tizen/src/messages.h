// Autogenerated from Pigeon (v22.3.0), do not edit directly.
// See also: https://pub.dev/packages/pigeon

#ifndef PIGEON_MESSAGES_H_
#define PIGEON_MESSAGES_H_
#include <flutter/basic_message_channel.h>
#include <flutter/binary_messenger.h>
#include <flutter/encodable_value.h>
#include <flutter/standard_message_codec.h>

#include <map>
#include <optional>
#include <string>



// Generated class from Pigeon.

class FlutterError {
 public:
  explicit FlutterError(const std::string& code)
    : code_(code) {}
  explicit FlutterError(const std::string& code, const std::string& message)
    : code_(code), message_(message) {}
  explicit FlutterError(const std::string& code, const std::string& message, const flutter::EncodableValue& details)
    : code_(code), message_(message), details_(details) {}

  const std::string& code() const { return code_; }
  const std::string& message() const { return message_; }
  const flutter::EncodableValue& details() const { return details_; }

 private:
  std::string code_;
  std::string message_;
  flutter::EncodableValue details_;
};

template<class T> class ErrorOr {
 public:
  ErrorOr(const T& rhs) : v_(rhs) {}
  ErrorOr(const T&& rhs) : v_(std::move(rhs)) {}
  ErrorOr(const FlutterError& rhs) : v_(rhs) {}
  ErrorOr(const FlutterError&& rhs) : v_(std::move(rhs)) {}

  bool has_error() const { return std::holds_alternative<FlutterError>(v_); }
  const T& value() const { return std::get<T>(v_); };
  const FlutterError& error() const { return std::get<FlutterError>(v_); };

 private:
  friend class TizenVideoPlayerApi;
  ErrorOr() = default;
  T TakeValue() && { return std::get<T>(std::move(v_)); }

  std::variant<T, FlutterError> v_;
};



// Generated class from Pigeon that represents data sent in messages.
class TextureMessage {
 public:
  // Constructs an object setting all fields.
  explicit TextureMessage(int64_t texture_id);

  int64_t texture_id() const;
  void set_texture_id(int64_t value_arg);


 private:
  static TextureMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class TizenVideoPlayerApi;
  friend class PigeonInternalCodecSerializer;
  int64_t texture_id_;

};


// Generated class from Pigeon that represents data sent in messages.
class LoopingMessage {
 public:
  // Constructs an object setting all fields.
  explicit LoopingMessage(
    int64_t texture_id,
    bool is_looping);

  int64_t texture_id() const;
  void set_texture_id(int64_t value_arg);

  bool is_looping() const;
  void set_is_looping(bool value_arg);


 private:
  static LoopingMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class TizenVideoPlayerApi;
  friend class PigeonInternalCodecSerializer;
  int64_t texture_id_;
  bool is_looping_;

};


// Generated class from Pigeon that represents data sent in messages.
class VolumeMessage {
 public:
  // Constructs an object setting all fields.
  explicit VolumeMessage(
    int64_t texture_id,
    double volume);

  int64_t texture_id() const;
  void set_texture_id(int64_t value_arg);

  double volume() const;
  void set_volume(double value_arg);


 private:
  static VolumeMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class TizenVideoPlayerApi;
  friend class PigeonInternalCodecSerializer;
  int64_t texture_id_;
  double volume_;

};


// Generated class from Pigeon that represents data sent in messages.
class PlaybackSpeedMessage {
 public:
  // Constructs an object setting all fields.
  explicit PlaybackSpeedMessage(
    int64_t texture_id,
    double speed);

  int64_t texture_id() const;
  void set_texture_id(int64_t value_arg);

  double speed() const;
  void set_speed(double value_arg);


 private:
  static PlaybackSpeedMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class TizenVideoPlayerApi;
  friend class PigeonInternalCodecSerializer;
  int64_t texture_id_;
  double speed_;

};


// Generated class from Pigeon that represents data sent in messages.
class PositionMessage {
 public:
  // Constructs an object setting all fields.
  explicit PositionMessage(
    int64_t texture_id,
    int64_t position);

  int64_t texture_id() const;
  void set_texture_id(int64_t value_arg);

  int64_t position() const;
  void set_position(int64_t value_arg);


 private:
  static PositionMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class TizenVideoPlayerApi;
  friend class PigeonInternalCodecSerializer;
  int64_t texture_id_;
  int64_t position_;

};


// Generated class from Pigeon that represents data sent in messages.
class CreateMessage {
 public:
  // Constructs an object setting all non-nullable fields.
  explicit CreateMessage(const flutter::EncodableMap& http_headers);

  // Constructs an object setting all fields.
  explicit CreateMessage(
    const std::string* asset,
    const std::string* uri,
    const std::string* package_name,
    const std::string* format_hint,
    const flutter::EncodableMap& http_headers);

  const std::string* asset() const;
  void set_asset(const std::string_view* value_arg);
  void set_asset(std::string_view value_arg);

  const std::string* uri() const;
  void set_uri(const std::string_view* value_arg);
  void set_uri(std::string_view value_arg);

  const std::string* package_name() const;
  void set_package_name(const std::string_view* value_arg);
  void set_package_name(std::string_view value_arg);

  const std::string* format_hint() const;
  void set_format_hint(const std::string_view* value_arg);
  void set_format_hint(std::string_view value_arg);

  const flutter::EncodableMap& http_headers() const;
  void set_http_headers(const flutter::EncodableMap& value_arg);


 private:
  static CreateMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class TizenVideoPlayerApi;
  friend class PigeonInternalCodecSerializer;
  std::optional<std::string> asset_;
  std::optional<std::string> uri_;
  std::optional<std::string> package_name_;
  std::optional<std::string> format_hint_;
  flutter::EncodableMap http_headers_;

};


// Generated class from Pigeon that represents data sent in messages.
class MixWithOthersMessage {
 public:
  // Constructs an object setting all fields.
  explicit MixWithOthersMessage(bool mix_with_others);

  bool mix_with_others() const;
  void set_mix_with_others(bool value_arg);


 private:
  static MixWithOthersMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class TizenVideoPlayerApi;
  friend class PigeonInternalCodecSerializer;
  bool mix_with_others_;

};


class PigeonInternalCodecSerializer : public flutter::StandardCodecSerializer {
 public:
  PigeonInternalCodecSerializer();
  inline static PigeonInternalCodecSerializer& GetInstance() {
    static PigeonInternalCodecSerializer sInstance;
    return sInstance;
  }

  void WriteValue(
    const flutter::EncodableValue& value,
    flutter::ByteStreamWriter* stream) const override;

 protected:
  flutter::EncodableValue ReadValueOfType(
    uint8_t type,
    flutter::ByteStreamReader* stream) const override;

};

// Generated interface from Pigeon that represents a handler of messages from Flutter.
class TizenVideoPlayerApi {
 public:
  TizenVideoPlayerApi(const TizenVideoPlayerApi&) = delete;
  TizenVideoPlayerApi& operator=(const TizenVideoPlayerApi&) = delete;
  virtual ~TizenVideoPlayerApi() {}
  virtual std::optional<FlutterError> Initialize() = 0;
  virtual ErrorOr<TextureMessage> Create(const CreateMessage& msg) = 0;
  virtual std::optional<FlutterError> Dispose(const TextureMessage& msg) = 0;
  virtual std::optional<FlutterError> SetLooping(const LoopingMessage& msg) = 0;
  virtual std::optional<FlutterError> SetVolume(const VolumeMessage& msg) = 0;
  virtual std::optional<FlutterError> SetPlaybackSpeed(const PlaybackSpeedMessage& msg) = 0;
  virtual std::optional<FlutterError> Play(const TextureMessage& msg) = 0;
  virtual ErrorOr<PositionMessage> Position(const TextureMessage& msg) = 0;
  virtual void SeekTo(
    const PositionMessage& msg,
    std::function<void(std::optional<FlutterError> reply)> result) = 0;
  virtual std::optional<FlutterError> Pause(const TextureMessage& msg) = 0;
  virtual std::optional<FlutterError> SetMixWithOthers(const MixWithOthersMessage& msg) = 0;

  // The codec used by TizenVideoPlayerApi.
  static const flutter::StandardMessageCodec& GetCodec();
  // Sets up an instance of `TizenVideoPlayerApi` to handle messages through the `binary_messenger`.
  static void SetUp(
    flutter::BinaryMessenger* binary_messenger,
    TizenVideoPlayerApi* api);
  static void SetUp(
    flutter::BinaryMessenger* binary_messenger,
    TizenVideoPlayerApi* api,
    const std::string& message_channel_suffix);
  static flutter::EncodableValue WrapError(std::string_view error_message);
  static flutter::EncodableValue WrapError(const FlutterError& error);

 protected:
  TizenVideoPlayerApi() = default;

};
#endif  // PIGEON_MESSAGES_H_
