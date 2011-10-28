// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cloud_print/virtual_driver/win/port_monitor/port_monitor.h"
#include <winspool.h>
#include "base/file_util.h"
#include "base/path_service.h"
#include "base/string16.h"
#include "base/win/registry.h"
#include "cloud_print/virtual_driver/win/port_monitor/spooler_win.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cloud_print {
const wchar_t kChromeExePath[] = L"google\\chrome\\application\\chrometest.exe";
const wchar_t kAlternateChromeExePath[] =
    L"google\\chrome\\application\\chrometestalternate.exe";
const wchar_t kChromePathRegValue[] =L"PathToChromeTestExe";

class PortMonitorTest : public testing::Test  {
 public:
  PortMonitorTest() {}
 protected:
  // Creates a registry entry pointing at a chrome
  virtual void SetUpChromeExeRegistry() {
    // Create a temporary chrome.exe location value.
    base::win::RegKey key(HKEY_CURRENT_USER,
                          cloud_print::kChromePathRegKey,
                          KEY_ALL_ACCESS);

    FilePath path;
    PathService::Get(base::DIR_LOCAL_APP_DATA, &path);
    path = path.Append(kAlternateChromeExePath);
    ASSERT_EQ(ERROR_SUCCESS,
              key.WriteValue(cloud_print::kChromePathRegValue,
                             path.value().c_str()));
  }
  // Deletes the registry entry created in SetUpChromeExeRegistry
  virtual void DeleteChromeExeRegistry() {
    base::win::RegKey key(HKEY_CURRENT_USER,
                          cloud_print::kChromePathRegKey,
                          KEY_ALL_ACCESS);
    key.DeleteValue(cloud_print::kChromePathRegValue);
  }

  virtual void CreateTempChromeExeFiles() {
    FilePath path;
    PathService::Get(base::DIR_LOCAL_APP_DATA, &path);
    FilePath main_path = path.Append(kChromeExePath);
    ASSERT_TRUE(file_util::CreateDirectory(main_path));
    FilePath alternate_path = path.Append(kAlternateChromeExePath);
    ASSERT_TRUE(file_util::CreateDirectory(alternate_path));
  }

  virtual void DeleteTempChromeExeFiles() {
    FilePath path;
    PathService::Get(base::DIR_LOCAL_APP_DATA, &path);
    FilePath main_path = path.Append(kChromeExePath);
    ASSERT_TRUE(file_util::Delete(main_path, true));
    PathService::Get(base::DIR_LOCAL_APP_DATA, &path);
    FilePath alternate_path = path.Append(kAlternateChromeExePath);
    ASSERT_TRUE(file_util::Delete(alternate_path, true));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PortMonitorTest);
};

TEST_F(PortMonitorTest, GetChromeExePathTest) {
  FilePath chrome_path;
  SetUpChromeExeRegistry();
  CreateTempChromeExeFiles();
  EXPECT_TRUE(cloud_print::GetChromeExePath(&chrome_path));
  EXPECT_TRUE(
      chrome_path.value().rfind(kAlternateChromeExePath) != std::string::npos);
  DeleteChromeExeRegistry();
  chrome_path.clear();
  EXPECT_TRUE(cloud_print::GetChromeExePath(&chrome_path));
  EXPECT_TRUE(chrome_path.value().rfind(kChromeExePath) != std::string::npos);
  EXPECT_TRUE(file_util::PathExists(FilePath(chrome_path)));
  DeleteTempChromeExeFiles();
  EXPECT_FALSE(cloud_print::GetChromeExePath(&chrome_path));
}

TEST_F(PortMonitorTest, EnumPortsTest) {
  DWORD needed_bytes = 0;
  DWORD returned = 0;
  EXPECT_FALSE(Monitor2EnumPorts(NULL,
                                 NULL,
                                 1,
                                 NULL,
                                 0,
                                 &needed_bytes,
                                 &returned));
  EXPECT_EQ(ERROR_INSUFFICIENT_BUFFER, GetLastError());
  EXPECT_NE(0u, needed_bytes);
  EXPECT_EQ(0u, returned);

  BYTE* buffer = new BYTE[needed_bytes];
  ASSERT_TRUE(buffer != NULL);
  EXPECT_TRUE(Monitor2EnumPorts(NULL,
                                NULL,
                                1,
                                buffer,
                                needed_bytes,
                                &needed_bytes,
                                &returned));
  EXPECT_NE(0u, needed_bytes);
  EXPECT_EQ(1u, returned);
  PORT_INFO_1* port_info_1 = reinterpret_cast<PORT_INFO_1*>(buffer);
  EXPECT_TRUE(port_info_1->pName != NULL);
  delete[] buffer;

  returned = 0;
  needed_bytes = 0;
  EXPECT_FALSE(Monitor2EnumPorts(NULL,
                                 NULL,
                                 2,
                                 NULL,
                                 0,
                                 &needed_bytes,
                                 &returned));
  EXPECT_EQ(ERROR_INSUFFICIENT_BUFFER, GetLastError());
  EXPECT_NE(0u, needed_bytes);
  EXPECT_EQ(0u, returned);

  buffer = new BYTE[needed_bytes];
  ASSERT_TRUE(buffer != NULL);
  EXPECT_TRUE(Monitor2EnumPorts(NULL,
                                NULL,
                                2,
                                buffer,
                                needed_bytes,
                                &needed_bytes,
                                &returned));
  EXPECT_NE(0u, needed_bytes);
  EXPECT_EQ(1u, returned);
  PORT_INFO_2* port_info_2 = reinterpret_cast<PORT_INFO_2*>(buffer);
  EXPECT_TRUE(port_info_2->pPortName != NULL);
  delete[] buffer;
}

