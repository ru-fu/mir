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

#include "src/server/frontend_xwayland/xwayland_client_manager.h"
#include "mir/test/doubles/stub_shell.h"
#include "mir/test/doubles/stub_session.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace testing;

namespace
{
struct MockShell : mtd::StubShell
{
    MOCK_METHOD4(open_session, std::shared_ptr<ms::Session>(
        pid_t, mir::Fd, std::string const&, std::shared_ptr<mf::EventSink> const&));
    MOCK_METHOD1(close_session, void(std::shared_ptr<ms::Session> const&));
};

struct XWaylandClientManagerTest : Test
{
    NiceMock<MockShell> shell;
    mtd::StubSession session_1;
    mtd::StubSession session_2;
    mtd::StubSession session_3;

    void SetUp() override
    {
        ON_CALL(shell, open_session(_, _, _, _))
            .WillByDefault(Return(mt::fake_shared(session_1)));
    }
};

}

TEST_F(XWaylandClientManagerTest, can_be_created)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};
}

TEST_F(XWaylandClientManagerTest, get_session_initially_creates_session)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};

    EXPECT_CALL(shell, open_session(1, _, _, _))
        .Times(1)
        .WillOnce(Return(mt::fake_shared(session_1)));

    auto const client_session_1 = manager.session_for_client(1);
    EXPECT_THAT(client_session_1->session().get(), Eq(&session_1));
}

TEST_F(XWaylandClientManagerTest, repeated_get_session_with_same_pid_returns_same_session)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};

    EXPECT_CALL(shell, open_session(1, _, _, _))
        .Times(1)
        .WillOnce(Return(mt::fake_shared(session_1)));

    auto const client_session_1 = manager.session_for_client(1);
    auto const client_session_2 = manager.session_for_client(1);
    auto const client_session_3 = manager.session_for_client(1);
    EXPECT_THAT(client_session_1.get(), Eq(client_session_2.get()));
    EXPECT_THAT(client_session_1.get(), Eq(client_session_3.get()));
}

TEST_F(XWaylandClientManagerTest, repeated_get_session_with_different_pids_creates_new_sessions)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};

    EXPECT_CALL(shell, open_session(_, _, _, _))
        .Times(3)
        .WillOnce(Return(mt::fake_shared(session_1)))
        .WillOnce(Return(mt::fake_shared(session_2)))
        .WillOnce(Return(mt::fake_shared(session_3)));

    auto const client_session_1 = manager.session_for_client(1);
    auto const client_session_2 = manager.session_for_client(2);
    auto const client_session_3 = manager.session_for_client(3);

    EXPECT_THAT(client_session_1->session().get(), Eq(&session_1));
    EXPECT_THAT(client_session_2->session().get(), Eq(&session_2));
    EXPECT_THAT(client_session_3->session().get(), Eq(&session_3));
}

TEST_F(XWaylandClientManagerTest, resetting_client_session_closes_session)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};

    EXPECT_CALL(shell, open_session(_, _, _, _))
        .Times(1)
        .WillOnce(Return(mt::fake_shared(session_1)));
    EXPECT_CALL(shell, close_session(_))
        .Times(0);
    auto client_session_1 = manager.session_for_client(1);
    Mock::VerifyAndClearExpectations(&shell);

    EXPECT_CALL(shell, close_session(Eq(mt::fake_shared(session_1))))
        .Times(1);
    client_session_1.reset();
    Mock::VerifyAndClearExpectations(&shell);
}

TEST_F(XWaylandClientManagerTest, reset_does_not_close_session_if_multiple_owners)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};

    EXPECT_CALL(shell, open_session(_, _, _, _))
        .Times(1)
        .WillOnce(Return(mt::fake_shared(session_1)));
    EXPECT_CALL(shell, close_session(Eq(mt::fake_shared(session_1))))
        .Times(0);

    auto client_session_1 = manager.session_for_client(1);
    auto client_session_2 = manager.session_for_client(1);
    client_session_1.reset();

    Mock::VerifyAndClearExpectations(&shell);
}

TEST_F(XWaylandClientManagerTest, session_closed_when_all_client_sessions_reset)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};

    EXPECT_CALL(shell, open_session(_, _, _, _))
        .Times(1)
        .WillOnce(Return(mt::fake_shared(session_1)));
    EXPECT_CALL(shell, close_session(Eq(mt::fake_shared(session_1))))
        .Times(1);

    auto client_session_1 = manager.session_for_client(1);
    auto client_session_2 = manager.session_for_client(1);
    auto client_session_3 = manager.session_for_client(1);

    client_session_1.reset();
    client_session_2.reset();
    client_session_3.reset();
}

TEST_F(XWaylandClientManagerTest, new_session_created_after_old_session_for_same_pid_closed)
{
    mf::XWaylandClientManager manager{mt::fake_shared(shell)};
    auto client_session_1 = manager.session_for_client(1);
    client_session_1.reset();

    EXPECT_CALL(shell, open_session(_, _, _, _))
        .Times(1)
        .WillRepeatedly(Return(mt::fake_shared(session_2)));
    auto const client_session_2 = manager.session_for_client(1);
    EXPECT_THAT(client_session_2->session().get(), Eq(&session_2));
}
