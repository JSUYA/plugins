// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "plus_player.h"

#include <app_manager.h>
#include <system_info.h>

#include <sstream>

#include "log.h"

static std::vector<std::string> split(const std::string &s, char delim) {
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> tokens;
  while (getline(ss, item, delim)) {
    tokens.push_back(item);
  }
  return tokens;
}

static plusplayer::TrackType ConvertTrackType(std::string track_type) {
  if (track_type == "video") {
    return plusplayer::TrackType::kTrackTypeVideo;
  }
  if (track_type == "audio") {
    return plusplayer::TrackType::kTrackTypeAudio;
  }
  if (track_type == "text") {
    return plusplayer::TrackType::kTrackTypeSubtitle;
  }
  return plusplayer::TrackType::kTrackTypeMax;
}

PlusPlayer::PlusPlayer(flutter::BinaryMessenger *messenger,
                       FlutterDesktopViewRef flutter_view)
    : VideoPlayer(messenger, flutter_view) {}

PlusPlayer::~PlusPlayer() {
  if (player_) {
    Stop(player_);
    Close(player_);
    UnregisterListener(player_);
    DestroyPlayer(player_);
    player_ = nullptr;
  }

  if (drm_manager_) {
    drm_manager_->ReleaseDrmSession();
  }
}

void PlusPlayer::RegisterListener() {
  listener_.buffering_callback = OnBufferStatus;
  listener_.adaptive_streaming_control_callback =
      OnAdaptiveStreamingControlEvent;
  listener_.completed_callback = OnEos;
  listener_.drm_init_data_callback = OnDrmInitData;
  listener_.error_callback = OnError;
  listener_.error_message_callback = OnErrorMsg;
  listener_.prepared_callback = OnPrepareDone;
  listener_.seek_completed_callback = OnSeekDone;
  listener_.subtitle_data_callback = OnSubtitleData;
  ::RegisterListener(player_, &listener_, this);
}

int64_t PlusPlayer::Create(const std::string &uri,
                           const CreateMessage &create_message) {
  LOG_INFO("[PlusPlayer] Create player.");

  std::string video_format;

  if (create_message.format_hint() && !create_message.format_hint()->empty()) {
    video_format = *create_message.format_hint();
  }

  if (video_format == "dash") {
    player_ = CreatePlayer(plusplayer::PlayerType::kDASH);
  } else {
    player_ = CreatePlayer(plusplayer::PlayerType::kDefault);
  }

  if (!player_) {
    LOG_ERROR("[PlusPlayer] Fail to create player.");
    return -1;
  }

  if (create_message.streaming_property() != nullptr &&
      !create_message.streaming_property()->empty()) {
    for (const auto &[key, value] : *create_message.streaming_property()) {
      SetStreamingProperty(player_, std::get<std::string>(key),
                           std::get<std::string>(value));
    }
  }

  if (!Open(player_, uri)) {
    LOG_ERROR("[PlusPlayer] Fail to open uri :  %s.", uri.c_str());
    return -1;
  }
  LOG_INFO("[PlusPlayer] Uri: %s", uri.c_str());

  char *appId = nullptr;
  int ret = app_manager_get_app_id(getpid(), &appId);
  if (ret != APP_MANAGER_ERROR_NONE) {
    LOG_ERROR("[PlusPlayer] Fail to get app id: %s.", get_error_message(ret));
    return -1;
  }
  SetAppId(player_, std::string(appId));
  free(appId);

  RegisterListener();

  int drm_type =
      flutter_common::GetValue(create_message.drm_configs(), "drmType", 0);
  std::string license_server_url = flutter_common::GetValue(
      create_message.drm_configs(), "licenseServerUrl", std::string());
  if (drm_type != 0) {
    if (!SetDrm(uri, drm_type, license_server_url)) {
      LOG_ERROR("[PlusPlayer] Fail to set drm.");
      return -1;
    }
  }

  if (!SetDisplay()) {
    LOG_ERROR("[PlusPlayer] Fail to set display.");
    return -1;
  }

  SetDisplayRoi(0, 0, 1, 1);

  bool is_prebuffer_mode = flutter_common::GetValue(
      create_message.player_options(), "prebufferMode", false);
  if (is_prebuffer_mode) {
    SetPrebufferMode(player_, true);
    is_prebuffer_mode_ = true;
  }

  if (!PrepareAsync(player_)) {
    LOG_ERROR("[PlusPlayer] Player fail to prepare.");
    return -1;
  }
  return SetUpEventChannel();
}