TEST_F(PortMonitorTest, FlowTest) {
  const wchar_t kXcvDataItem[] = L"MonitorUI";
  MONITORINIT monitor_init = {0};
  HANDLE monitor_handle = NULL;
  HANDLE port_handle = NULL;
  HANDLE xcv_handle = NULL;
  DWORD bytes_processed = 0;
  DWORD bytes_needed = 0;
  const size_t kBufferSize = 100;
  BYTE buffer[kBufferSize] = {0};

  // Initialize the print monitor
  MONITOR2* monitor2 = InitializePrintMonitor2(&monitor_init, &monitor_handle);
  EXPECT_TRUE(monitor2 != NULL);
  EXPECT_TRUE(monitor_handle != NULL);

  // Test the XCV functions.  Used for reporting the location of the
  // UI portion of the port monitor.
  EXPECT_TRUE(monitor2->pfnXcvOpenPort != NULL);
  EXPECT_TRUE(monitor2->pfnXcvOpenPort(monitor_handle, NULL, 0, &xcv_handle));
  EXPECT_TRUE(xcv_handle != NULL);
  EXPECT_TRUE(monitor2->pfnXcvDataPort != NULL);
  EXPECT_EQ(ERROR_ACCESS_DENIED,
            monitor2->pfnXcvDataPort(xcv_handle,
                                     kXcvDataItem,
                                     NULL,
                                     0,
                                     buffer,
                                     kBufferSize,
                                     &bytes_needed));
  EXPECT_TRUE(monitor2->pfnXcvClosePort != NULL);
  EXPECT_TRUE(monitor2->pfnXcvClosePort(xcv_handle));
  EXPECT_TRUE(monitor2->pfnXcvOpenPort(monitor_handle,
                                       NULL,
                                       SERVER_ACCESS_ADMINISTER,
                                       &xcv_handle));
  EXPECT_TRUE(xcv_handle != NULL);
  EXPECT_TRUE(monitor2->pfnXcvDataPort != NULL);
  EXPECT_EQ(ERROR_SUCCESS,
            monitor2->pfnXcvDataPort(xcv_handle,
                                     kXcvDataItem,
                                     NULL,
                                     0,
                                     buffer,
                                     kBufferSize,
                                     &bytes_needed));
  EXPECT_TRUE(monitor2->pfnXcvClosePort != NULL);
  EXPECT_TRUE(monitor2->pfnXcvClosePort(xcv_handle));

  // Test opening the port and running a print job.
  EXPECT_TRUE(monitor2->pfnOpenPort != NULL);
  EXPECT_TRUE(monitor2->pfnOpenPort(monitor_handle, NULL, &port_handle));
  EXPECT_TRUE(port_handle != NULL);
  EXPECT_TRUE(monitor2->pfnStartDocPort != NULL);
  EXPECT_TRUE(monitor2->pfnStartDocPort(port_handle, L"", 0, 0, NULL));
  EXPECT_TRUE(monitor2->pfnWritePort != NULL);
  EXPECT_TRUE(monitor2->pfnWritePort(port_handle,
                                     buffer,
                                     kBufferSize,
                                     &bytes_processed));
  EXPECT_EQ(kBufferSize, bytes_processed);
  EXPECT_TRUE(monitor2->pfnReadPort != NULL);
  EXPECT_FALSE(monitor2->pfnReadPort(port_handle,
                                     buffer,
                                     sizeof(buffer),
                                     &bytes_processed));
  EXPECT_EQ(0u, bytes_processed);
  EXPECT_TRUE(monitor2->pfnEndDocPort != NULL);
  EXPECT_TRUE(monitor2->pfnEndDocPort(port_handle));
  EXPECT_TRUE(monitor2->pfnClosePort != NULL);
  EXPECT_TRUE(monitor2->pfnClosePort(port_handle));

  // Shutdown the port monitor.
  Monitor2Shutdown(monitor_handle);
}

}  // namespace cloud_print

