// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_WEBTHEMEENGINE_IMPL_DEFAULT_H_
#define WEBKIT_GLUE_WEBTHEMEENGINE_IMPL_DEFAULT_H_

#include "third_party/WebKit/Source/WebKit/chromium/public/platform/default/WebThemeEngine.h"

#if defined (__LB_SHELL__)
// For NOTIMPLEMENTED()
#include "base/logging.h"
#endif

namespace webkit_glue {

class WebThemeEngineImpl : public WebKit::WebThemeEngine {
 public:
  // WebThemeEngine methods:
#if defined (__LB_SHELL__)
  virtual WebKit::WebSize getSize(WebKit::WebThemeEngine::Part) {
    return WebKit::WebSize();
  }
  virtual void paint(
      WebKit::WebCanvas* canvas,
      WebKit::WebThemeEngine::Part part,
      WebKit::WebThemeEngine::State state,
      const WebKit::WebRect& rect,
      const WebKit::WebThemeEngine::ExtraParams* extra_params) {
    NOTIMPLEMENTED();
  }
#else
  virtual WebKit::WebSize getSize(WebKit::WebThemeEngine::Part);
  virtual void paint(
      WebKit::WebCanvas* canvas,
      WebKit::WebThemeEngine::Part part,
      WebKit::WebThemeEngine::State state,
      const WebKit::WebRect& rect,
      const WebKit::WebThemeEngine::ExtraParams* extra_params);
#endif
};

}  // namespace webkit_glue

#endif  // WEBKIT_GLUE_WEBTHEMEENGINE_IMPL_DEFAULT_H_
