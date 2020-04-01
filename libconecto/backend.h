/* backend.h
 *
 * Copyright 2020 Hannes Schulze <haschu0103@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "discovery.h"
#include "device.h"
#include "config-file.h"
#include <map>
#include <string>
#include <giomm/tlscertificate.h>

namespace Conecto {

class Backend {
  public:
    /**
     * @throw (on first call) GnuTLSInitializationError, PEMWriteError, InvalidCertificateException
     */
    static Backend& get_instance ();
    ~Backend () {}

    /**
     * Start listening for new devices
     *
     * @throw BindSocketException
     */
    void listen ();

    /**
     * A register a new plugin (capability/packet handler). If the capability already exists, it will be overridden
     *
     * @param capability The capability's name
     * @param handler The packet handler for this capability
     */
    void register_plugin (const std::string&                            capability,
                          const std::shared_ptr<AbstractPacketHandler>& handler) noexcept;
    /**
     * Get the packet handler for a capability (may be empty)
     *
     * @param capability The capability name (see @p register_plugin)
     * @return The handler interface (or nullptr)
     */
    std::shared_ptr<AbstractPacketHandler> get_plugin (const std::string& capability) const noexcept;

    ConfigFile&       get_config () noexcept;
    const ConfigFile& get_config () const noexcept;

    Glib::RefPtr<Gio::TlsCertificate> get_certificate () const noexcept;
    std::list<std::string>            get_handler_interfaces () const noexcept;

    static std::string get_storage_dir () noexcept;
    static std::string get_config_dir () noexcept;
    static std::string get_cache_dir () noexcept;
    static void        init_user_dirs ();

    /**
     * Allow given device
     *
     * @param device The device
     * @throw DeviceNotFoundException
     */
    void allow_device (const std::shared_ptr<Device>& device);
    /**
     * Disallow given device
     *
     * @param device The device
     * @throw DeviceNotFoundException
     */
    void disallow_device (const std::shared_ptr<Device>& device);

    using type_signal_found_new_device = sigc::signal<void, const Device& /* device */>;
    using type_signal_device_capability_added =
            sigc::signal<void, const std::shared_ptr<Device>& /* device */, const std::string& /* capability */,
                         const std::shared_ptr<AbstractPacketHandler>& /* handler */>;
    type_signal_found_new_device        signal_found_new_device () { return m_signal_found_new_device; }
    type_signal_device_capability_added signal_device_capability_added () { return m_signal_device_capability_added; }

    Backend (const Backend&) = delete;
    Backend& operator= (const Backend&) = delete;

  protected:
    Backend ();

  private:
    struct DeviceEntry {
        DeviceEntry (const std::shared_ptr<Device>& dev)
            : device (dev)
        {
        }
        DeviceEntry (const DeviceEntry&) = delete;
        DeviceEntry& operator= (const DeviceEntry&) = delete;

        sigc::connection        paired_conn;
        sigc::connection        disconnected_conn;
        std::shared_ptr<Device> device;
    };

    void        on_new_device (std::shared_ptr<Device> device);
    void        on_capability_added (const std::string& cap, const std::shared_ptr<Device>& device);
    void        on_capability_removed (const std::string& cap, const std::shared_ptr<Device>& device);
    bool        get_allowed_in_config (const Device& device) const;
    void        activate_device (const std::shared_ptr<DeviceEntry>& entry);
    std::string get_cache_file () const; // Path to devices cache file
    void        load_from_cache () noexcept;
    void        update_cache () noexcept;

    std::map<std::string, std::shared_ptr<DeviceEntry>> m_devices;

    Discovery                         m_discovery;
    std::unique_ptr<ConfigFile>       m_config;
    Glib::RefPtr<Gio::TlsCertificate> m_certificate;

    std::map<std::string, std::shared_ptr<AbstractPacketHandler>> m_plugins;

    type_signal_found_new_device        m_signal_found_new_device;
    type_signal_device_capability_added m_signal_device_capability_added;
};

} // namespace Conecto