void PlusPlayer::Dispose() {
  LOG_INFO("[PlusPlayer] Player disposing.");
  ClearUpEventChannel();
}

void PlusPlayer::SetDisplayRoi(int32_t x, int32_t y, int32_t width,
                               int32_t height) {
  plusplayer::Geometry roi;
  roi.x = x;
  roi.y = y;
  roi.w = width;
  roi.h = height;
  if (!::SetDisplayRoi(player_, roi)) {
    LOG_ERROR("[PlusPlayer] Player fail to set display roi.");
  }
}

bool PlusPlayer::Play() {
  LOG_INFO("[PlusPlayer] Player starting.");

  plusplayer::State state = GetState(player_);
  if (state < plusplayer::State::kTrackSourceReady) {
    LOG_ERROR("[PlusPlayer] Player is not ready.");
    return false;
  }

  if (state <= plusplayer::State::kReady) {
    if (!Start(player_)) {
      LOG_ERROR("[PlusPlayer] Player fail to start.");
      return false;
    }
    return true;
  } else if (state == plusplayer::State::kPaused) {
    if (!Resume(player_)) {
      LOG_ERROR("[PlusPlayer] Player fail to resume.");
      return false;
    }
    return true;
  }
  return false;
}

bool PlusPlayer::Activate() {
  if (!::Activate(player_, plusplayer::kTrackTypeVideo)) {
    LOG_ERROR("[PlusPlayer] Fail to activate video.");
    return false;
  }
  if (!::Activate(player_, plusplayer::kTrackTypeAudio)) {
    LOG_ERROR("[PlusPlayer] Fail to activate audio.");
    return false;
  }
  if (!::Activate(player_, plusplayer::kTrackTypeSubtitle)) {
    LOG_ERROR("[PlusPlayer] Fail to activate subtitle.");
  }

  return true;
}

bool PlusPlayer::Deactivate() {
  if (is_prebuffer_mode_) {
    Stop(player_);
    return true;
  }

  if (!::Deactivate(player_, plusplayer::kTrackTypeVideo)) {
    LOG_ERROR("[PlusPlayer] Fail to activate video.");
    return false;
  }
  if (!::Deactivate(player_, plusplayer::kTrackTypeAudio)) {
    LOG_ERROR("[PlusPlayer] Fail to activate audio.");
    return false;
  }
  if (!::Deactivate(player_, plusplayer::kTrackTypeSubtitle)) {
    LOG_ERROR("[PlusPlayer] Fail to activate subtitle.");
  }

  return true;
}

bool PlusPlayer::Pause() {
  LOG_INFO("[PlusPlayer] Player pausing.");

  plusplayer::State state = GetState(player_);
  if (state < plusplayer::State::kReady) {
    LOG_ERROR("[PlusPlayer] Player is not ready.");
    return false;
  }

  if (state != plusplayer::State::kPlaying) {
    LOG_INFO("[PlusPlayer] Player not playing.");
    return false;
  }

  if (!::Pause(player_)) {
    LOG_ERROR("[PlusPlayer] Player fail to pause.");
    return false;
  }

  return true;
}

bool PlusPlayer::SetLooping(bool is_looping) {
  LOG_ERROR("[PlusPlayer] Not support to set looping.");
  return true;
}

bool PlusPlayer::SetVolume(double volume) {
  if (GetState(player_) < plusplayer::State::kPlaying) {
    LOG_ERROR("[PlusPlayer] Player is in invalid state");
    return false;
  }
  // dart api volume range[0,1], plusplaer volume range[0,100]
  int new_volume = volume * 100;
  LOG_INFO("[PlusPlayer] Volume: %d", new_volume);
  if (!::SetVolume(player_, new_volume)) {
    LOG_ERROR("[PlusPlayer] Fail to set volume.");
    return false;
  }
  return true;
}

