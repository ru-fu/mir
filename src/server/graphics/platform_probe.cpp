/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/log.h"
#include "mir/graphics/platform.h"
#include "platform_probe.h"

#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;

namespace
{
auto probe_module(
    mir::graphics::PlatformProbe const& probe,
    mir::SharedLibrary& module,
    char const* platform_type_name,
    mir::options::ProgramOption const& options,
    std::shared_ptr<mir::ConsoleServices> const& console) -> std::vector<mg::SupportedDevice>
{
    auto describe = module.load_function<mir::graphics::DescribeModule>(
        "describe_graphics_module",
        MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

    auto desc = describe();
    mir::log_info("Found %s driver: %s (version %d.%d.%d)",
                  platform_type_name,
                  desc->name,
                  desc->major_version,
                  desc->minor_version,
                  desc->micro_version);

    auto supported_devices = probe(console, std::make_shared<mir::udev::Context>(), options);
    if (supported_devices.empty())
    {
        mir::log_info("(Unsupported by system environment)");
    }
    else
    {
        mir::log_info("Driver supports:");
        for (auto const& device : supported_devices)
        {
            auto const device_name =
                [&device]() -> std::string
                {
                    if (device.device)
                    {
                        // If the platform claims a particular device, name it.
                        return device.device->devpath();
                    }
                    else
                    {
                        // The platform claims it can support the system in general
                        return "System";
                    }
                }();
            mir::log_info("\t%s (priority %i)", device_name.c_str(), device.support_level);
        }
    }
    return supported_devices;
}
}

auto mir::graphics::probe_display_module(
    SharedLibrary& module,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console) -> std::vector<SupportedDevice>
{
    return probe_module(
        module.load_function<mir::graphics::PlatformProbe>(
            "probe_display_platform",
            MIR_SERVER_GRAPHICS_PLATFORM_VERSION),
        module,
        "display",
        options,
        console);
}

auto mir::graphics::probe_rendering_module(
    SharedLibrary& module,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console) -> std::vector<SupportedDevice>
{
    return probe_module(
        module.load_function<mir::graphics::PlatformProbe>(
            "probe_rendering_platform",
            MIR_SERVER_GRAPHICS_PLATFORM_VERSION),
        module,
        "rendering",
        options,
        console);
}

namespace
{
bool is_same_device(
    std::unique_ptr<mir::udev::Device> const& a,
    std::unique_ptr<mir::udev::Device> const& b)
{
    /* TODO: This compares for exact udev path equality. We *may* want to treat
     * some devices as equal even if they have different device paths.
     */
    if (!a || !b)
    {
        // A null device is no equal to any other device, not even another null device
        return false;
    }
    return *a == *b;
}

enum class ModuleType
{
    Rendering,
    Display
};

auto modules_for_device(
    ModuleType type,
    std::vector<std::shared_ptr<mir::SharedLibrary>> const& modules,
    mir::options::ProgramOption const& options,
    std::shared_ptr<mir::ConsoleServices> const& console)
-> std::vector<std::pair<mg::SupportedDevice, std::shared_ptr<mir::SharedLibrary>>>
{
    std::vector<std::pair<mg::SupportedDevice, std::shared_ptr<mir::SharedLibrary>>> best_modules_so_far;
    for (auto& module : modules)
    {
        try
        {
            std::vector<mg::SupportedDevice> supported_devices;
            switch (type)
            {
            case ModuleType::Rendering:
                supported_devices = mg::probe_rendering_module(*module, options, console);
                break;
            case ModuleType::Display:
                supported_devices = mg::probe_display_module(*module, options, console);
                break;
            }
            for (auto& device : supported_devices)
            {
                if (device.device)
                {
                    auto const existing_device = std::find_if(
                        best_modules_so_far.begin(),
                        best_modules_so_far.end(),
                        [&device](auto& candidate)
                        {
                            return is_same_device(device.device, candidate.first.device);
                        });
                    if (existing_device != best_modules_so_far.end())
                    {
                        // This device is also supported by some other platform; pick only the best one
                        if (existing_device->first.support_level < device.support_level)
                        {
                            *existing_device = std::make_pair(std::move(device), module);
                        }
                    }
                    else if (device.support_level > mg::PlatformPriority::unsupported)
                    {
                        // Not-seen-before device, which this platform supports in some fashion
                        best_modules_so_far.emplace_back(std::move(device), module);
                    }
                }
                else if (device.support_level > mg::PlatformPriority::unsupported)
                {
                    // Devices with null associated udev device are not combined with any others
                    best_modules_so_far.emplace_back(std::move(device), module);
                }
            }
        }
        catch (std::runtime_error const&)
        {
        }
    }
    if (best_modules_so_far.empty())
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find any platforms for current system"}));
    }
    else
    {
        // If there are any non-dummy platforms in our support list, remove all the dummy platforms.

        // First, move all the non-dummy platforms to the front…
        auto first_dummy_platform = std::partition(
            best_modules_so_far.begin(),
            best_modules_so_far.end(),
            [](auto const& module)
            {
                return module.first.support_level > mg::PlatformPriority::dummy;
            });

        // …then, if there are any platforms before the start of the dummy platforms…
        if (first_dummy_platform != best_modules_so_far.begin())
        {
            // …erase everything after the start of the dummy platforms.
            best_modules_so_far.erase(first_dummy_platform, best_modules_so_far.end());
        }
    }
    return best_modules_so_far;
}
}

auto mir::graphics::display_modules_for_device(
    std::vector<std::shared_ptr<SharedLibrary>> const& modules,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console) -> std::vector<std::pair<SupportedDevice, std::shared_ptr<SharedLibrary>>>
{
    return modules_for_device(
        ModuleType::Display,
        modules,
        options,
        console);
}

auto mir::graphics::rendering_modules_for_device(
    std::vector<std::shared_ptr<SharedLibrary>> const& modules,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console) -> std::vector<std::pair<SupportedDevice, std::shared_ptr<SharedLibrary>>>
{
    return modules_for_device(
        ModuleType::Rendering,
        modules,
        options,
        console);
}
