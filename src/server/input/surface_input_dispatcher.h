/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_INPUT_DEFAULT_INPUT_DISPATCHER_H_
#define MIR_INPUT_DEFAULT_INPUT_DISPATCHER_H_

#include "mir/input/input_dispatcher.h"
#include "mir/input/keyboard_observer.h"
#include "mir/shell/input_targeter.h"
#include "mir/geometry/point.h"
#include "mir/observer_registrar.h"
#include "mir/observer_multiplexer.h"
#include "mir/executor.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace mir
{
namespace scene
{
class Observer;
class Surface;
}
namespace input
{
class Surface;
class Scene;

class SurfaceInputDispatcher :
    public input::InputDispatcher,
    public shell::InputTargeter,
    public ObserverRegistrar<KeyboardObserver>
{
public:
    SurfaceInputDispatcher(std::shared_ptr<input::Scene> const& scene);
    ~SurfaceInputDispatcher();

    // mir::input::InputDispatcher
    bool dispatch(std::shared_ptr<MirEvent const> const& event) override;
    void start() override;
    void stop() override;

    // InputTargeter
    void set_focus(std::shared_ptr<input::Surface> const& target) override;
    void clear_focus() override;

    void set_drag_and_drop_handle(std::vector<uint8_t> const& handle) override;
    void clear_drag_and_drop_handle() override;

    // ObserverRegistrar
    void register_interest(std::weak_ptr<KeyboardObserver> const& observer) override;
    void register_interest(
        std::weak_ptr<KeyboardObserver> const& observer,
        Executor& executor) override;
    void unregister_interest(KeyboardObserver const& observer) override;

private:
    void device_reset(MirInputDeviceId reset_device_id, std::chrono::nanoseconds when);
    bool dispatch_key(std::shared_ptr<MirEvent const> const& ev);
    bool dispatch_pointer(MirInputDeviceId id, std::shared_ptr<MirEvent const> const& ev);
    bool dispatch_touch(MirInputDeviceId id, MirEvent const* tev);

    void send_enter_exit_event(std::shared_ptr<input::Surface> const& surface,
        MirPointerEvent const* triggering_ev, MirPointerAction action);

    void set_focus_locked(std::lock_guard<std::mutex> const&, std::shared_ptr<input::Surface> const&);

    void surface_removed(std::shared_ptr<scene::Surface> surface);

    void surface_moved(scene::Surface const* moved_surface);
    void surface_resized();

    // Look in to homognizing index on KeyInputState and PointerInputState (wrt to device id)
    struct PointerInputState
    {
        std::shared_ptr<input::Surface> current_target;
        std::shared_ptr<input::Surface> gesture_owner;
    };
    std::unordered_map<MirInputDeviceId, PointerInputState> pointer_state_by_id;
    PointerInputState& ensure_pointer_state(MirInputDeviceId id);

    struct TouchInputState
    {
        std::shared_ptr<input::Surface> gesture_owner;
    };
    std::unordered_map<MirInputDeviceId, TouchInputState> touch_state_by_id;
    TouchInputState& ensure_touch_state(MirInputDeviceId id);
    
    struct KeyboardEventMultiplexer : ObserverMultiplexer<KeyboardObserver>
    {
        KeyboardEventMultiplexer()
            : ObserverMultiplexer{linearising_executor}
        {
        }

        void keyboard_event(std::shared_ptr<MirEvent const> const& event) override
        {
            for_each_observer(&KeyboardObserver::keyboard_event, event);
        }

        void keyboard_focus_set(std::shared_ptr<Surface> const& surface) override
        {
            for_each_observer(&KeyboardObserver::keyboard_focus_set, surface);
        }
    } keyboard_multiplexer;

    std::shared_ptr<input::Scene> const scene;

    std::shared_ptr<scene::Observer> scene_observer;

    std::mutex dispatcher_mutex;
    std::shared_ptr<MirEvent const> last_pointer_event;
    std::weak_ptr<input::Surface> focus_surface;
    std::vector<uint8_t> drag_and_drop_handle;
    bool started;
};

}
}

#endif // MIR_INPUT_DEFAULT_INPUT_DISPATCHER_H_