bool PlusPlayer::SetPlaybackSpeed(double speed) {
  LOG_INFO("[PlusPlayer] Speed: %f", speed);

  if (GetState(player_) <= plusplayer::State::kIdle) {
    LOG_ERROR("[PlusPlayer] Player is not prepared.");
    return false;
  }
  if (!SetPlaybackRate(player_, speed)) {
    LOG_ERROR("[PlusPlayer] Player fail to set playback rate.");
    return false;
  }
  return true;
}

bool PlusPlayer::SeekTo(int64_t position, SeekCompletedCallback callback) {
  LOG_INFO("[PlusPlayer] Seek to position: %lld", position);

  if (GetState(player_) < plusplayer::State::kReady) {
    LOG_ERROR("[PlusPlayer] Player is not ready.");
    return false;
  }

  if (on_seek_completed_) {
    LOG_ERROR("[PlusPlayer] Player is already seeking.");
    return false;
  }

  on_seek_completed_ = std::move(callback);
  if (!Seek(player_, position)) {
    on_seek_completed_ = nullptr;
    LOG_ERROR("[PlusPlayer] Player fail to seek.");
    return false;
  }

  return true;
}

int64_t PlusPlayer::GetPosition() {
  uint64_t position = 0;
  plusplayer::State state = GetState(player_);
  if (state == plusplayer::State::kPlaying ||
      state == plusplayer::State::kPaused) {
    if (!GetPlayingTime(player_, &position)) {
      LOG_ERROR("[PlusPlayer] Player fail to get the current playing time.");
    }
  }
  return static_cast<int64_t>(position);
}

bool PlusPlayer::IsLive() {
  plusplayer::PlayerMemento memento;
  if (!GetMemento(player_, &memento)) {
    LOG_ERROR("[PlusPlayer] Player fail to get memento.");
    return false;
  }

  return memento.is_live;
}

std::pair<int64_t, int64_t> PlusPlayer::GetLiveDuration() {
  std::string live_duration_str =
      ::GetStreamingProperty(player_, "GET_LIVE_DURATION");
  if (live_duration_str.empty()) {
    LOG_ERROR("[PlusPlayer] Player fail to get live duration.");
    return std::make_pair(0, 0);
  }

  std::vector<std::string> time_vec = split(live_duration_str, '|');
  return std::make_pair(std::stoll(time_vec[0]), std::stoll(time_vec[1]));
}

std::pair<int64_t, int64_t> PlusPlayer::GetDuration() {
  if (IsLive()) {
    return GetLiveDuration();
  } else {
    int64_t duration = 0;
    if (!::GetDuration(player_, &duration)) {
      LOG_ERROR("[PlusPlayer] Player fail to get the duration.");
      return std::make_pair(0, 0);
    }
    LOG_INFO("[PlusPlayer] Video duration: %lld.", duration);
    return std::make_pair(0, duration);
  }
}

void PlusPlayer::GetVideoSize(int32_t *width, int32_t *height) {
  if (GetState(player_) >= plusplayer::State::kTrackSourceReady) {
    bool found = false;
    std::vector<plusplayer::Track> tracks = GetActiveTrackInfo(player_);
    for (auto track : tracks) {
      if (track.type == plusplayer::TrackType::kTrackTypeVideo) {
        *width = track.width;
        *height = track.height;
        found = true;
        break;
      }
    }
    if (!found) {
      LOG_ERROR("[PlusPlayer] Player fail to get video size.");
    } else {
      LOG_INFO("[PlusPlayer] Video width: %d, height: %d.", *width, *height);
    }
  }
}

bool PlusPlayer::IsReady() {
  return plusplayer::State::kReady == GetState(player_);
}

