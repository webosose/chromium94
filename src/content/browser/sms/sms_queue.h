// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SMS_SMS_QUEUE_H_
#define CONTENT_BROWSER_SMS_SMS_QUEUE_H_

#include <map>

#include "base/observer_list.h"
#include "content/common/content_export.h"
#include "content/public/browser/sms_fetcher.h"
#include "url/origin.h"

namespace content {

// SmsQueue manages the queue of pending requests for each origin.
class CONTENT_EXPORT SmsQueue {
 public:
  SmsQueue();
  ~SmsQueue();

  using FailureType = SmsFetchFailureType;
  using Subscriber = SmsFetcher::Subscriber;

  void Push(const OriginList& origin_list, Subscriber* subscriber);
  Subscriber* Pop(const OriginList& origin_list);
  void Remove(const OriginList& origin_list, Subscriber* subscriber);
  bool HasSubscribers();
  bool HasSubscriber(const OriginList& origin_list,
                     const Subscriber* subscriber);
  // Pops and notifies the first subscriber of an origin iff there's one origin
  // in the |subscribers_| map. Returns true if the failure is handled.
  bool NotifyFailure(FailureType failure_type);

 private:
  std::map<OriginList, base::ObserverList<Subscriber>> subscribers_;

  DISALLOW_COPY_AND_ASSIGN(SmsQueue);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SMS_SMS_QUEUE_H_
