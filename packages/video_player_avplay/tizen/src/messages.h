// Autogenerated from Pigeon (v10.1.6), do not edit directly.
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
  explicit FlutterError(const std::string& code) : code_(code) {}
  explicit FlutterError(const std::string& code, const std::string& message)
      : code_(code), message_(message) {}
  explicit FlutterError(const std::string& code, const std::string& message,
                        const flutter::EncodableValue& details)
      : code_(code), message_(message), details_(details) {}

  const std::string& code() const { return code_; }
  const std::string& message() const { return message_; }
  const flutter::EncodableValue& details() const { return details_; }

 private:
  std::string code_;
  std::string message_;
  flutter::EncodableValue details_;
};

template <class T>
class ErrorOr {
 public:
  ErrorOr(const T& rhs) : v_(rhs) {}
  ErrorOr(const T&& rhs) : v_(std::move(rhs)) {}
  ErrorOr(const FlutterError& rhs) : v_(rhs) {}
  ErrorOr(const FlutterError&& rhs) : v_(std::move(rhs)) {}

  bool has_error() const { return std::holds_alternative<FlutterError>(v_); }
  const T& value() const { return std::get<T>(v_); };
  const FlutterError& error() const { return std::get<FlutterError>(v_); };

 private:
  friend class VideoPlayerAvplayApi;
  ErrorOr() = default;
  T TakeValue() && { return std::get<T>(std::move(v_)); }

  std::variant<T, FlutterError> v_;
};

// Generated class from Pigeon that represents data sent in messages.
class PlayerMessage {
 public:
  // Constructs an object setting all fields.
  explicit PlayerMessage(int64_t player_id);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

 private:
  static PlayerMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
};

// Generated class from Pigeon that represents data sent in messages.
class LoopingMessage {
 public:
  // Constructs an object setting all fields.
  explicit LoopingMessage(int64_t player_id, bool is_looping);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  bool is_looping() const;
  void set_is_looping(bool value_arg);

 private:
  static LoopingMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  bool is_looping_;
};

// Generated class from Pigeon that represents data sent in messages.
class VolumeMessage {
 public:
  // Constructs an object setting all fields.
  explicit VolumeMessage(int64_t player_id, double volume);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  double volume() const;
  void set_volume(double value_arg);

 private:
  static VolumeMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  double volume_;
};

// Generated class from Pigeon that represents data sent in messages.
class PlaybackSpeedMessage {
 public:
  // Constructs an object setting all fields.
  explicit PlaybackSpeedMessage(int64_t player_id, double speed);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  double speed() const;
  void set_speed(double value_arg);

 private:
  static PlaybackSpeedMessage FromEncodableList(
      const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  double speed_;
};

// Generated class from Pigeon that represents data sent in messages.
class TrackMessage {
 public:
  // Constructs an object setting all fields.
  explicit TrackMessage(int64_t player_id,
                        const flutter::EncodableList& tracks);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  const flutter::EncodableList& tracks() const;
  void set_tracks(const flutter::EncodableList& value_arg);

 private:
  static TrackMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  flutter::EncodableList tracks_;
};

// Generated class from Pigeon that represents data sent in messages.
class TrackTypeMessage {
 public:
  // Constructs an object setting all fields.
  explicit TrackTypeMessage(int64_t player_id, const std::string& track_type);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  const std::string& track_type() const;
  void set_track_type(std::string_view value_arg);

 private:
  static TrackTypeMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  std::string track_type_;
};

// Generated class from Pigeon that represents data sent in messages.
class SelectedTracksMessage {
 public:
  // Constructs an object setting all fields.
  explicit SelectedTracksMessage(int64_t player_id, int64_t track_id,
                                 const std::string& track_type);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  int64_t track_id() const;
  void set_track_id(int64_t value_arg);

  const std::string& track_type() const;
  void set_track_type(std::string_view value_arg);

 private:
  static SelectedTracksMessage FromEncodableList(
      const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  int64_t track_id_;
  std::string track_type_;
};

// Generated class from Pigeon that represents data sent in messages.
class PositionMessage {
 public:
  // Constructs an object setting all fields.
  explicit PositionMessage(int64_t player_id, int64_t position);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  int64_t position() const;
  void set_position(int64_t value_arg);

 private:
  static PositionMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  int64_t position_;
};

// Generated class from Pigeon that represents data sent in messages.
class CreateMessage {
 public:
  // Constructs an object setting all non-nullable fields.
  CreateMessage();

  // Constructs an object setting all fields.
  explicit CreateMessage(const std::string* asset, const std::string* uri,
                         const std::string* package_name,
                         const std::string* format_hint,
                         const flutter::EncodableMap* http_headers,
                         const flutter::EncodableMap* drm_configs,
                         const flutter::EncodableMap* player_options,
                         const flutter::EncodableMap* streaming_property);

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