bool PlusPlayer::SetDisplay() {
  void *native_window = GetWindowHandle();
  if (!native_window) {
    LOG_ERROR("[PlusPlayer] Could not get a native window handle.");
    return false;
  }
  int x = 0, y = 0, width = 0, height = 0;
  ecore_wl2_window_proxy_->ecore_wl2_window_geometry_get(native_window, &x, &y,
                                                         &width, &height);
  uint32_t resource_id = FlutterDesktopViewGetResourceId(flutter_view_);
  if (resource_id == 0) {
    LOG_ERROR("[PlusPlayer] Fail to get resource id.");
    return false;
  }
  bool ret = ::SetDisplay(player_, plusplayer::DisplayType::kOverlay,
                          resource_id, x, y, width, height);
  if (!ret) {
    LOG_ERROR("[PlusPlayer] Player fail to set display.");
    return false;
  }

  ret = SetDisplayMode(player_, plusplayer::DisplayMode::kDstRoi);
  if (!ret) {
    LOG_ERROR("[PlusPlayer] Player fail to set display mode.");
    return false;
  }

  return true;
}

flutter::EncodableList PlusPlayer::GetTrackInfo(std::string track_type) {
  if (!player_) {
    LOG_ERROR("[PlusPlayer] Player not created.");
    return {};
  }

  plusplayer::State state = GetState(player_);
  if (state < plusplayer::State::kTrackSourceReady) {
    LOG_ERROR("[PlusPlayer] Player is in invalid state.");
    return {};
  }

  plusplayer::TrackType type = ConvertTrackType(track_type);

  int track_count = GetTrackCount(player_, type);
  if (track_count <= 0) {
    return {};
  }

  const std::vector<plusplayer::Track> track_info = ::GetTrackInfo(player_);
  if (track_info.empty()) {
    return {};
  }

  flutter::EncodableList trackSelections = {};
  flutter::EncodableMap trackSelection = {};
  trackSelection.insert(
      {flutter::EncodableValue("trackType"), flutter::EncodableValue(type)});
  if (type == plusplayer::TrackType::kTrackTypeVideo) {
    LOG_INFO("[PlusPlayer] Video track count: %d", track_count);
    for (const auto &track : track_info) {
      if (track.type == plusplayer::kTrackTypeVideo) {
        trackSelection.insert_or_assign(flutter::EncodableValue("trackId"),
                                        flutter::EncodableValue(track.index));
        trackSelection.insert_or_assign(flutter::EncodableValue("width"),
                                        flutter::EncodableValue(track.width));
        trackSelection.insert_or_assign(flutter::EncodableValue("height"),
                                        flutter::EncodableValue(track.height));
        trackSelection.insert_or_assign(flutter::EncodableValue("bitrate"),
                                        flutter::EncodableValue(track.bitrate));
        LOG_INFO(
            "[PlusPlayer] video track info[%d]: width[%d], height[%d], "
            "bitrate[%d]",
            track.index, track.width, track.height, track.bitrate);

        trackSelections.push_back(flutter::EncodableValue(trackSelection));
      }
    }
  } else if (type == plusplayer::TrackType::kTrackTypeAudio) {
    LOG_INFO("[PlusPlayer] Audio track count: %d", track_count);
    for (const auto &track : track_info) {
      if (track.type == plusplayer::kTrackTypeAudio) {
        trackSelection.insert_or_assign(flutter::EncodableValue("trackId"),
                                        flutter::EncodableValue(track.index));
        trackSelection.insert_or_assign(
            flutter::EncodableValue("language"),
            flutter::EncodableValue(track.language_code));
        trackSelection.insert_or_assign(
            flutter::EncodableValue("channel"),
            flutter::EncodableValue(track.channels));
        trackSelection.insert_or_assign(flutter::EncodableValue("bitrate"),
                                        flutter::EncodableValue(track.bitrate));
        LOG_INFO(
            "[PlusPlayer] Audio track info[%d]: language[%s], channel[%d], "
            "sample_rate[%d], bitrate[%d]",
            track.index, track.language_code.c_str(), track.channels,
            track.sample_rate, track.bitrate);

        trackSelections.push_back(flutter::EncodableValue(trackSelection));
      }
    }
  } else if (type == plusplayer::TrackType::kTrackTypeSubtitle) {
    LOG_INFO("[PlusPlayer] Subtitle track count: %d", track_count);
    for (const auto &track : track_info) {
      if (track.type == plusplayer::kTrackTypeSubtitle) {
        trackSelection.insert_or_assign(flutter::EncodableValue("trackId"),
                                        flutter::EncodableValue(track.index));
        trackSelection.insert_or_assign(
            flutter::EncodableValue("language"),
            flutter::EncodableValue(track.language_code));
        LOG_INFO("[PlusPlayer] Subtitle track info[%d]: language[%s]",
                 track.index, track.language_code.c_str());

        trackSelections.push_back(flutter::EncodableValue(trackSelection));
      }
    }
  }

  return trackSelections;
}

