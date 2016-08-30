/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include <cstdint>
#include <vector>

namespace mir
{
namespace graphics
{

class DisplayGamma
{
public:
    DisplayGamma() = default;

    DisplayGamma(std::vector<uint16_t> const& red,
                 std::vector<uint16_t> const& green,
                 std::vector<uint16_t> const& blue);

    DisplayGamma(std::vector<uint16_t>&& red,
                 std::vector<uint16_t>&& green,
                 std::vector<uint16_t>&& blue);

    DisplayGamma(DisplayGamma const& gamma) = default;
    DisplayGamma(DisplayGamma&& gamma) = default;

    DisplayGamma& operator=(DisplayGamma const& gamma) = default;
    DisplayGamma& operator=(DisplayGamma&& gamma) = default;

    uint16_t const* red() const;
    uint16_t const* green() const;
    uint16_t const* blue() const;
    uint32_t size() const;

private:
    void throw_if_lut_size_mismatch() const;

    std::vector<uint16_t> red_;
    std::vector<uint16_t> green_;
    std::vector<uint16_t> blue_;
};

}
}
