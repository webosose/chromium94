// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/file_system_access/file_system_access_handle_base.h"

#include "base/files/scoped_temp_dir.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/task_environment.h"
#include "content/browser/file_system_access/mock_file_system_access_permission_grant.h"
#include "content/public/test/browser_task_environment.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/quota/quota_manager_proxy.h"
#include "storage/browser/test/test_file_system_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/storage_key/storage_key.h"

namespace content {

using base::test::RunOnceCallback;
using blink::mojom::PermissionStatus;
using storage::FileSystemURL;
using UserActivationState =
    FileSystemAccessPermissionGrant::UserActivationState;

class TestFileSystemAccessHandle : public FileSystemAccessHandleBase {
 public:
  TestFileSystemAccessHandle(FileSystemAccessManagerImpl* manager,
                             const BindingContext& context,
                             const storage::FileSystemURL& url,
                             const SharedHandleState& handle_state)
      : FileSystemAccessHandleBase(manager, context, url, handle_state) {}

 private:
  base::WeakPtr<FileSystemAccessHandleBase> AsWeakPtr() override {
    return weak_factory_.GetWeakPtr();
  }
  base::WeakPtrFactory<TestFileSystemAccessHandle> weak_factory_{this};
};

class FileSystemAccessHandleBaseTest : public testing::Test {
 public:
  FileSystemAccessHandleBaseTest()
      : task_environment_(base::test::TaskEnvironment::MainThreadType::IO) {}

  void SetUp() override {
    ASSERT_TRUE(dir_.CreateUniqueTempDir());
    file_system_context_ = storage::CreateFileSystemContextForTesting(
        /*quota_manager_proxy=*/nullptr, dir_.GetPath());

    chrome_blob_context_ = base::MakeRefCounted<ChromeBlobStorageContext>();
    chrome_blob_context_->InitializeOnIOThread(base::FilePath(),
                                               base::FilePath(), nullptr);

    manager_ = base::MakeRefCounted<FileSystemAccessManagerImpl>(
        file_system_context_, chrome_blob_context_,
        /*permission_context=*/nullptr,
        /*off_the_record=*/false);
  }

 protected:
  const GURL kTestURL = GURL("https://example.com/test");
  const blink::StorageKey kTestStorageKey =
      blink::StorageKey::CreateFromStringForTesting("https://example.com/");
  BrowserTaskEnvironment task_environment_;

  base::ScopedTempDir dir_;
  scoped_refptr<storage::FileSystemContext> file_system_context_;
  scoped_refptr<ChromeBlobStorageContext> chrome_blob_context_;

  scoped_refptr<MockFileSystemAccessPermissionGrant> read_grant_ =
      base::MakeRefCounted<
          testing::StrictMock<MockFileSystemAccessPermissionGrant>>();
  scoped_refptr<MockFileSystemAccessPermissionGrant> write_grant_ =
      base::MakeRefCounted<
          testing::StrictMock<MockFileSystemAccessPermissionGrant>>();

  scoped_refptr<FileSystemAccessManagerImpl> manager_;

