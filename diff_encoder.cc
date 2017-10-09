// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bsdiff/diff_encoder.h"

#include <vector>

#include "bsdiff/logging.h"

using std::endl;

namespace {

// The maximum positive number that we should encode. A number larger than this
// for unsigned fields will be interpreted as a negative value and thus a
// corrupt patch.
const uint64_t kMaxEncodedUint64Value = (1ULL << 63) - 1;

}  // namespace

namespace bsdiff {

bool DiffEncoder::AddControlEntry(const ControlEntry& entry) {
  if (entry.diff_size > kMaxEncodedUint64Value) {
    LOG(ERROR) << "Encoding value out of range " << entry.diff_size << endl;
    return false;
  }

  if (entry.extra_size > kMaxEncodedUint64Value) {
    LOG(ERROR) << "Encoding value out of range " << entry.extra_size << endl;
    return false;
  }

  if (written_output_ + entry.diff_size + entry.extra_size > new_size_) {
    LOG(ERROR) << "Wrote more output than the declared new_size" << endl;
    return false;
  }

  if (entry.diff_size > 0 &&
      (old_pos_ < 0 || static_cast<uint64_t>(old_pos_) >= old_size_)) {
    LOG(ERROR) << "The pointer in the old stream (" << old_pos_
               << ") is out of bounds [0, " << old_size_ << ")" << endl;
    return false;
  }

  // Pass down the control entry.
  if (!patch_->AddControlEntry(entry))
    return false;

  // Generate the diff stream.
  std::vector<uint8_t> diff(entry.diff_size);
  for (uint64_t i = 0; i < entry.diff_size; ++i) {
    diff[i] = new_buf_[written_output_ + i] - old_buf_[old_pos_ + i];
  }
  if (!patch_->WriteDiffStream(diff.data(), diff.size())) {
    LOG(ERROR) << "Writing " << diff.size() << " bytes to the diff stream"
               << endl;
    return false;
  }

  if (!patch_->WriteExtraStream(new_buf_ + written_output_ + entry.diff_size,
                                entry.extra_size)) {
    LOG(ERROR) << "Writing " << entry.extra_size << " bytes to the extra stream"
               << endl;
    return false;
  }

  old_pos_ += entry.diff_size + entry.offset_increment;
  written_output_ += entry.diff_size + entry.extra_size;

  return true;
}

bool DiffEncoder::Close() {
  if (written_output_ != new_size_) {
    LOG(ERROR) << "Close() called but not all the output was written" << endl;
    return false;
  }
  return patch_->Close();
}

}  // namespace bsdiff
