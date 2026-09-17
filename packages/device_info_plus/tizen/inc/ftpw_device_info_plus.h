// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef FLUTTER_TIZEN_PLUGINS_WRAPPER_DEVICE_INFO_PLUS_H_
#define FLUTTER_TIZEN_PLUGINS_WRAPPER_DEVICE_INFO_PLUS_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // Native libraries remain loaded for the process lifetime (callbacks may outlive calls).

    char *ftpw_device_info_plus_vconf_get_str(const char *key);

#ifdef __cplusplus
}
#endif

#endif