bool PlusPlayer::SetTrackSelection(int32_t track_id, std::string track_type) {
  LOG_INFO("[PlusPlayer] Track id is: %d,track type is: %s", track_id,
           track_type.c_str());

  if (!player_) {
    LOG_ERROR("[PlusPlayer] Player not created.");
    return false;
  }

  plusplayer::State state = GetState(player_);
  if (state < plusplayer::State::kTrackSourceReady) {
    LOG_ERROR("[PlusPlayer] Player is in invalid state.");
    return false;
  }

  if (!SelectTrack(player_, ConvertTrackType(track_type), track_id)) {
    LOG_ERROR("[PlusPlayer] Player fail to select track.");
    return false;
  }
  return true;
}

bool PlusPlayer::SetDrm(const std::string &uri, int drm_type,
                        const std::string &license_server_url) {
  drm_manager_ = std::make_unique<DrmManager>();
  if (!drm_manager_->CreateDrmSession(drm_type, true)) {
    LOG_ERROR("[PlusPlayer] Fail to create drm session.");
    return false;
  }

  int drm_handle = 0;
  if (!drm_manager_->GetDrmHandle(&drm_handle)) {
    LOG_ERROR("[PlusPlayer] Fail to get drm handle.");
    return false;
  }

  plusplayer::drm::Type type;
  switch (drm_type) {
    case DrmManager::DrmType::DRM_TYPE_PLAYREADAY:
      type = plusplayer::drm::Type::kPlayready;
      break;
    case DrmManager::DrmType::DRM_TYPE_WIDEVINECDM:
      type = plusplayer::drm::Type::kWidevineCdm;
      break;
    default:
      type = plusplayer::drm::Type::kNone;
      break;
  }

  plusplayer::drm::Property property;
  property.handle = drm_handle;
  property.type = type;
  property.license_acquired_cb =
      reinterpret_cast<plusplayer::drm::LicenseAcquiredCb>(OnLicenseAcquired);
  property.license_acquired_userdata =
      reinterpret_cast<plusplayer::drm::UserData>(this);
  property.external_decryption = false;
  ::SetDrm(player_, property);

  if (license_server_url.empty()) {
    bool success = drm_manager_->SetChallenge(uri, binary_messenger_);
    if (!success) {
      LOG_ERROR("[PlusPlayer]Fail to set challenge.");
      return false;
    }
  } else {
    if (!drm_manager_->SetChallenge(uri, license_server_url)) {
      LOG_ERROR("[PlusPlayer]Fail to set challenge.");
      return false;
    }
  }
  return true;
}

std::string PlusPlayer::GetStreamingProperty(
    const std::string &streaming_property_type) {
  if (!player_) {
    LOG_ERROR("[PlusPlayer] Player not created.");
    return "";
  }
  plusplayer::State state = GetState(player_);
  if (state == plusplayer::State::kNone || state == plusplayer::State::kIdle) {
    LOG_ERROR("[PlusPlayer]:Player is in invalid state[%d]", state);
    return "";
  }
  return ::GetStreamingProperty(player_, streaming_property_type);
}

bool PlusPlayer::SetBufferConfig(const std::string &key, int64_t value) {
  if (!player_) {
    LOG_ERROR("[PlusPlayer] Player not created.");
    return false;
  }

  plusplayer::State state = GetState(player_);
  if (state == plusplayer::State::kNone) {
    LOG_ERROR("[PlusPlayer]:Player is in invalid state[%d]", state);
    return false;
  }
  const std::pair<std::string, int> config = std::make_pair(key, value);
  return ::SetBufferConfig(player_, config);
}

