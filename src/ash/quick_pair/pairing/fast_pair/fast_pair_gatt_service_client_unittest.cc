// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/quick_pair/pairing/fast_pair/fast_pair_gatt_service_client_impl.h"

#include <stddef.h>

#include "ash/quick_pair/common/constants.h"
#include "ash/quick_pair/common/logging.h"
#include "ash/quick_pair/common/pair_failure.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/test/mock_bluetooth_adapter.h"
#include "device/bluetooth/test/mock_bluetooth_device.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_characteristic.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_connection.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_notify_session.h"
#include "device/bluetooth/test/mock_bluetooth_gatt_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace {

using NotifySessionCallback = base::OnceCallback<void(
    std::unique_ptr<device::BluetoothGattNotifySession>)>;
using ErrorCallback =
    base::OnceCallback<void(device::BluetoothGattService::GattErrorCode)>;

constexpr base::TimeDelta kConnectingTestTimeout =
    base::TimeDelta::FromSeconds(5);

// Below constants are used to construct MockBluetoothDevice for testing.
constexpr char kTestBleDeviceAddress[] = "11:12:13:14:15:16";
const char kTestServiceId[] = "service_id1";
const std::string kUUIDString1 = "keybased";
const std::string kUUIDString2 = "passkey";
const std::string kUUIDString3 = "accountkey";
const device::BluetoothUUID kNonFastPairUuid("0xFE2B");

const device::BluetoothUUID kKeyBasedCharacteristicUuid1("1234");
const device::BluetoothUUID kKeyBasedCharacteristicUuid2(
    "FE2C1234-8366-4814-8EB0-01DE32100BEA");
const device::BluetoothUUID kPasskeyCharacteristicUuid1("1235");
const device::BluetoothUUID kPasskeyCharacteristicUuid2(
    "FE2C1235-8366-4814-8EB0-01DE32100BEA");
const device::BluetoothUUID kAccountKeyCharacteristicUuid1("1236");
const device::BluetoothUUID kAccountKeyCharacteristicUuid2(
    "FE2C1236-8366-4814-8EB0-01DE32100BEA");

const uint8_t kMessageType = 0x00;
const uint8_t kFlags = 0x00;
const std::string kProviderAddress = "abcde";
const std::string kSeekersAddress = "abcde";
const std::vector<uint8_t>& kTestWriteResponse{0x01, 0x03, 0x02, 0x01, 0x02};

const device::BluetoothRemoteGattCharacteristic::Properties kProperties =
    device::BluetoothRemoteGattCharacteristic::PROPERTY_READ |
    device::BluetoothRemoteGattCharacteristic::PROPERTY_WRITE_WITHOUT_RESPONSE |
    device::BluetoothRemoteGattCharacteristic::PROPERTY_INDICATE;

const device::BluetoothRemoteGattCharacteristic::Permissions kPermissions =
    device::BluetoothRemoteGattCharacteristic::PERMISSION_READ_ENCRYPTED |
    device::BluetoothRemoteGattCharacteristic::PERMISSION_WRITE_ENCRYPTED;

class FakeBluetoothAdapter
    : public testing::NiceMock<device::MockBluetoothAdapter> {
 public:
  FakeBluetoothAdapter() = default;

  // Move-only class
  FakeBluetoothAdapter(const FakeBluetoothAdapter&) = delete;
  FakeBluetoothAdapter& operator=(const FakeBluetoothAdapter&) = delete;

  device::BluetoothDevice* GetDevice(const std::string& address) override {
    for (const auto& it : mock_devices_) {
      if (it->GetAddress() == address)
        return it.get();
    }
    return nullptr;
  }

  void NotifyGattDiscoveryCompleteForService(
      device::BluetoothRemoteGattService* service) {
    device::BluetoothAdapter::NotifyGattDiscoveryComplete(service);
  }

  void NotifyGattCharacteristicValueChanged(
      device::BluetoothRemoteGattCharacteristic* characteristic) {
    device::BluetoothAdapter::NotifyGattCharacteristicValueChanged(
        characteristic, kTestWriteResponse);
  }

 protected:
  ~FakeBluetoothAdapter() override = default;
};

