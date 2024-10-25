// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_MEDIA_PLAYER_PROXY_H_
#define FLUTTER_PLUGIN_MEDIA_PLAYER_PROXY_H_

#include <player.h>

typedef enum {
  PLAYER_ADAPTIVE_INFO_BITRATE_INIT,
  PLAYER_ADAPTIVE_INFO_BITRATE_INIT_NUM,
  PLAYER_ADAPTIVE_INFO_DURATION,
  PLAYER_ADAPTIVE_INFO_LIVE_DURATION,
  PLAYER_ADAPTIVE_INFO_AVAILABLE_BITRATES,
  PLAYER_ADAPTIVE_INFO_RATE_RETURNED,
  PLAYER_ADAPTIVE_INFO_CURRENT_BITRATE,
  PLAYER_ADAPTIVE_INFO_GET_BUFFER_SIZE,
  PLAYER_ADAPTIVE_INFO_FIXED_BITRATE,
  PLAYER_ADAPTIVE_INFO_ADAPTIVE_BITRATE,
  PLAYER_ADAPTIVE_INFO_MAX_BYTES,
  PLAYER_ADAPTIVE_INFO_DRM_TYPE,
  PLAYER_ADAPTIVE_INFO_RATE_REQUESTED,
  PLAYER_ADAPTIVE_INFO_AUDIO_TRACK_REQUESTED,
  PLAYER_ADAPTIVE_INFO_FORMAT,
  PLAYER_ADAPTIVE_INFO_BLOCK,
  PLAYER_ADAPTIVE_INFO_MIN_PERCENT,
  PLAYER_ADAPTIVE_INFO_MIN_LATENCY,
  PLAYER_ADAPTIVE_INFO_MAX_LATENCY,
  PLAYER_ADAPTIVE_INFO_IS_LIVE,
  PLAYER_ADAPTIVE_INFO_EMIT_SIGNAL,
  PLAYER_ADAPTIVE_INFO_LOG_LEVEL,
  PLAYER_ADAPTIVE_INFO_CURRENT_BANDWIDTH,
  PLAYER_ADAPTIVE_INFO_URL_CUSTOM,
  PLAYER_ADAPTIVE_INFO_GET_AUDIO_INFO,
  PLAYER_ADAPTIVE_INFO_GET_VIDEO_INFO,
  PLAYER_ADAPTIVE_INFO_GET_TEXT_INFO,
  PLAYER_ADAPTIVE_INFO_RESUME_TIME,
  PLAYER_ADAPTIVE_INFO_AUDIO_INDEX,
  PLAYER_ADAPTIVE_INFO_PROXY_SETTING,
  PLAYER_ADAPTIVE_INFO_ATSC3_L1_SERVER_TIME,
  PLAYER_ADAPTIVE_INFO_VIDEO_FRAMES_DROPPED,
  PLAYER_ADAPTIVE_INFO_STREAMING_IS_BUFFERING,
  PLAYER_ADAPTIVE_INFO_PRESELECTION_ID,
  PLAYER_ADAPTIVE_INFO_URI_TYPE
} player_adaptive_Info_e;

class MediaPlayerProxy {
 public:
  MediaPlayerProxy();
  ~MediaPlayerProxy();

  int player_get_adaptive_streaming_info(player_h player, void* adaptive_info,
                                         int adaptive_type);
  int player_set_video_codec_type(player_h player, int video_codec_type);

 private:
  void* media_player_handle_ = nullptr;
};

#endif  // FLUTTER_PLUGIN_MEDIA_PLAYER_PROXY_H_
