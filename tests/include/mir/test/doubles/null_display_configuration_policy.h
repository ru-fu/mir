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

#ifndef MIR_TEST_DOUBLES_NULL_DISPLAY_CONFIGURATION_POLICY_H_
#define MIR_TEST_DOUBLES_NULL_DISPLAY_CONFIGURATION_POLICY_H_

#include "mir/graphics/display_configuration_policy.h"

namespace mir
{
namespace test
{
namespace doubles
{
class NullDisplayConfigurationPolicy : public graphics::DisplayConfigurationPolicy
{
public:
    void apply_to(graphics::DisplayConfiguration&) override {}
};
}
}
}

#endif //MIR_TEST_DOUBLES_NULL_DISPLAY_CONFIGURATION_POLICY_H_