class FakeBluetoothDevice
    : public testing::NiceMock<device::MockBluetoothDevice> {
 public:
  FakeBluetoothDevice(FakeBluetoothAdapter* adapter, const std::string& address)
      : testing::NiceMock<device::MockBluetoothDevice>(adapter,
                                                       /*bluetooth_class=*/0u,
                                                       /*name=*/"Test Device",
                                                       address,
                                                       /*paired=*/true,
                                                       /*connected=*/true),
        fake_adapter_(adapter) {}

  void CreateGattConnection(
      device::BluetoothDevice::GattConnectionCallback callback,
      absl::optional<device::BluetoothUUID> service_uuid =
          absl::nullopt) override {
    if (has_gatt_connection_error_) {
      std::move(callback).Run(
          std::make_unique<
              testing::NiceMock<device::MockBluetoothGattConnection>>(
              fake_adapter_, kTestBleDeviceAddress),
          /*error_code=*/device::BluetoothDevice::ConnectErrorCode::
              ERROR_FAILED);
    } else {
      std::move(callback).Run(
          std::make_unique<
              testing::NiceMock<device::MockBluetoothGattConnection>>(
              fake_adapter_, kTestBleDeviceAddress),
          /*error_code=*/absl::nullopt);
    }
  }

  void SetError(bool has_gatt_connection_error) {
    has_gatt_connection_error_ = has_gatt_connection_error;
  }

  // Move-only class
  FakeBluetoothDevice(const FakeBluetoothDevice&) = delete;
  FakeBluetoothDevice& operator=(const FakeBluetoothDevice&) = delete;

 protected:
  bool has_gatt_connection_error_ = false;
  FakeBluetoothAdapter* fake_adapter_;
};

class FakeBluetoothGattCharacteristic
    : public testing::NiceMock<device::MockBluetoothGattCharacteristic> {
 public:
  FakeBluetoothGattCharacteristic(device::MockBluetoothGattService* service,
                                  const std::string& identifier,
                                  const device::BluetoothUUID& uuid,
                                  Properties properties,
                                  Permissions permissions)
      : testing::NiceMock<device::MockBluetoothGattCharacteristic>(
            service,
            identifier,
            uuid,
            properties,
            permissions) {}

  // Move-only class
  FakeBluetoothGattCharacteristic(const FakeBluetoothGattCharacteristic&) =
      delete;
  FakeBluetoothGattCharacteristic operator=(
      const FakeBluetoothGattCharacteristic&) = delete;

  void StartNotifySession(NotifySessionCallback callback,
                          ErrorCallback error_callback) override {
    if (notify_session_error_) {
      std::move(error_callback)
          .Run(device::BluetoothGattService::GATT_ERROR_NOT_PERMITTED);
      return;
    }

    auto fake_notify_session = std::make_unique<
        testing::NiceMock<device::MockBluetoothGattNotifySession>>(
        GetWeakPtr());

    if (timeout_)
      task_environment_->FastForwardBy(kConnectingTestTimeout);

    std::move(callback).Run(std::move(fake_notify_session));
  }

  void WriteRemoteCharacteristic(const std::vector<uint8_t>& value,
                                 WriteType write_type,
                                 base::OnceClosure callback,
                                 ErrorCallback error_callback) override {
    if (write_remote_error_) {
      std::move(error_callback)
          .Run(device::BluetoothGattService::GATT_ERROR_NOT_PERMITTED);
      return;
    }

    std::move(callback).Run();
  }

  void SetWriteError(bool write_remote_error) {
    write_remote_error_ = write_remote_error;
  }

  void SetNotifySessionError(bool notify_session_error) {
    notify_session_error_ = notify_session_error;
  }

  void SetNotifySessionTimeout(bool timeout,
                               base::test::TaskEnvironment* task_environment) {
    timeout_ = timeout;
    task_environment_ = task_environment;
  }

 private:
  bool notify_session_error_ = false;
  bool write_remote_error_ = false;
  bool timeout_ = false;
  base::test::TaskEnvironment* task_environment_ = nullptr;
};

std::unique_ptr<FakeBluetoothDevice> CreateTestBluetoothDevice(
    FakeBluetoothAdapter* adapter,
    device::BluetoothUUID uuid) {
  auto mock_device = std::make_unique<FakeBluetoothDevice>(
      /*adapter=*/adapter, kTestBleDeviceAddress);
  mock_device->AddUUID(uuid);
  mock_device->SetServiceDataForUUID(uuid, {1, 2, 3});
  return mock_device;
}

}  // namespace

