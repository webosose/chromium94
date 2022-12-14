// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/content/browser/web_api_handshake_checker.h"

#include "components/safe_browsing/content/browser/web_ui/safe_browsing_ui.h"
#include "components/safe_browsing/core/browser/safe_browsing_url_checker_impl.h"
#include "components/safe_browsing/core/browser/url_checker_delegate.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/http/http_request_headers.h"

namespace safe_browsing {

class WebApiHandshakeChecker::CheckerOnIO
    : public base::SupportsWeakPtr<WebApiHandshakeChecker::CheckerOnIO> {
 public:
  CheckerOnIO(base::WeakPtr<WebApiHandshakeChecker> handshake_checker,
              GetDelegateCallback delegate_getter,
              const GetWebContentsCallback& web_contents_getter,
              int frame_tree_node_id)
      : handshake_checker_(std::move(handshake_checker)),
        delegate_getter_(std::move(delegate_getter)),
        web_contents_getter_(web_contents_getter),
        frame_tree_node_id_(frame_tree_node_id) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    DCHECK(handshake_checker_);
    DCHECK(delegate_getter_);
    DCHECK(web_contents_getter_);
  }

  void Check(const GURL& url) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    DCHECK(delegate_getter_);
    DCHECK(web_contents_getter_);

    scoped_refptr<UrlCheckerDelegate> url_checker_delegate =
        std::move(delegate_getter_).Run();
    bool skip_checks =
        !url_checker_delegate ||
        !url_checker_delegate->GetDatabaseManager()->IsSupported() ||
        url_checker_delegate->ShouldSkipRequestCheck(
            url, frame_tree_node_id_,
            /*render_process_id=*/content::ChildProcessHost::kInvalidUniqueID,
            /*render_frame_id=*/MSG_ROUTING_NONE,
            /*originated_from_service_worker=*/false);
    if (skip_checks) {
      OnCompleteCheck(/*slow_check=*/false, /*proceed=*/true,
                      /*showed_interstitial=*/false);
      return;
    }

    url_checker_ = std::make_unique<SafeBrowsingUrlCheckerImpl>(
        net::HttpRequestHeaders(), /*load_flags=*/0,
        network::mojom::RequestDestination::kEmpty, /*has_user_gesture=*/false,
        url_checker_delegate, web_contents_getter_,
        /*render_process_id=*/content::ChildProcessHost::kInvalidUniqueID,
        /*render_frame_id=*/MSG_ROUTING_NONE, frame_tree_node_id_,
        /*real_time_lookup_enabled=*/false,
        /*can_rt_check_subresource_url=*/false,
        /*can_check_db=*/true, content::GetUIThreadTaskRunner({}),
        /*url_lookup_service=*/nullptr, WebUIInfoSingleton::GetInstance());
    url_checker_->CheckUrl(
        url, "GET",
        base::BindOnce(&WebApiHandshakeChecker::CheckerOnIO::OnCheckUrlResult,
                       base::Unretained(this)));
  }

 private:
  // See comments in BrowserUrlLoaderThrottle::OnCheckUrlResult().
  void OnCheckUrlResult(
      SafeBrowsingUrlCheckerImpl::NativeUrlCheckNotifier* slow_check_notifier,
      bool proceed,
      bool showed_interstitial) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    if (!slow_check_notifier) {
      OnCompleteCheck(/*slow_check=*/false, proceed, showed_interstitial);
      return;
    }

    *slow_check_notifier =
        base::BindOnce(&WebApiHandshakeChecker::CheckerOnIO::OnCompleteCheck,
                       base::Unretained(this), /*slow_check=*/true);
  }

  void OnCompleteCheck(bool slow_check,
                       bool proceed,
                       bool showed_interstitial) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(&WebApiHandshakeChecker::OnCompleteCheck,
                                  handshake_checker_, slow_check, proceed,
                                  showed_interstitial));
  }

  base::WeakPtr<WebApiHandshakeChecker> handshake_checker_;
  GetDelegateCallback delegate_getter_;
  GetWebContentsCallback web_contents_getter_;
  const int frame_tree_node_id_;
  std::unique_ptr<SafeBrowsingUrlCheckerImpl> url_checker_;
};

WebApiHandshakeChecker::WebApiHandshakeChecker(
    GetDelegateCallback delegate_getter,
    const GetWebContentsCallback& web_contents_getter,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  io_checker_ = std::make_unique<CheckerOnIO>(
      weak_factory_.GetWeakPtr(), std::move(delegate_getter),
      web_contents_getter, frame_tree_node_id);
}

WebApiHandshakeChecker::~WebApiHandshakeChecker() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::GetIOThreadTaskRunner({})->DeleteSoon(FROM_HERE,
                                                 std::move(io_checker_));
}

void WebApiHandshakeChecker::Check(const GURL& url, CheckCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!check_callback_);
  check_callback_ = std::move(callback);
  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&WebApiHandshakeChecker::CheckerOnIO::Check,
                                io_checker_->AsWeakPtr(), url));
}

void WebApiHandshakeChecker::OnCompleteCheck(bool slow_check,
                                             bool proceed,
                                             bool showed_interstitial) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(check_callback_);

  CheckResult result = proceed ? CheckResult::kProceed : CheckResult::kBlocked;
  std::move(check_callback_).Run(result);
}

}  // namespace safe_browsing
