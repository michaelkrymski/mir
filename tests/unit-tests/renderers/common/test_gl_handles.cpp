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

#include "common/gl_handles.h"

#include <mir/test/doubles/mock_gl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

namespace mrc = mir::renderer::common;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
static_assert(!std::is_copy_constructible_v<mrc::ProgramHandle>);
static_assert(!std::is_copy_assignable_v<mrc::ProgramHandle>);
static_assert(!std::is_constructible_v<mrc::ShaderHandle, mrc::ShaderHandle&>);
static_assert(!std::is_default_constructible_v<mrc::ShaderHandle>);
static_assert(!std::is_constructible_v<mrc::ProgramHandle, GLuint>);
static_assert(std::is_nothrow_move_constructible_v<mrc::ShaderHandle>);
static_assert(std::is_nothrow_move_assignable_v<mrc::ShaderHandle>);

class GLHandleTest : public Test
{
public:
    StrictMock<mtd::MockGL> gl;
};
}

TEST_F(GLHandleTest, default_construction_allocates_program)
{
    EXPECT_CALL(gl, glCreateProgram()).WillOnce(Return(42));
    EXPECT_CALL(gl, glDeleteProgram(42));

    mrc::ProgramHandle const handle;

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(42u));
}

TEST_F(GLHandleTest, shader_construction_passes_type_to_allocator)
{
    EXPECT_CALL(gl, glCreateShader(GL_VERTEX_SHADER)).WillOnce(Return(42));
    EXPECT_CALL(gl, glDeleteShader(42));

    mrc::ShaderHandle const handle{GL_VERTEX_SHADER};

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(42u));
}

TEST_F(GLHandleTest, default_construction_allocates_one_texture)
{
    EXPECT_CALL(gl, glGenTextures(1, _)).WillOnce(SetArgPointee<1>(42));
    EXPECT_CALL(gl, glDeleteTextures(1, Pointee(42)));

    mrc::TextureHandle const handle;

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(42u));
}

TEST_F(GLHandleTest, default_construction_allocates_one_buffer)
{
    EXPECT_CALL(gl, glGenBuffers(1, _)).WillOnce(SetArgPointee<1>(42));
    EXPECT_CALL(gl, glDeleteBuffers(1, Pointee(42)));

    mrc::GLBufferHandle const handle;

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(42u));
}

TEST_F(GLHandleTest, default_construction_allocates_one_renderbuffer)
{
    EXPECT_CALL(gl, glGenRenderbuffers(1, _)).WillOnce(SetArgPointee<1>(42));
    EXPECT_CALL(gl, glDeleteRenderbuffers(1, Pointee(42)));

    mrc::RenderbufferHandle const handle;

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(42u));
}

TEST_F(GLHandleTest, default_construction_allocates_one_framebuffer)
{
    EXPECT_CALL(gl, glGenFramebuffers(1, _)).WillOnce(SetArgPointee<1>(42));
    EXPECT_CALL(gl, glDeleteFramebuffers(1, Pointee(42)));

    mrc::FramebufferHandle const handle;

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(42u));
}

TEST_F(GLHandleTest, adoption_does_not_allocate)
{
    EXPECT_CALL(gl, glDeleteProgram(42));

    auto const handle = mrc::ProgramHandle::adopt(42);

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(42u));
}

TEST_F(GLHandleTest, shader_adoption_does_not_treat_id_as_shader_type)
{
    EXPECT_CALL(gl, glDeleteShader(42));

    auto const handle = mrc::ShaderHandle::adopt(42);

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(42u));
}

TEST_F(GLHandleTest, adoption_of_zero_does_not_allocate_or_delete)
{
    auto handle = mrc::TextureHandle::adopt(0);

    handle.reset();

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(0u));
}

TEST_F(GLHandleTest, failed_allocation_is_not_deleted)
{
    EXPECT_CALL(gl, glCreateProgram()).WillOnce(Return(0));

    mrc::ProgramHandle const handle;

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(0u));
}

TEST_F(GLHandleTest, bulk_allocation_that_does_not_write_an_id_leaves_handle_empty)
{
    EXPECT_CALL(gl, glGenFramebuffers(1, _));

    mrc::FramebufferHandle const handle;

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(0u));
}

TEST_F(GLHandleTest, move_construction_transfers_ownership_and_clears_source)
{
    EXPECT_CALL(gl, glDeleteTextures(1, Pointee(42)));
    auto source = mrc::TextureHandle::adopt(42);

    auto const destination = std::move(source);
    source.reset();

    EXPECT_THAT(static_cast<GLuint>(source), Eq(0u));
    EXPECT_THAT(static_cast<GLuint>(destination), Eq(42u));
}

TEST_F(GLHandleTest, move_assignment_exchanges_ownership)
{
    InSequence const sequence;
    EXPECT_CALL(gl, glDeleteProgram(12));
    EXPECT_CALL(gl, glDeleteProgram(42));
    auto destination = mrc::ProgramHandle::adopt(12);
    {
        auto source = mrc::ProgramHandle::adopt(42);

        destination = std::move(source);

        EXPECT_THAT(static_cast<GLuint>(source), Eq(12u));
        EXPECT_THAT(static_cast<GLuint>(destination), Eq(42u));
    }
}

TEST_F(GLHandleTest, move_assignment_to_empty_handle_clears_source)
{
    EXPECT_CALL(gl, glDeleteShader(42));
    auto destination = mrc::ShaderHandle::adopt(0);
    auto source = mrc::ShaderHandle::adopt(42);

    destination = std::move(source);

    EXPECT_THAT(static_cast<GLuint>(source), Eq(0u));
    EXPECT_THAT(static_cast<GLuint>(destination), Eq(42u));
}

TEST_F(GLHandleTest, self_move_assignment_keeps_ownership)
{
    EXPECT_CALL(gl, glDeleteProgram(42));
    auto handle = mrc::ProgramHandle::adopt(42);
    auto& same_handle = handle;

    handle = std::move(same_handle);

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(42u));
}

TEST_F(GLHandleTest, reset_releases_and_clears_handle_exactly_once)
{
    auto handle = mrc::RenderbufferHandle::adopt(42);
    EXPECT_CALL(gl, glDeleteRenderbuffers(1, Pointee(42)));

    handle.reset();

    EXPECT_THAT(static_cast<GLuint>(handle), Eq(0u));
    Mock::VerifyAndClearExpectations(&gl);
    handle.reset();
}

TEST_F(GLHandleTest, exception_unwinding_releases_handle)
{
    EXPECT_CALL(gl, glDeleteShader(42));

    EXPECT_THROW(
        {
            auto const handle = mrc::ShaderHandle::adopt(42);
            throw 1;
        },
        int);
}