namespace ash {
namespace quick_pair {

class FastPairGattServiceClientTest : public testing::Test {
 public:
  void SuccessfulGattConnectionSetUp() {
    adapter_ = base::MakeRefCounted<FakeBluetoothAdapter>();
    device_ = CreateTestBluetoothDevice(
        adapter_.get(), ash::quick_pair::kFastPairBluetoothUuid);
    adapter_->AddMockDevice(std::move(device_));
    gatt_service_client_ = FastPairGattServiceClientImpl::Factory::Create(
        adapter_->GetDevice(kTestBleDeviceAddress), adapter_.get(),
        base::BindRepeating(&::ash::quick_pair::FastPairGattServiceClientTest::
                                InitializedTestCallback,
                            weak_ptr_factory_.GetWeakPtr()));
  }

  void FailedGattConnectionSetUp() {
    adapter_ = base::MakeRefCounted<FakeBluetoothAdapter>();
    device_ = CreateTestBluetoothDevice(
        adapter_.get(), ash::quick_pair::kFastPairBluetoothUuid);
    device_->SetError(true);
    adapter_->AddMockDevice(std::move(device_));
    gatt_service_client_ = FastPairGattServiceClientImpl::Factory::Create(
        adapter_->GetDevice(kTestBleDeviceAddress), adapter_.get(),
        base::BindRepeating(&::ash::quick_pair::FastPairGattServiceClientTest::
                                InitializedTestCallback,
                            weak_ptr_factory_.GetWeakPtr()));
  }

  void NonFastPairServiceDataSetUp() {
    adapter_ = base::MakeRefCounted<FakeBluetoothAdapter>();
    device_ = CreateTestBluetoothDevice(adapter_.get(), kNonFastPairUuid);
    adapter_->AddMockDevice(std::move(device_));
    gatt_service_client_ = FastPairGattServiceClientImpl::Factory::Create(
        adapter_->GetDevice(kTestBleDeviceAddress), adapter_.get(),
        base::BindRepeating(&::ash::quick_pair::FastPairGattServiceClientTest::
                                InitializedTestCallback,
                            weak_ptr_factory_.GetWeakPtr()));
  }

  void NotifyGattDiscoveryCompleteForService() {
    auto gatt_service =
        std::make_unique<testing::NiceMock<device::MockBluetoothGattService>>(
            CreateTestBluetoothDevice(adapter_.get(),
                                      ash::quick_pair::kFastPairBluetoothUuid)
                .get(),
            kTestServiceId, ash::quick_pair::kFastPairBluetoothUuid,
            /*is_primary=*/true);
    gatt_service_ = std::move(gatt_service);
    ON_CALL(*(gatt_service_.get()), GetDevice)
        .WillByDefault(
            testing::Return(adapter_->GetDevice(kTestBleDeviceAddress)));
    if (!keybased_char_error_) {
      fake_key_based_characteristic_ =
          std::make_unique<FakeBluetoothGattCharacteristic>(
              gatt_service_.get(), kUUIDString1, kKeyBasedCharacteristicUuid1,
              kProperties, kPermissions);

      if (keybased_notify_session_error_)
        fake_key_based_characteristic_->SetNotifySessionError(true);

      if (keybased_notify_session_timeout_) {
        fake_key_based_characteristic_->SetNotifySessionTimeout(
            true, &task_environment_);
      }

      if (key_based_write_error_) {
        fake_key_based_characteristic_->SetWriteError(true);
      }

      temp_fake_key_based_characteristic_ =
          fake_key_based_characteristic_.get();
      gatt_service_->AddMockCharacteristic(
          std::move(fake_key_based_characteristic_));
    }

    if (!passkey_char_error_) {
      fake_passkey_characteristic_ =
          std::make_unique<FakeBluetoothGattCharacteristic>(
              gatt_service_.get(), kUUIDString2, kPasskeyCharacteristicUuid1,
              kProperties, kPermissions);

      if (passkey_notify_session_error_)
        fake_passkey_characteristic_->SetNotifySessionError(true);

      if (passkey_notify_session_timeout_) {
        fake_passkey_characteristic_->SetNotifySessionTimeout(
            true, &task_environment_);
      }

      gatt_service_->AddMockCharacteristic(
          std::move(fake_passkey_characteristic_));
    }

    auto fake_account_key_characteristic =
        std::make_unique<FakeBluetoothGattCharacteristic>(
            gatt_service_.get(), kUUIDString3, kAccountKeyCharacteristicUuid1,
            kProperties, kPermissions);
    gatt_service_->AddMockCharacteristic(
        std::move(fake_account_key_characteristic));
    adapter_->NotifyGattDiscoveryCompleteForService(gatt_service_.get());
  }