  const flutter::EncodableMap* http_headers() const;
  void set_http_headers(const flutter::EncodableMap* value_arg);
  void set_http_headers(const flutter::EncodableMap& value_arg);

  const flutter::EncodableMap* drm_configs() const;
  void set_drm_configs(const flutter::EncodableMap* value_arg);
  void set_drm_configs(const flutter::EncodableMap& value_arg);

  const flutter::EncodableMap* player_options() const;
  void set_player_options(const flutter::EncodableMap* value_arg);
  void set_player_options(const flutter::EncodableMap& value_arg);

  const flutter::EncodableMap* streaming_property() const;
  void set_streaming_property(const flutter::EncodableMap* value_arg);
  void set_streaming_property(const flutter::EncodableMap& value_arg);

 private:
  static CreateMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  std::optional<std::string> asset_;
  std::optional<std::string> uri_;
  std::optional<std::string> package_name_;
  std::optional<std::string> format_hint_;
  std::optional<flutter::EncodableMap> http_headers_;
  std::optional<flutter::EncodableMap> drm_configs_;
  std::optional<flutter::EncodableMap> player_options_;
  std::optional<flutter::EncodableMap> streaming_property_;
};

// Generated class from Pigeon that represents data sent in messages.
class MixWithOthersMessage {
 public:
  // Constructs an object setting all fields.
  explicit MixWithOthersMessage(bool mix_with_others);

  bool mix_with_others() const;
  void set_mix_with_others(bool value_arg);

 private:
  static MixWithOthersMessage FromEncodableList(
      const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  bool mix_with_others_;
};

// Generated class from Pigeon that represents data sent in messages.
class GeometryMessage {
 public:
  // Constructs an object setting all fields.
  explicit GeometryMessage(int64_t player_id, int64_t x, int64_t y,
                           int64_t width, int64_t height);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  int64_t x() const;
  void set_x(int64_t value_arg);

  int64_t y() const;
  void set_y(int64_t value_arg);

  int64_t width() const;
  void set_width(int64_t value_arg);

  int64_t height() const;
  void set_height(int64_t value_arg);

 private:
  static GeometryMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  int64_t x_;
  int64_t y_;
  int64_t width_;
  int64_t height_;
};

// Generated class from Pigeon that represents data sent in messages.
class DurationMessage {
 public:
  // Constructs an object setting all non-nullable fields.
  explicit DurationMessage(int64_t player_id);

  // Constructs an object setting all fields.
  explicit DurationMessage(int64_t player_id,
                           const flutter::EncodableList* duration_range);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  const flutter::EncodableList* duration_range() const;
  void set_duration_range(const flutter::EncodableList* value_arg);
  void set_duration_range(const flutter::EncodableList& value_arg);

 private:
  static DurationMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  std::optional<flutter::EncodableList> duration_range_;
};

// Generated class from Pigeon that represents data sent in messages.
class StreamingPropertyTypeMessage {
 public:
  // Constructs an object setting all fields.
  explicit StreamingPropertyTypeMessage(
      int64_t player_id, const std::string& streaming_property_type);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  const std::string& streaming_property_type() const;
  void set_streaming_property_type(std::string_view value_arg);

 private:
  static StreamingPropertyTypeMessage FromEncodableList(
      const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  std::string streaming_property_type_;
};

// Generated class from Pigeon that represents data sent in messages.
class StreamingPropertyMessage {
 public:
  // Constructs an object setting all fields.
  explicit StreamingPropertyMessage(
      int64_t player_id, const std::string& streaming_property_type,
      const std::string& streaming_property_value);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  const std::string& streaming_property_type() const;
  void set_streaming_property_type(std::string_view value_arg);

  const std::string& streaming_property_value() const;
  void set_streaming_property_value(std::string_view value_arg);

 private:
  static StreamingPropertyMessage FromEncodableList(
      const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  std::string streaming_property_type_;
  std::string streaming_property_value_;
};

// Generated class from Pigeon that represents data sent in messages.
class BufferConfigMessage {
 public:
  // Constructs an object setting all fields.
  explicit BufferConfigMessage(int64_t player_id,
                               const std::string& buffer_config_type,
                               int64_t buffer_config_value);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  const std::string& buffer_config_type() const;
  void set_buffer_config_type(std::string_view value_arg);

  int64_t buffer_config_value() const;
  void set_buffer_config_value(int64_t value_arg);

 private:
  static BufferConfigMessage FromEncodableList(
      const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  std::string buffer_config_type_;
  int64_t buffer_config_value_;
};

// Generated class from Pigeon that represents data sent in messages.
class RotationMessage {
 public:
  // Constructs an object setting all fields.
  explicit RotationMessage(int64_t player_id, int64_t rotation);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  int64_t rotation() const;
  void set_rotation(int64_t value_arg);

