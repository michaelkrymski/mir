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

#ifndef MIR_RENDERER_COMMON_GL_HANDLES_H_
#define MIR_RENDERER_COMMON_GL_HANDLES_H_

#include <GLES2/gl2.h>
#include <type_traits>
#include <utility>

namespace mir::renderer::common
{

/// Resource supplies allocate(args...) and release(GLuint). A GL context must be current when either is called.
template<typename Resource>
class GLHandle
{
public:
    GLHandle() requires requires { Resource::allocate(); }
        : id{Resource::allocate()}
    {
    }

    template<typename... Args>
        requires (sizeof...(Args) > 0 &&
                  (!std::is_same_v<GLHandle, std::remove_cvref_t<Args>> && ...) &&
                  requires(Args&&... args) { Resource::allocate(std::forward<Args>(args)...); })
    explicit GLHandle(Args&&... args)
        : id{Resource::allocate(std::forward<Args>(args)...)}
    {
    }

    ~GLHandle()
    {
        reset();
    }

    GLHandle(GLHandle const&) = delete;

    GLHandle& operator=(GLHandle const&) = delete;

    GLHandle(GLHandle&& from) noexcept
        : id{std::exchange(from.id, 0)}
    {
    }

    GLHandle& operator=(GLHandle&& from) noexcept
    {
        std::swap(id, from.id);
        return *this;
    }

    static auto adopt(GLuint id) -> GLHandle
    {
        return GLHandle{Adopt{}, id};
    }

    void reset()
    {
        if (id)
            Resource::release(std::exchange(id, 0));
    }

    operator GLuint() const
    {
        return id;
    }

private:
    struct Adopt {};

    GLHandle(Adopt, GLuint id)
        : id{id}
    {
    }

    GLuint id;
};

struct ProgramResource
{
    static auto allocate() -> GLuint
    {
        return glCreateProgram();
    }

    static void release(GLuint id)
    {
        glDeleteProgram(id);
    }
};

struct ShaderResource
{
    static auto allocate(GLenum type) -> GLuint
    {
        return glCreateShader(type);
    }

    static void release(GLuint id)
    {
        glDeleteShader(id);
    }
};

template<void (*allocator)(GLsizei, GLuint*), void (*deleter)(GLsizei, GLuint const*)>
struct GLBulkResource
{
    static auto allocate() -> GLuint
    {
        GLuint id{0};
        allocator(1, &id);
        return id;
    }

    static void release(GLuint id)
    {
        deleter(1, &id);
    }
};

using TextureResource = GLBulkResource<&glGenTextures, &glDeleteTextures>;
using BufferResource = GLBulkResource<&glGenBuffers, &glDeleteBuffers>;
using RenderbufferResource = GLBulkResource<&glGenRenderbuffers, &glDeleteRenderbuffers>;
using FramebufferResource = GLBulkResource<&glGenFramebuffers, &glDeleteFramebuffers>;

using ProgramHandle = GLHandle<ProgramResource>;
using ShaderHandle = GLHandle<ShaderResource>;
using TextureHandle = GLHandle<TextureResource>;
using GLBufferHandle = GLHandle<BufferResource>;
using RenderbufferHandle = GLHandle<RenderbufferResource>;
using FramebufferHandle = GLHandle<FramebufferResource>;

}

#endif // MIR_RENDERER_COMMON_GL_HANDLES_H_