  FileSystemAccessManagerImpl::SharedHandleState handle_state_ = {read_grant_,
                                                                  write_grant_};
};

TEST_F(FileSystemAccessHandleBaseTest, GetReadPermissionStatus) {
  auto url = FileSystemURL::CreateForTest(
      kTestStorageKey, storage::kFileSystemTypeTest,
      base::FilePath::FromUTF8Unsafe("/test"));
  TestFileSystemAccessHandle handle(manager_.get(),
                                    FileSystemAccessManagerImpl::BindingContext(
                                        kTestStorageKey.origin(), kTestURL,
                                        /*worker_process_id=*/1),
                                    url, handle_state_);

  EXPECT_CALL(*read_grant_, GetStatus())
      .WillOnce(testing::Return(PermissionStatus::ASK));
  EXPECT_EQ(PermissionStatus::ASK, handle.GetReadPermissionStatus());

  EXPECT_CALL(*read_grant_, GetStatus())
      .WillOnce(testing::Return(PermissionStatus::GRANTED));
  EXPECT_EQ(PermissionStatus::GRANTED, handle.GetReadPermissionStatus());
}

TEST_F(FileSystemAccessHandleBaseTest,
       GetWritePermissionStatus_ReadStatusNotGranted) {
  auto url = FileSystemURL::CreateForTest(
      kTestStorageKey, storage::kFileSystemTypeTest,
      base::FilePath::FromUTF8Unsafe("/test"));
  TestFileSystemAccessHandle handle(manager_.get(),
                                    FileSystemAccessManagerImpl::BindingContext(
                                        kTestStorageKey.origin(), kTestURL,
                                        /*worker_process_id=*/1),
                                    url, handle_state_);

  EXPECT_CALL(*read_grant_, GetStatus())
      .WillOnce(testing::Return(PermissionStatus::ASK));
  EXPECT_EQ(PermissionStatus::ASK, handle.GetWritePermissionStatus());

  EXPECT_CALL(*read_grant_, GetStatus())
      .WillOnce(testing::Return(PermissionStatus::DENIED));
  EXPECT_EQ(PermissionStatus::DENIED, handle.GetWritePermissionStatus());
}

TEST_F(FileSystemAccessHandleBaseTest,
       GetWritePermissionStatus_ReadStatusGranted) {
  auto url = FileSystemURL::CreateForTest(
      kTestStorageKey, storage::kFileSystemTypeTest,
      base::FilePath::FromUTF8Unsafe("/test"));
  TestFileSystemAccessHandle handle(manager_.get(),
                                    FileSystemAccessManagerImpl::BindingContext(
                                        kTestStorageKey.origin(), kTestURL,
                                        /*worker_process_id=*/1),
                                    url, handle_state_);

  EXPECT_CALL(*read_grant_, GetStatus())
      .WillOnce(testing::Return(PermissionStatus::GRANTED));
  EXPECT_CALL(*write_grant_, GetStatus())
      .WillOnce(testing::Return(PermissionStatus::ASK));
  EXPECT_EQ(PermissionStatus::ASK, handle.GetWritePermissionStatus());
}

TEST_F(FileSystemAccessHandleBaseTest, RequestWritePermission_AlreadyGranted) {
  auto url = FileSystemURL::CreateForTest(
      kTestStorageKey, storage::kFileSystemTypeTest,
      base::FilePath::FromUTF8Unsafe("/test"));
  TestFileSystemAccessHandle handle(manager_.get(),
                                    FileSystemAccessManagerImpl::BindingContext(
                                        kTestStorageKey.origin(), kTestURL,
                                        /*worker_process_id=*/1),
                                    url, handle_state_);

  EXPECT_CALL(*read_grant_, GetStatus())
      .WillOnce(testing::Return(PermissionStatus::GRANTED));
  EXPECT_CALL(*write_grant_, GetStatus())
      .WillOnce(testing::Return(PermissionStatus::GRANTED));

  base::RunLoop loop;
  handle.DoRequestPermission(
      /*writable=*/true,
      base::BindLambdaForTesting(
          [&](blink::mojom::FileSystemAccessErrorPtr error,
              PermissionStatus result) {
            EXPECT_EQ(blink::mojom::FileSystemAccessStatus::kOk, error->status);
            EXPECT_EQ(PermissionStatus::GRANTED, result);
            loop.Quit();
          }));
  loop.Run();
}

TEST_F(FileSystemAccessHandleBaseTest, RequestWritePermission) {
  const int kProcessId = 1;
  const int kFrameRoutingId = 2;
  const GlobalRenderFrameHostId kFrameId(kProcessId, kFrameRoutingId);

  auto url = FileSystemURL::CreateForTest(
      kTestStorageKey, storage::kFileSystemTypeTest,
      base::FilePath::FromUTF8Unsafe("/test"));
  TestFileSystemAccessHandle handle(
      manager_.get(),
      FileSystemAccessManagerImpl::BindingContext(kTestStorageKey.origin(),
                                                  kTestURL, kFrameId),
      url, handle_state_);

  EXPECT_CALL(*read_grant_, GetStatus())
      .WillRepeatedly(testing::Return(PermissionStatus::GRANTED));
  {
    testing::InSequence sequence;
    EXPECT_CALL(*write_grant_, GetStatus())
        .WillOnce(testing::Return(PermissionStatus::ASK));
    EXPECT_CALL(*write_grant_,
                RequestPermission_(kFrameId, UserActivationState::kRequired,
                                   testing::_))
        .WillOnce(
            RunOnceCallback<2>(FileSystemAccessPermissionGrant::
                                   PermissionRequestOutcome::kUserGranted));
    EXPECT_CALL(*write_grant_, GetStatus())
        .WillOnce(testing::Return(PermissionStatus::GRANTED));
  }

  base::RunLoop loop;
  handle.DoRequestPermission(
      /*writable=*/true,
      base::BindLambdaForTesting(
          [&](blink::mojom::FileSystemAccessErrorPtr error,
              PermissionStatus result) {
            EXPECT_EQ(blink::mojom::FileSystemAccessStatus::kOk, error->status);
            EXPECT_EQ(PermissionStatus::GRANTED, result);
            loop.Quit();
          }));
  loop.Run();
}

}  // namespace content
