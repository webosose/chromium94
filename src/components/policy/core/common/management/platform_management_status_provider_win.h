// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_MANAGEMENT_PLATFORM_MANAGEMENT_STATUS_PROVIDER_WIN_H_
#define COMPONENTS_POLICY_CORE_COMMON_MANAGEMENT_PLATFORM_MANAGEMENT_STATUS_PROVIDER_WIN_H_

#include "build/build_config.h"
#include "components/policy/core/common/management/management_service.h"
#include "components/policy/policy_export.h"

namespace policy {

#if defined(OS_WIN)
class POLICY_EXPORT DomainEnrollmentStatusProvider final
    : public ManagementStatusProvider {
 public:
  DomainEnrollmentStatusProvider();
  ~DomainEnrollmentStatusProvider() override;

  // ManagementStatusProvider impl
  bool IsManaged() override;
  EnterpriseManagementAuthority GetAuthority() override;

  static bool IsEnrolledToDomain();
};
#endif

class POLICY_EXPORT EnterpriseMDMManagementStatusProvider final
    : public ManagementStatusProvider {
 public:
  EnterpriseMDMManagementStatusProvider();
  ~EnterpriseMDMManagementStatusProvider() override;

  // ManagementStatusProvider impl
  bool IsManaged() override;
  EnterpriseManagementAuthority GetAuthority() override;

  static bool IsEnrolledToDomain();
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_MANAGEMENT_PLATFORM_MANAGEMENT_STATUS_PROVIDER_WIN_H_