bool PlusPlayer::OnLicenseAcquired(int *drm_handle, unsigned int length,
                                   unsigned char *pssh_data, void *user_data) {
  LOG_INFO("[PlusPlayer] License acquired.");
  PlusPlayer *self = static_cast<PlusPlayer *>(user_data);

  if (self->drm_manager_) {
    return self->drm_manager_->SecurityInitCompleteCB(drm_handle, length,
                                                      pssh_data, nullptr);
  }
  return false;
}

void PlusPlayer::OnPrepareDone(bool ret, void *user_data) {
  LOG_INFO("[PlusPlayer] Prepare done, result: %d.", ret);
  PlusPlayer *self = reinterpret_cast<PlusPlayer *>(user_data);

  if (!SetDisplayVisible(self->player_, true)) {
    LOG_ERROR("[PlusPlayer] Fail to set display visible.");
  }

  if (!self->is_initialized_ && ret) {
    self->SendInitialized();
  }
}

void PlusPlayer::OnBufferStatus(int percent, void *user_data) {
  LOG_INFO("[PlusPlayer] Buffering percent: %d.", percent);
  PlusPlayer *self = reinterpret_cast<PlusPlayer *>(user_data);

  if (percent == 100) {
    self->SendBufferingEnd();
    self->is_buffering_ = false;
  } else if (!self->is_buffering_ && percent <= 5) {
    self->SendBufferingStart();
    self->is_buffering_ = true;
  } else {
    self->SendBufferingUpdate(percent);
  }
}

void PlusPlayer::OnSeekDone(void *user_data) {
  LOG_INFO("[PlusPlayer] Seek completed.");
  PlusPlayer *self = reinterpret_cast<PlusPlayer *>(user_data);

  if (self->on_seek_completed_) {
    self->on_seek_completed_();
    self->on_seek_completed_ = nullptr;
  }
}

void PlusPlayer::OnEos(void *user_data) {
  LOG_INFO("[PlusPlayer] Play completed.");
  PlusPlayer *self = reinterpret_cast<PlusPlayer *>(user_data);

  self->SendPlayCompleted();
}

void PlusPlayer::OnSubtitleData(char *data, const int size,
                                const plusplayer::SubtitleType &type,
                                const uint64_t duration, void *user_data) {
  LOG_INFO("[PlusPlayer] Subtitle updated, duration: %llu, text: %s", duration,
           data);
  PlusPlayer *self = reinterpret_cast<PlusPlayer *>(user_data);

  self->SendSubtitleUpdate(duration, data);
}

void PlusPlayer::OnResourceConflicted(void *user_data) {
  LOG_ERROR("[PlusPlayer] Resource conflicted.");
  PlusPlayer *self = reinterpret_cast<PlusPlayer *>(user_data);

  self->SendError("PlusPlayer error", "Resource conflicted");
}

std::string GetErrorMessage(plusplayer::ErrorType error_code) {
  switch (error_code) {
    case plusplayer::ErrorType::kNone:
      return "Successful";
    case plusplayer::ErrorType::kOutOfMemory:
      return "Out of memory";
    case plusplayer::ErrorType::kInvalidParameter:
      return "Invalid parameter";
    case plusplayer::ErrorType::kNoSuchFile:
      return "No such file or directory";
    case plusplayer::ErrorType::kInvalidOperation:
      return "Invalid operation";
    case plusplayer::ErrorType::kFileNoSpaceOnDevice:
      return "No space left on the device";
    case plusplayer::ErrorType::kFeatureNotSupportedOnDevice:
      return "Not supported file on this device";
    case plusplayer::ErrorType::kSeekFailed:
      return "Seek operation failure";
    case plusplayer::ErrorType::kInvalidState:
      return "Invalid player state";
    case plusplayer::ErrorType::kNotSupportedFile:
      return "File format not supported";
    case plusplayer::ErrorType::kNotSupportedFormat:
      return "Not supported media format";
    case plusplayer::ErrorType::kInvalidUri:
      return "Invalid URI";
    case plusplayer::ErrorType::kSoundPolicy:
      return "Sound policy error";
    case plusplayer::ErrorType::kConnectionFailed:
      return "Streaming connection failed";
    case plusplayer::ErrorType::kVideoCaptureFailed:
      return "Video capture failed";
    case plusplayer::ErrorType::kDrmExpired:
      return "DRM license expired";
    case plusplayer::ErrorType::kDrmNoLicense:
      return "DRM no license";
    case plusplayer::ErrorType::kDrmFutureUse:
      return "License for future use";
    case plusplayer::ErrorType::kDrmNotPermitted:
      return "DRM format not permitted";
    case plusplayer::ErrorType::kResourceLimit:
      return "Resource limit";
    case plusplayer::ErrorType::kPermissionDenied:
      return "Permission denied";
    case plusplayer::ErrorType::kServiceDisconnected:
      return "Service disconnected";
    case plusplayer::ErrorType::kBufferSpace:
      return "No buffer space available";
    case plusplayer::ErrorType::kNotSupportedAudioCodec:
      return "Not supported audio codec but video can be played";
    case plusplayer::ErrorType::kNotSupportedVideoCodec:
      return "Not supported video codec but audio can be played";
    case plusplayer::ErrorType::kNotSupportedSubtitle:
      return "Not supported subtitle format";
    default:
      return "Not defined error";
  }
}

