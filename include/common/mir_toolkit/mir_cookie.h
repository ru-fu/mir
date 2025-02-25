/*
* Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TOOLKIT_MIR_COOKIE_H_
#define MIR_TOOLKIT_MIR_COOKIE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Queries the size needed to serialize a given cookie
 *
 * \param [in] cookie A cookie instance
 * \return            The size of the serialized representation of the given cookie
 */
size_t mir_cookie_buffer_size(MirCookie const* cookie);

/**
 * Serializes a cookie into the given buffer
 *
 * \pre The size must be equal to mir_cookie_size
 * \param [in] cookie A cookie instance
 * \param [in] buffer A buffer which is filled with the serialized representation
                      of the given cookie
 * \param [in] size   The size of the given buffer
 */
void mir_cookie_to_buffer(MirCookie const* cookie, void* buffer, size_t size);

/**
 * Create a cookie from a serialized representation
 *
 * \param [in] buffer The buffer containing a serialized cookie.
 * \param [in] size   The size of the buffer.
 *                    The buffer may be freed immediately after this call.
 * \return            A MirCookie instance. The instance must be released
 *                    with a call to mir_cookie_release.
 *                    NULL will be returned if the buffer and size don't describe
 *                    the contents of a MirCookie.
 */
MirCookie const* mir_cookie_from_buffer(void const* buffer, size_t size);

/**
 * Release the MirCookie
 *
 * \param [in] cookie The cookie to release
 */
void mir_cookie_release(MirCookie const* cookie);

#ifdef __cplusplus
}
#endif

#endif // MIR_TOOLKIT_MIR_COOKIE_H_
