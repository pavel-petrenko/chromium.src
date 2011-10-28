// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dbus/dbus_thread_manager.h"

#include "base/command_line.h"
#include "base/threading/thread.h"
#include "chrome/browser/chromeos/dbus/bluetooth_adapter_client.h"
#include "chrome/browser/chromeos/dbus/bluetooth_manager_client.h"
#include "chrome/browser/chromeos/dbus/cros_dbus_service.h"
#include "chrome/browser/chromeos/dbus/power_manager_client.h"
#include "chrome/browser/chromeos/dbus/sensors_client.h"
#include "chrome/browser/chromeos/dbus/session_manager_client.h"
#include "chrome/browser/chromeos/dbus/speech_synthesizer_client.h"
#include "chrome/common/chrome_switches.h"
#include "dbus/bus.h"

namespace chromeos {

static DBusThreadManager* g_dbus_thread_manager = NULL;

DBusThreadManager::DBusThreadManager() {
  // Create the D-Bus thread.
  base::Thread::Options thread_options;
  thread_options.message_loop_type = MessageLoop::TYPE_IO;
  dbus_thread_.reset(new base::Thread("D-Bus thread"));
  dbus_thread_->StartWithOptions(thread_options);

  // Create the connection to the system bus.
  dbus::Bus::Options system_bus_options;
  system_bus_options.bus_type = dbus::Bus::SYSTEM;
  system_bus_options.connection_type = dbus::Bus::PRIVATE;
  system_bus_options.dbus_thread_message_loop_proxy =
      dbus_thread_->message_loop_proxy();
  system_bus_ = new dbus::Bus(system_bus_options);

  // Create and start the cros D-Bus service.
  cros_dbus_service_.reset(CrosDBusService::Create(system_bus_.get()));
  cros_dbus_service_->Start();

  // Start monitoring sensors if needed.
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kEnableSensors)) {
    sensors_client_.reset(SensorsClient::Create(system_bus_.get()));
  }

  // Create bluetooth clients if bluetooth is enabled.
  if (command_line.HasSwitch(switches::kEnableBluetooth)) {
    bluetooth_manager_client_.reset(BluetoothManagerClient::Create(
        system_bus_.get()));
    bluetooth_adapter_client_.reset(BluetoothAdapterClient::Create(
        system_bus_.get()));
  }

  // Create the power manager client.
  power_manager_client_.reset(PowerManagerClient::Create(system_bus_.get()));
  // Create the session manager client.
  session_manager_client_.reset(
      SessionManagerClient::Create(system_bus_.get()));
  // Create the speech synthesizer client.
  speech_synthesizer_client_.reset(
      SpeechSynthesizerClient::Create(system_bus_.get()));
}

DBusThreadManager::~DBusThreadManager() {
  // Shut down the bus. During the browser shutdown, it's ok to shut down
  // the bus synchronously.
  system_bus_->ShutdownOnDBusThreadAndBlock();

  // Stop the D-Bus thread.
  dbus_thread_->Stop();
}

void DBusThreadManager::Initialize() {
  if (g_dbus_thread_manager) {
    // This can happen in tests.
    LOG(WARNING) << "DBusThreadManager::Initialize() was already called";
    return;
  }
  g_dbus_thread_manager = new DBusThreadManager;
  VLOG(1) << "DBusThreadManager initialized";
}

void DBusThreadManager::Shutdown() {
  if (!g_dbus_thread_manager) {
    // This can happen in tests.
    LOG(WARNING) << "DBusThreadManager::Shutdown() called with NULL manager";
    return;
  }
  delete g_dbus_thread_manager;
  g_dbus_thread_manager = NULL;
  VLOG(1) << "DBusThreadManager Shutdown completed";
}

DBusThreadManager* DBusThreadManager::Get() {
  CHECK(g_dbus_thread_manager)
      << "DBusThreadManager::Get() called before Initialize()";
  return g_dbus_thread_manager;
}

void DBusThreadManager::set_session_manager_client_for_testing(
    SessionManagerClient* session_manager_client) {
  session_manager_client_.reset(session_manager_client);
}

}  // namespace chromeos