 private:
  static RotationMessage FromEncodableList(const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  int64_t rotation_;
};

// Generated class from Pigeon that represents data sent in messages.
class DisplayModeMessage {
 public:
  // Constructs an object setting all fields.
  explicit DisplayModeMessage(int64_t player_id, int64_t display_mode);

  int64_t player_id() const;
  void set_player_id(int64_t value_arg);

  int64_t display_mode() const;
  void set_display_mode(int64_t value_arg);

 private:
  static DisplayModeMessage FromEncodableList(
      const flutter::EncodableList& list);
  flutter::EncodableList ToEncodableList() const;
  friend class VideoPlayerAvplayApi;
  friend class VideoPlayerAvplayApiCodecSerializer;
  int64_t player_id_;
  int64_t display_mode_;
};

class VideoPlayerAvplayApiCodecSerializer
    : public flutter::StandardCodecSerializer {
 public:
  VideoPlayerAvplayApiCodecSerializer();
  inline static VideoPlayerAvplayApiCodecSerializer& GetInstance() {
    static VideoPlayerAvplayApiCodecSerializer sInstance;
    return sInstance;
  }

  void WriteValue(const flutter::EncodableValue& value,
                  flutter::ByteStreamWriter* stream) const override;

 protected:
  flutter::EncodableValue ReadValueOfType(
      uint8_t type, flutter::ByteStreamReader* stream) const override;
};

// Generated interface from Pigeon that represents a handler of messages from
// Flutter.
class VideoPlayerAvplayApi {
 public:
  VideoPlayerAvplayApi(const VideoPlayerAvplayApi&) = delete;
  VideoPlayerAvplayApi& operator=(const VideoPlayerAvplayApi&) = delete;
  virtual ~VideoPlayerAvplayApi() {}
  virtual std::optional<FlutterError> Initialize() = 0;
  virtual ErrorOr<PlayerMessage> Create(const CreateMessage& msg) = 0;
  virtual std::optional<FlutterError> Dispose(const PlayerMessage& msg) = 0;
  virtual std::optional<FlutterError> SetLooping(const LoopingMessage& msg) = 0;
  virtual std::optional<FlutterError> SetVolume(const VolumeMessage& msg) = 0;
  virtual std::optional<FlutterError> SetPlaybackSpeed(
      const PlaybackSpeedMessage& msg) = 0;
  virtual std::optional<FlutterError> Play(const PlayerMessage& msg) = 0;
  virtual ErrorOr<bool> SetDeactivate(const PlayerMessage& msg) = 0;
  virtual ErrorOr<bool> SetActivate(const PlayerMessage& msg) = 0;
  virtual ErrorOr<TrackMessage> Track(const TrackTypeMessage& msg) = 0;
  virtual ErrorOr<bool> SetTrackSelection(const SelectedTracksMessage& msg) = 0;
  virtual ErrorOr<PositionMessage> Position(const PlayerMessage& msg) = 0;
  virtual ErrorOr<DurationMessage> Duration(const PlayerMessage& msg) = 0;
  virtual void SeekTo(
      const PositionMessage& msg,
      std::function<void(std::optional<FlutterError> reply)> result) = 0;
  virtual std::optional<FlutterError> Pause(const PlayerMessage& msg) = 0;
  virtual std::optional<FlutterError> SetMixWithOthers(
      const MixWithOthersMessage& msg) = 0;
  virtual std::optional<FlutterError> SetDisplayGeometry(
      const GeometryMessage& msg) = 0;
  virtual ErrorOr<std::string> GetStreamingProperty(
      const StreamingPropertyTypeMessage& msg) = 0;
  virtual ErrorOr<bool> SetBufferConfig(const BufferConfigMessage& msg) = 0;
  virtual std::optional<FlutterError> SetStreamingProperty(
      const StreamingPropertyMessage& msg) = 0;
  virtual ErrorOr<bool> SetDisplayRotate(const RotationMessage& msg) = 0;
  virtual ErrorOr<bool> SetDisplayMode(const DisplayModeMessage& msg) = 0;

  // The codec used by VideoPlayerAvplayApi.
  static const flutter::StandardMessageCodec& GetCodec();
  // Sets up an instance of `VideoPlayerAvplayApi` to handle messages through
  // the `binary_messenger`.
  static void SetUp(flutter::BinaryMessenger* binary_messenger,
                    VideoPlayerAvplayApi* api);
  static flutter::EncodableValue WrapError(std::string_view error_message);
  static flutter::EncodableValue WrapError(const FlutterError& error);

 protected:
  VideoPlayerAvplayApi() = default;
};
#endif  // PIGEON_MESSAGES_H_