  bool ServiceIsSet() {
    if (!gatt_service_client_->gatt_service())
      return false;
    return gatt_service_client_->gatt_service() == gatt_service_.get();
  }

  void SetPasskeyCharacteristicError(bool passkey_char_error) {
    passkey_char_error_ = passkey_char_error;
  }

  void SetKeybasedCharacteristicError(bool keybased_char_error) {
    keybased_char_error_ = keybased_char_error;
  }

  void SetPasskeyNotifySessionError(bool passkey_notify_session_error) {
    passkey_notify_session_error_ = passkey_notify_session_error;
  }

  void SetKeybasedNotifySessionError(bool keybased_notify_session_error) {
    keybased_notify_session_error_ = keybased_notify_session_error;
  }

  void InitializedTestCallback(absl::optional<PairFailure> failure) {
    initalized_failure_ = failure;
  }

  absl::optional<PairFailure> GetInitializedCallbackResult() {
    return initalized_failure_;
  }

  void WriteTestCallback(std::vector<uint8_t> response,
                         absl::optional<PairFailure> failure) {
    write_failure_ = failure;
  }

  absl::optional<PairFailure> GetWriteCallbackResult() {
    return write_failure_;
  }

  void SetPasskeyNotifySessionTimeout(bool timeout) {
    passkey_notify_session_timeout_ = timeout;
  }

  void SetKeybasedNotifySessionTimeout(bool timeout) {
    keybased_notify_session_timeout_ = timeout;
  }

  void FastForwardTimeByConnetingTimeout() {
    task_environment_.FastForwardBy(kConnectingTestTimeout);
  }

  void WriteRequestToKeyBased() {
    gatt_service_client_->WriteRequestAsync(
        kMessageType, kFlags, kProviderAddress, kSeekersAddress,
        base::BindRepeating(&::ash::quick_pair::FastPairGattServiceClientTest::
                                WriteTestCallback,
                            weak_ptr_factory_.GetWeakPtr()));
  }

  void TriggerKeyBasedGattChanged() {
    adapter_->NotifyGattCharacteristicValueChanged(
        temp_fake_key_based_characteristic_);
  }

  void SetKeyBasedWriteError() { key_based_write_error_ = true; }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};

 private:
  // We need temporary pointers to use for write/ready requests because we
  // move the unique pointers when we notify the session.
  FakeBluetoothGattCharacteristic* temp_fake_key_based_characteristic_;
  bool passkey_char_error_ = false;
  bool keybased_char_error_ = false;
  bool passkey_notify_session_error_ = false;
  bool keybased_notify_session_error_ = false;
  bool passkey_notify_session_timeout_ = false;
  bool keybased_notify_session_timeout_ = false;
  bool key_based_write_error_ = false;

