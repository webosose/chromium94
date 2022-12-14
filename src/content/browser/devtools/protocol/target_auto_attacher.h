// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_PROTOCOL_TARGET_AUTO_ATTACHER_H_
#define CONTENT_BROWSER_DEVTOOLS_PROTOCOL_TARGET_AUTO_ATTACHER_H_

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/memory/scoped_refptr.h"
#include "base/observer_list.h"

namespace content {

class DevToolsAgentHost;
class DevToolsAgentHostImpl;
class DevToolsRendererChannel;
class NavigationRequest;

namespace protocol {

class TargetAutoAttacher {
 public:
  class Client : public base::CheckedObserver {
   public:
    virtual bool AutoAttach(DevToolsAgentHost* host,
                            bool waiting_for_debugger) = 0;
    virtual void AutoDetach(DevToolsAgentHost* host) = 0;
    virtual void SetAttachedTargetsOfType(
        const base::flat_set<scoped_refptr<DevToolsAgentHost>>& hosts,
        const std::string& type) = 0;

   protected:
    Client() = default;
    ~Client() override = default;
  };

  virtual ~TargetAutoAttacher();

  void AddClient(Client* client,
                 bool wait_for_debugger_on_start,
                 base::OnceClosure callback);
  void RemoveClient(Client* client);

  DevToolsAgentHost* AutoAttachToFrame(NavigationRequest* navigation_request,
                                       bool wait_for_debugger_on_start);

 protected:
  using Hosts = base::flat_set<scoped_refptr<DevToolsAgentHost>>;

  TargetAutoAttacher();

  bool auto_attach() const;
  bool wait_for_debugger_on_start() const;

  virtual void UpdateAutoAttach(base::OnceClosure callback);

  bool DispatchAutoAttach(DevToolsAgentHost* host, bool waiting_for_debugger);
  void DispatchAutoDetach(DevToolsAgentHost* host);
  void DispatchSetAttachedTargetsOfType(
      const base::flat_set<scoped_refptr<DevToolsAgentHost>>& hosts,
      const std::string& type);

 private:
  base::ObserverList<Client, true, true> clients_;
  base::flat_set<Client*> clients_requesting_wait_for_debugger_;

  DISALLOW_COPY_AND_ASSIGN(TargetAutoAttacher);
};

class RendererAutoAttacherBase : public TargetAutoAttacher {
 public:
  explicit RendererAutoAttacherBase(DevToolsRendererChannel* renderer_channel);
  ~RendererAutoAttacherBase() override;

 protected:
  void UpdateAutoAttach(base::OnceClosure callback) override;
  void ChildWorkerCreated(DevToolsAgentHostImpl* agent_host,
                          bool waiting_for_debugger);

 private:
  DevToolsRendererChannel* const renderer_channel_;
};

}  // namespace protocol
}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PROTOCOL_TARGET_AUTO_ATTACHER_H_
