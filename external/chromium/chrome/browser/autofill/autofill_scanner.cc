// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autofill/autofill_scanner.h"

#include "base/logging.h"
#include "chrome/browser/autofill/autofill_field.h"

AutofillScanner::AutofillScanner(
    const std::vector<const AutofillField*>& fields)
    : cursor_(fields.begin()),
      saved_cursor_(fields.begin()),
      begin_(fields.begin()),
      end_(fields.end()) {
}

AutofillScanner::~AutofillScanner() {
}

void AutofillScanner::Advance() {
  DCHECK(!IsEnd());
  ++cursor_;
}

const AutofillField* AutofillScanner::Cursor() const {
  if (IsEnd()) {
    NOTREACHED();
    return NULL;
  }

  return *cursor_;
}

bool AutofillScanner::IsEnd() const {
  return cursor_ == end_;
}

void AutofillScanner::Rewind() {
  DCHECK(saved_cursor_ != end_);
  cursor_ = saved_cursor_;
  saved_cursor_ = end_;
}

void AutofillScanner::RewindTo(size_t index) {
  DCHECK(index < static_cast<size_t>(end_ - begin_));
  cursor_ = begin_ + index;
  saved_cursor_ = end_;
}

size_t AutofillScanner::SaveCursor() {
  saved_cursor_ = cursor_;
  return static_cast<size_t>(cursor_ - begin_);
}
