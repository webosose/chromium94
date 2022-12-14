// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feed/core/v2/stream/notice_card_tracker.h"

#include "base/time/time.h"
#include "components/feed/core/common/pref_names.h"
#include "components/feed/core/proto/v2/wire/content_id.pb.h"
#include "components/feed/core/v2/ios_shared_prefs.h"
#include "components/feed/core/v2/proto_util.h"
#include "components/feed/feed_feature_list.h"
#include "components/prefs/pref_service.h"

namespace feed {
namespace {

// The number of views of the notice card to consider it acknowledged by the
// user.
const int kViewsCountThreshold = 3;

bool IsPrivacyNoticeCard(const feedwire::ContentId& id) {
  // TODO(b/192015346): This is a less than ideal solution. We're relying on
  // the server to continue serving the notice card with this domain (and not
  // serving other types of cards with this domain). See the bug for the
  // suggested improvement.
  constexpr base::StringPiece kNoticeCardDomain = "privacynoticecard.f";
  return id.content_domain() == kNoticeCardDomain;
}

}  // namespace

NoticeCardTracker::NoticeCardTracker(PrefService* profile_prefs)
    : profile_prefs_(profile_prefs) {
  DCHECK(profile_prefs_);
  views_count_ = prefs::GetNoticeCardViewsCount(*profile_prefs_);
  has_clicked_ = prefs::GetNoticeCardClicksCount(*profile_prefs_) > 0;
}

void NoticeCardTracker::OnCardViewed(bool is_signed_in,
                                     const feedwire::ContentId& content_id) {
  if (!IsPrivacyNoticeCard(content_id))
    return;

  // Do two things if the notice card was viewed:
  // * If kInterestFeedV2ClicksAndViewsConditionalUpload is enabled, remember
  // the notice card was viewed.
  // * If the notice card is viewed and auto-dismiss is enabled, increment view
  // count.

  if (is_signed_in &&
      base::FeatureList::IsEnabled(
          feed::kInterestFeedV2ClicksAndViewsConditionalUpload)) {
    feed::prefs::SetHasReachedClickAndViewActionsUploadConditions(
        *profile_prefs_, true);
  }

  auto now = base::TimeTicks::Now();
  if (now - last_view_time_ < base::TimeDelta::FromMinutes(5))
    return;

  last_view_time_ = now;

  prefs::IncrementNoticeCardViewsCount(*profile_prefs_);
  views_count_++;
}

void NoticeCardTracker::OnOpenAction(const feedwire::ContentId& content_id) {
  if (!IsPrivacyNoticeCard(content_id) ||
      !prefs::GetLastFetchHadNoticeCard(*profile_prefs_) || has_clicked_) {
    return;
  }

  prefs::IncrementNoticeCardClicksCount(*profile_prefs_);
  has_clicked_ = true;
}

bool NoticeCardTracker::HasAcknowledgedNoticeCard() const {
  return has_clicked_ || (views_count_ >= kViewsCountThreshold);
}

}  // namespace feed
