/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */
#ifndef MIR_ANDROID_INPUT_READER_POLICY_H_
#define MIR_ANDROID_INPUT_READER_POLICY_H_

#include "dummy_input_reader_policy.h"

#include <memory>

namespace mir
{
namespace input
{
class CursorListener;
namespace android
{

class InputReaderPolicy : public DummyInputReaderPolicy
{
public:
    explicit InputReaderPolicy(std::shared_ptr<CursorListener> const& cursor_listener);
    virtual ~InputReaderPolicy() = default;

    virtual droidinput::sp<droidinput::PointerControllerInterface> obtainPointerController(int32_t device_id);
private:
    droidinput::sp<droidinput::PointerControllerInterface> pointer_controller;
};

}
}
} // namespace mir

#endif // MIR_ANDROID_INPUT_READER_POLICY_H_
