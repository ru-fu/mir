/*
 * Copyright © 2021 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "mir/scene/surface_state_stack.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace ms = mir::scene;

namespace
{
auto precedence(MirWindowState state) -> int
{
    switch (state)
    {
    case mir_window_state_restored: return 1;
    case mir_window_state_horizmaximized: // fallthrough
    case mir_window_state_vertmaximized: return 2;
    case mir_window_state_maximized: return 3;
    case mir_window_state_attached: return 4;
    case mir_window_state_fullscreen: return 5;
    case mir_window_state_minimized: return 6;
    case mir_window_state_hidden: return 7;
    default: throw "Invalid state " + std::to_string(state);
    }
}
}

struct SurfaceStateStackTestSingle : TestWithParam<MirWindowState>
{
};

struct SurfaceStateStackTestDouble : TestWithParam<std::tuple<MirWindowState, MirWindowState>>
{
};

TEST(SurfaceStateStackTest, by_default_all_states_false_except_restored)
{
    ms::SurfaceStateStack stack{mir_window_state_restored};
    EXPECT_THAT(stack.has(mir_window_state_hidden), Eq(false));
    EXPECT_THAT(stack.has(mir_window_state_minimized), Eq(false));
    EXPECT_THAT(stack.has(mir_window_state_fullscreen), Eq(false));
    EXPECT_THAT(stack.has(mir_window_state_attached), Eq(false));
    EXPECT_THAT(stack.has(mir_window_state_horizmaximized), Eq(false));
    EXPECT_THAT(stack.has(mir_window_state_vertmaximized), Eq(false));
    EXPECT_THAT(stack.has(mir_window_state_maximized), Eq(false));
    EXPECT_THAT(stack.has(mir_window_state_restored), Eq(true));
}

TEST(SurfaceStateStackTest, with_and_without_return_this)
{
    ms::SurfaceStateStack stack{mir_window_state_fullscreen};
    stack.with(mir_window_state_minimized).without(mir_window_state_minimized).with(mir_window_state_attached);
    EXPECT_THAT(stack.active_state(), Eq(mir_window_state_fullscreen));
    EXPECT_THAT(stack.has(mir_window_state_fullscreen), Eq(true));
    EXPECT_THAT(stack.has(mir_window_state_minimized), Eq(false));
    EXPECT_THAT(stack.has(mir_window_state_attached), Eq(true));
}

TEST(SurfaceStateStackTest, with_maximized_sets_both_horiz_and_virt)
{
    ms::SurfaceStateStack stack{mir_window_state_restored};
    stack.with(mir_window_state_maximized);
    EXPECT_THAT(stack.has(mir_window_state_horizmaximized), Eq(true));
    EXPECT_THAT(stack.has(mir_window_state_vertmaximized), Eq(true));
}

TEST_P(SurfaceStateStackTestSingle, can_create_with_state)
{
    MirWindowState const state = GetParam();
    ms::SurfaceStateStack stack{state};
    EXPECT_THAT(stack.active_state(), Eq(state));
}

TEST_P(SurfaceStateStackTestSingle, always_has_state_created_with)
{
    MirWindowState const state = GetParam();
    ms::SurfaceStateStack stack{state};
    EXPECT_THAT(stack.has(state), Eq(true));
}

TEST_P(SurfaceStateStackTestDouble, can_set_active_state)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateStack stack{a};
    stack.set_active_state(b);
    EXPECT_THAT(stack.active_state(), Eq(b));
}

TEST_P(SurfaceStateStackTestDouble, can_set_active_state_multiple_times)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateStack stack{a};
    stack.set_active_state(b);
    stack.set_active_state(a);
    EXPECT_THAT(stack.active_state(), Eq(a));
}

TEST_P(SurfaceStateStackTestDouble, has_state_once_added)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateStack stack{a};
    if (b != mir_window_state_restored)
    {
        stack.with(b);
        EXPECT_THAT(stack.has(b), Eq(true));
    }
}

TEST_P(SurfaceStateStackTestDouble, does_not_have_state_once_removed)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateStack stack{a};
    if (b != mir_window_state_restored)
    {
        stack.without(b);
        EXPECT_THAT(stack.has(b), Eq(false));
    }
}

TEST_P(SurfaceStateStackTestDouble, can_add_and_remove_same_state)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateStack stack{a};
    if (b != mir_window_state_restored)
    {
        stack.with(b);
        stack.without(b);
    }
    if (a == b)
    {
        EXPECT_THAT(stack.active_state(), Eq(mir_window_state_restored));
    }
    else if (a == mir_window_state_maximized && b == mir_window_state_horizmaximized)
    {
        EXPECT_THAT(stack.active_state(), Eq(mir_window_state_vertmaximized));
    }
    else if (a == mir_window_state_maximized && b == mir_window_state_vertmaximized)
    {
        EXPECT_THAT(stack.active_state(), Eq(mir_window_state_horizmaximized));
    }
    else if ((a == mir_window_state_horizmaximized || a == mir_window_state_vertmaximized) &&
        b == mir_window_state_maximized)
    {
        EXPECT_THAT(stack.active_state(), Eq(mir_window_state_restored));
    }
    else
    {
        EXPECT_THAT(stack.active_state(), Eq(a));
    }
}

TEST_P(SurfaceStateStackTestDouble, state_with_highest_precedence_is_active_state)
{
    MirWindowState a, b;
    std::tie(a, b) = GetParam();
    ms::SurfaceStateStack stack{a};
    if (b != mir_window_state_restored)
    {
        stack.with(b);
    }
    if (a != b && precedence(a) == 2 && precedence(b) == 2)
    {
        // they are vert and horiz maximized
        EXPECT_THAT(stack.active_state(), Eq(mir_window_state_maximized));
    }
    else if (precedence(a) > precedence(b))
    {
        EXPECT_THAT(stack.active_state(), Eq(a));
    }
    else
    {
        EXPECT_THAT(stack.active_state(), Eq(b));
    }
}

auto const all_states = Values(
    mir_window_state_restored,
    mir_window_state_horizmaximized,
    mir_window_state_vertmaximized,
    mir_window_state_maximized,
    mir_window_state_attached,
    mir_window_state_fullscreen,
    mir_window_state_minimized,
    mir_window_state_hidden);

INSTANTIATE_TEST_SUITE_P(
    SurfaceStateStackTestSingle,
    SurfaceStateStackTestSingle,
    all_states);

INSTANTIATE_TEST_SUITE_P(
    SurfaceStateStackTestDouble,
    SurfaceStateStackTestDouble,
    Combine(all_states, all_states));
