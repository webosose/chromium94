// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_PROTOCOL_TARGET_HANDLER_H_
#define CONTENT_BROWSER_DEVTOOLS_PROTOCOL_TARGET_HANDLER_H_

#include <map>
#include <set>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/devtools/devtools_throttle_handle.h"
#include "content/browser/devtools/protocol/devtools_domain_handler.h"
#include "content/browser/devtools/protocol/target.h"
#include "content/browser/devtools/protocol/target_auto_attacher.h"
#include "content/public/browser/devtools_agent_host_observer.h"
#include "net/proxy_resolution/proxy_config.h"
#include "services/network/public/mojom/network_context.mojom.h"

namespace content {

class DevToolsAgentHostImpl;
class DevToolsSession;
class NavigationHandle;
class NavigationThrottle;

namespace protocol {

class TargetHandler : public DevToolsDomainHandler,
                      public Target::Backend,
                      public DevToolsAgentHostObserver,
                      public TargetAutoAttacher::Client {
 public:
  enum class AccessMode {
    // Only setAutoAttach is supported. Any non-related target are not
    // accessible.
    kAutoAttachOnly,
    // Standard mode of operation: both auto-attach and discovery.
    kRegular,
    // This mode also allows advanced method like Target.exposeDevToolsProtocol,
    // which should not be exposed on a non-browser-wide connection.
    kBrowser
  };
  TargetHandler(AccessMode access_mode,
                const std::string& owner_target_id,
                TargetAutoAttacher* auto_attacher,
                DevToolsSession* root_session);
  ~TargetHandler() override;

  static std::vector<TargetHandler*> ForAgentHost(DevToolsAgentHostImpl* host);

  void Wire(UberDispatcher* dispatcher) override;
  Response Disable() override;

  std::unique_ptr<NavigationThrottle> CreateThrottleForNavigation(
      NavigationHandle* navigation_handle);

  void UpdatePortals();
  bool ShouldThrottlePopups() const;

  // Domain implementation.
  Response SetDiscoverTargets(bool discover) override;
  void SetAutoAttach(bool auto_attach,
                     bool wait_for_debugger_on_start,
                     Maybe<bool> flatten,
                     std::unique_ptr<SetAutoAttachCallback> callback) override;
  Response SetRemoteLocations(
      std::unique_ptr<protocol::Array<Target::RemoteLocation>>) override;
  Response AttachToTarget(const std::string& target_id,
                          Maybe<bool> flatten,
                          std::string* out_session_id) override;
  Response AttachToBrowserTarget(std::string* out_session_id) override;
  Response DetachFromTarget(Maybe<std::string> session_id,
                            Maybe<std::string> target_id) override;
  Response SendMessageToTarget(const std::string& message,
                               Maybe<std::string> session_id,
                               Maybe<std::string> target_id) override;
  Response GetTargetInfo(
      Maybe<std::string> target_id,
      std::unique_ptr<Target::TargetInfo>* target_info) override;
  Response ActivateTarget(const std::string& target_id) override;
  Response CloseTarget(const std::string& target_id,
                       bool* out_success) override;
  Response ExposeDevToolsProtocol(const std::string& target_id,
                                  Maybe<std::string> binding_name) override;
  void CreateBrowserContext(
      Maybe<bool> in_disposeOnDetach,
      Maybe<String> in_proxyServer,
      Maybe<String> in_proxyBypassList,
      std::unique_ptr<CreateBrowserContextCallback> callback) override;
  void DisposeBrowserContext(
      const std::string& context_id,
      std::unique_ptr<DisposeBrowserContextCallback> callback) override;
  Response GetBrowserContexts(
      std::unique_ptr<protocol::Array<String>>* browser_context_ids) override;
  Response CreateTarget(const std::string& url,
                        Maybe<int> width,
                        Maybe<int> height,
                        Maybe<std::string> context_id,
                        Maybe<bool> enable_begin_frame_control,
                        Maybe<bool> new_window,
                        Maybe<bool> background,
                        std::string* out_target_id) override;
  Response GetTargets(
      std::unique_ptr<protocol::Array<Target::TargetInfo>>* target_infos)
      override;

  void ApplyNetworkContextParamsOverrides(
      BrowserContext* browser_context,
      network::mojom::NetworkContextParams* network_context_params);

  // Adds a ServiceWorker throttle for an auto attaching session. If none is
  // known for this `agent_host`, is a no-op.
  void AddServiceWorkerThrottle(
      DevToolsAgentHost* agent_host,
      scoped_refptr<DevToolsThrottleHandle> throttle_handle);

 private:
  class Session;
  class Throttle;
  class RequestThrottle;
  class ResponseThrottle;

  // TargetAutoAttacher::Delegate implementation.
  bool AutoAttach(DevToolsAgentHost* host, bool waiting_for_debugger) override;
  void AutoDetach(DevToolsAgentHost* host) override;
  void SetAttachedTargetsOfType(
      const base::flat_set<scoped_refptr<DevToolsAgentHost>>& new_hosts,
      const std::string& type) override;

  Response FindSession(Maybe<std::string> session_id,
                       Maybe<std::string> target_id,
                       Session** session);
  void ClearThrottles();
  void SetAutoAttachInternal(bool auto_attach,
                             bool wait_for_debugger_on_start,
                             bool flatten,
                             base::OnceClosure callback);
  void UpdateAgentHostObserver();

  // DevToolsAgentHostObserver implementation.
  bool ShouldForceDevToolsAgentHostCreation() override;
  void DevToolsAgentHostCreated(DevToolsAgentHost* agent_host) override;
  void DevToolsAgentHostNavigated(DevToolsAgentHost* agent_host) override;
  void DevToolsAgentHostDestroyed(DevToolsAgentHost* agent_host) override;
  void DevToolsAgentHostAttached(DevToolsAgentHost* agent_host) override;
  void DevToolsAgentHostDetached(DevToolsAgentHost* agent_host) override;
  void DevToolsAgentHostCrashed(DevToolsAgentHost* agent_host,
                                base::TerminationStatus status) override;

  TargetAutoAttacher* const auto_attacher_;
  std::unique_ptr<Target::Frontend> frontend_;
  bool flatten_auto_attach_ = false;
  bool auto_attach_ = false;
  bool wait_for_debugger_on_start_ = false;
  bool discover_;
  bool observing_agent_hosts_ = false;
  std::map<std::string, std::unique_ptr<Session>> attached_sessions_;
  std::map<DevToolsAgentHost*, Session*> auto_attached_sessions_;
  std::set<DevToolsAgentHost*> reported_hosts_;
  base::flat_set<std::string> dispose_on_detach_context_ids_;
  base::flat_map<std::string, net::ProxyConfig> contexts_with_overridden_proxy_;
  AccessMode access_mode_;
  std::string owner_target_id_;
  DevToolsSession* root_session_;
  base::flat_set<Throttle*> throttles_;
  absl::optional<net::ProxyConfig> pending_proxy_config_;
  base::WeakPtrFactory<TargetHandler> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(TargetHandler);
};

}  // namespace protocol
}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PROTOCOL_TARGET_HANDLER_H_
