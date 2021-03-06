// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_DEVICE_STATE_H_
#define CHROMEOS_NETWORK_DEVICE_STATE_H_

#include "chromeos/network/managed_state.h"

namespace chromeos {

// Simple class to provide device state information. Similar to NetworkState;
// see network_state.h for usage guidelines.
class CHROMEOS_EXPORT DeviceState : public ManagedState {
 public:
  explicit DeviceState(const std::string& path);
  virtual ~DeviceState();

  // ManagedState overrides
  virtual bool PropertyChanged(const std::string& key,
                               const base::Value& value) OVERRIDE;

  // Accessors
  const std::string& mac_address() const { return mac_address_; }
  bool provider_requires_roaming() const { return provider_requires_roaming_; }
  bool support_network_scan() const { return support_network_scan_; }

 private:
  // Common Device Properties
  std::string mac_address_;
  // Cellular specific propeties
  std::string home_provider_id_;
  bool provider_requires_roaming_;
  bool support_network_scan_;

  DISALLOW_COPY_AND_ASSIGN(DeviceState);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_DEVICE_STATE_H_