  absl::optional<PairFailure> initalized_failure_;
  absl::optional<PairFailure> write_failure_;
  scoped_refptr<FakeBluetoothAdapter> adapter_;
  std::unique_ptr<FakeBluetoothDevice> device_;
  std::unique_ptr<FakeBluetoothGattCharacteristic>
      fake_key_based_characteristic_;
  std::unique_ptr<FakeBluetoothGattCharacteristic> fake_passkey_characteristic_;
  std::unique_ptr<testing::NiceMock<device::MockBluetoothGattService>>
      gatt_service_;
  std::unique_ptr<FastPairGattServiceClient> gatt_service_client_;
  base::WeakPtrFactory<FastPairGattServiceClientTest> weak_ptr_factory_{this};
};

TEST_F(FastPairGattServiceClientTest, GattServiceDiscoveryTimeout) {
  SuccessfulGattConnectionSetUp();
  FastForwardTimeByConnetingTimeout();
  NotifyGattDiscoveryCompleteForService();
  EXPECT_EQ(GetInitializedCallbackResult(),
            PairFailure::kGattServiceDiscoveryTimeout);
  EXPECT_FALSE(ServiceIsSet());
}

TEST_F(FastPairGattServiceClientTest, FailedGattConnection) {
  FailedGattConnectionSetUp();
  EXPECT_EQ(GetInitializedCallbackResult(), PairFailure::kCreateGattConnection);
  EXPECT_FALSE(ServiceIsSet());
}

TEST_F(FastPairGattServiceClientTest, IgnoreNonFastPairServices) {
  NonFastPairServiceDataSetUp();
  EXPECT_EQ(GetInitializedCallbackResult(), absl::nullopt);
  EXPECT_FALSE(ServiceIsSet());
}

TEST_F(FastPairGattServiceClientTest, FailedKeyBasedCharacteristics) {
  SetKeybasedCharacteristicError(true);
  SuccessfulGattConnectionSetUp();
  NotifyGattDiscoveryCompleteForService();
  EXPECT_EQ(GetInitializedCallbackResult(),
            PairFailure::kKeyBasedPairingCharacteristicDiscovery);
  EXPECT_FALSE(ServiceIsSet());
}

TEST_F(FastPairGattServiceClientTest, FailedPasskeyCharacteristics) {
  SetPasskeyCharacteristicError(true);
  SuccessfulGattConnectionSetUp();
  NotifyGattDiscoveryCompleteForService();
  EXPECT_EQ(GetInitializedCallbackResult(),
            PairFailure::kPasskeyCharacteristicDiscovery);
  EXPECT_FALSE(ServiceIsSet());
}

TEST_F(FastPairGattServiceClientTest, SuccessfulCharacteristicsStartNotify) {
  SetKeybasedCharacteristicError(false);
  SetPasskeyCharacteristicError(false);
  SuccessfulGattConnectionSetUp();
  NotifyGattDiscoveryCompleteForService();
  EXPECT_EQ(GetInitializedCallbackResult(), absl::nullopt);
  EXPECT_TRUE(ServiceIsSet());
}

TEST_F(FastPairGattServiceClientTest, StartNotifyPasskeyFailure) {
  SuccessfulGattConnectionSetUp();
  SetPasskeyNotifySessionError(true);
  NotifyGattDiscoveryCompleteForService();
  EXPECT_EQ(GetInitializedCallbackResult(),
            PairFailure::kPasskeyCharacteristicNotifySession);
  EXPECT_FALSE(ServiceIsSet());
}

TEST_F(FastPairGattServiceClientTest, StartNotifyKeybasedFailure) {
  SuccessfulGattConnectionSetUp();
  SetKeybasedNotifySessionError(true);
  NotifyGattDiscoveryCompleteForService();
  EXPECT_EQ(GetInitializedCallbackResult(),
            PairFailure::kKeyBasedPairingCharacteristicNotifySession);
  EXPECT_FALSE(ServiceIsSet());
}

TEST_F(FastPairGattServiceClientTest, PasskeyStartNotifyTimeout) {
  SetPasskeyNotifySessionTimeout(true);
  SuccessfulGattConnectionSetUp();
  NotifyGattDiscoveryCompleteForService();
  EXPECT_EQ(GetInitializedCallbackResult(),
            PairFailure::kPasskeyCharacteristicNotifySessionTimeout);
  EXPECT_FALSE(ServiceIsSet());
}

TEST_F(FastPairGattServiceClientTest, KeyBasedStartNotifyTimeout) {
  SetKeybasedNotifySessionTimeout(true);
  SuccessfulGattConnectionSetUp();
  NotifyGattDiscoveryCompleteForService();
  EXPECT_EQ(GetInitializedCallbackResult(),
            PairFailure::kKeyBasedPairingCharacteristicNotifySessionTimeout);
  EXPECT_FALSE(ServiceIsSet());
}

TEST_F(FastPairGattServiceClientTest, WriteKeyBasedRequest) {
  SuccessfulGattConnectionSetUp();
  NotifyGattDiscoveryCompleteForService();
  EXPECT_EQ(GetInitializedCallbackResult(), absl::nullopt);
  EXPECT_TRUE(ServiceIsSet());
  WriteRequestToKeyBased();
  TriggerKeyBasedGattChanged();
  EXPECT_EQ(GetWriteCallbackResult(), absl::nullopt);
}

TEST_F(FastPairGattServiceClientTest, WriteKeyBasedRequestError) {
  SetKeyBasedWriteError();
  SuccessfulGattConnectionSetUp();
  NotifyGattDiscoveryCompleteForService();
  EXPECT_EQ(GetInitializedCallbackResult(), absl::nullopt);
  EXPECT_TRUE(ServiceIsSet());
  WriteRequestToKeyBased();
  TriggerKeyBasedGattChanged();
  EXPECT_NE(GetWriteCallbackResult(), absl::nullopt);
}

}  // namespace quick_pair
}  // namespace ash
