/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/headless_test.h"
#include "mir_test_framework/stub_server_platform_factory.h"

#include "mir/shared_library.h"
#include "mir/geometry/rectangle.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/scene_element.h"
#include "mir/compositor/occlusion.h"
#include "mir/graphics/display_buffer.h"
#include "mir/renderer/gl/render_target.h"
#include "mir_test_framework/executable_path.h"

#include <boost/throw_exception.hpp>

namespace geom = mir::geometry;
namespace mtf = mir_test_framework;

mtf::HeadlessTest::HeadlessTest()
{
    add_to_environment("MIR_SERVER_PLATFORM_GRAPHICS_LIB", mtf::server_platform("graphics-dummy.so").c_str());
    add_to_environment("MIR_SERVER_PLATFORM_INPUT_LIB", mtf::server_platform("input-stub.so").c_str());
    add_to_environment("MIR_SERVER_ENABLE_KEY_REPEAT", "false");

    struct HeadlessCompositorFactory : mir::compositor::DisplayBufferCompositorFactory
    {
        std::unique_ptr<mir::compositor::DisplayBufferCompositor> create_compositor_for(
            mir::graphics::DisplayBuffer& display_buffer) override
        {
            struct HeadlessDBC : mir::compositor::DisplayBufferCompositor
            {
                HeadlessDBC(mir::graphics::DisplayBuffer& db) :
                    db(db),
                    render_target(dynamic_cast<mir::renderer::gl::RenderTarget*>(
                        db.native_display_buffer()))
                {
                }

                void composite(mir::compositor::SceneElementSequence&& seq) override
                {
                    auto occluded = mir::compositor::filter_occlusions_from(seq, db.view_area());
                    for(auto const& element : occluded)
                        element->occluded();
                    mir::graphics::RenderableList renderlist;
                    for(auto const& r : seq)
                    {
                        r->rendered();
                        renderlist.push_back(r->renderable());
                    }

                    if (db.post_renderables_if_optimizable(renderlist))
                        return;

                    // Invoke GL renderer specific functions if the DisplayBuffer supports them
                    if (render_target)
                        render_target->make_current();

                    // We need to consume a buffer to unblock client tests
                    for (auto const& renderable : renderlist)
                        renderable->buffer(); 

                    if (render_target)
                        render_target->swap_buffers();
                }
                mir::graphics::DisplayBuffer& db;
                mir::renderer::gl::RenderTarget* const render_target;
            };
            return std::make_unique<HeadlessDBC>(display_buffer);
        }
    };

    server.override_the_display_buffer_compositor_factory([]
    {
        return std::make_shared<HeadlessCompositorFactory>();
    });
}

mtf::HeadlessTest::~HeadlessTest() noexcept = default;

void mtf::HeadlessTest::preset_display(std::shared_ptr<mir::graphics::Display> const& display)
{
    mtf::set_next_preset_display(display);
}

void mtf::HeadlessTest::initial_display_layout(std::vector<geom::Rectangle> const& display_rects)
{
    mtf::set_next_display_rects(std::unique_ptr<std::vector<geom::Rectangle>>(new std::vector<geom::Rectangle>(display_rects)));
}
