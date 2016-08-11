/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 *              Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "default_display_buffer_compositor.h"

#include "mir/compositor/scene.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/buffer.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/renderer/renderer.h"
#include "occlusion.h"
#include <mutex>
#include <cstdlib>
#include <algorithm>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::DefaultDisplayBufferCompositor::DefaultDisplayBufferCompositor(
    mg::DisplayBuffer& display_buffer,
    std::shared_ptr<mir::renderer::Renderer> const& renderer,
    std::shared_ptr<mc::CompositorReport> const& report) :
    display_buffer(display_buffer),
    renderer(renderer),
    report(report)
{
}

void mc::DefaultDisplayBufferCompositor::composite(mc::SceneElementSequence&& scene_elements)
{
    report->began_frame(this);

    auto const& view_area = display_buffer.view_area();
    auto const& occlusions = mc::filter_occlusions_from(scene_elements, view_area);

    for (auto const& element : occlusions)
        element->occluded();

    mg::RenderableList renderable_list;
    renderable_list.reserve(scene_elements.size());
    for (auto const& element : scene_elements)
    {
        element->rendered();
        renderable_list.push_back(element->renderable());
    }

    /*
     * TODO: The plan is to eventually record what the main Displaybuffer
     *       is per Renderable. That is the "only" or the "main/fastest"
     *       display buffer that the Renderable is used on. In doing this
     *       clients will be able to query important information they need
     *       such as what is the best frame clock to sync their rendering to
     *       or what is the best sub-pixel RGB ordering to assume for a
     *       specific surface, based on which monitor(s) it is visible on.
     */

    /*
     * Note: Buffer lifetimes are ensured by the two objects holding
     *       references to them; scene_elements and renderable_list.
     *       So no buffer is going to be released back to the client till
     *       both of those containers get destroyed (end of the function).
     *       Actually, there's a third reference held by the texture cache
     *       in GLRenderer, but that gets released earlier in render().
     */
    scene_elements.clear();  // Those in use are still in renderable_list

    if (display_buffer.post_renderables_if_optimizable(renderable_list))
    {
        report->renderables_in_frame(this, renderable_list);
        renderer->suspend();
    }
    else
    {
        renderer->set_output_transform(display_buffer.orientation(), display_buffer.mirror_mode());
        renderer->render(renderable_list);

        report->renderables_in_frame(this, renderable_list);
        report->rendered_frame(this);

        // Release the buffers we did use back to the clients, before starting
        // on the potentially slow flip().
        // FIXME: This clear() call is blocking a little because we drive IPC here (LP: #1395421)
        renderable_list.clear();
    }

    report->finished_frame(this);
}