void PlusPlayer::OnError(const plusplayer::ErrorType &error_code,
                         void *user_data) {
  LOG_ERROR("[PlusPlayer] Error code: %d", error_code);
  PlusPlayer *self = reinterpret_cast<PlusPlayer *>(user_data);

  self->SendError("[PlusPlayer] error",
                  std::string("Error: ") + GetErrorMessage(error_code));
}

void PlusPlayer::OnErrorMsg(const plusplayer::ErrorType &error_code,
                            const char *error_msg, void *user_data) {
  LOG_ERROR("[PlusPlayer] Error code: %d, message: %s.", error_code, error_msg);
  PlusPlayer *self = reinterpret_cast<PlusPlayer *>(user_data);

  self->SendError("PlusPlayer error", std::string("Error: ") + error_msg);
}

void PlusPlayer::OnDrmInitData(int *drm_handle, unsigned int len,
                               unsigned char *pssh_data,
                               plusplayer::TrackType type, void *user_data) {
  LOG_INFO("[PlusPlayer] Drm init completed.");
  PlusPlayer *self = reinterpret_cast<PlusPlayer *>(user_data);

  if (self->drm_manager_) {
    if (self->drm_manager_->SecurityInitCompleteCB(drm_handle, len, pssh_data,
                                                   nullptr)) {
      DrmLicenseAcquiredDone(self->player_, type);
    }
  }
}

void PlusPlayer::OnAdaptiveStreamingControlEvent(
    const plusplayer::StreamingMessageType &type,
    const plusplayer::MessageParam &msg, void *user_data) {
  LOG_INFO("[PlusPlayer] Message type: %d, is DrmInitData (%d)", type,
           type == plusplayer::StreamingMessageType::kDrmInitData);
  PlusPlayer *self = reinterpret_cast<PlusPlayer *>(user_data);

  if (type == plusplayer::StreamingMessageType::kDrmInitData) {
    if (msg.data.empty() || 0 == msg.size) {
      LOG_ERROR("[PlusPlayer] Empty message.");
      return;
    }

    if (self->drm_manager_) {
      self->drm_manager_->UpdatePsshData(msg.data.data(), msg.size);
    }
  }
}

void PlusPlayer::OnClosedCaptionData(std::unique_ptr<char[]> data,
                                     const int size, void *user_data) {}

void PlusPlayer::OnCueEvent(const char *cue_data, void *user_data) {}

void PlusPlayer::OnDateRangeEvent(const char *date_range_data,
                                  void *user_data) {}

void PlusPlayer::OnStopReachEvent(bool stop_reach, void *user_data) {}

void PlusPlayer::OnCueOutContEvent(const char *cue_out_cont_data,
                                   void *user_data) {}

void PlusPlayer::OnChangeSourceDone(bool ret, void *user_data) {}

void PlusPlayer::OnStateChangedToPlaying(void *user_data) {}
