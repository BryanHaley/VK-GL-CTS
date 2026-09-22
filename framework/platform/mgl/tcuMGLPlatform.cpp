/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core -- MGL platform port
 * -------------------------------------------------------------
 *
 * Copyright (C) The MooGL Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief macOS platform that runs the CTS against MGL (libmoogl.dylib).
 *
 * MGL publishes the GL 4.6 core entry points as plain C symbols and builds
 * itself a headless Metal context the moment the dylib is loaded, so there is
 * no CGL/WGL/EGL step here: loading the library IS creating the context.
 *//*--------------------------------------------------------------------*/

#include "tcuPlatform.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuCommandLine.hpp"

#include "gluDefs.hpp"
#include "gluPlatform.hpp"
#include "gluRenderContext.hpp"
#include "gluRenderConfig.hpp"
#include "glwFunctions.hpp"
#include "glwInitFunctions.hpp"
#include "glwEnums.hpp"
#include "deDynamicLibrary.hpp"

#include <cstdlib>
#include <string>

#ifndef MGL_DEFAULT_LIBRARY_PATH
#define MGL_DEFAULT_LIBRARY_PATH "/Users/bryan/Projects/MGL/build/libmoogl.dylib"
#endif

namespace tcu
{
namespace
{

const char *getMGLLibraryPath(void)
{
    const char *env = getenv("MGL_LIBRARY_PATH");

    return (env && env[0]) ? env : MGL_DEFAULT_LIBRARY_PATH;
}

class MGLFunctionLoader : public glw::FunctionLoader
{
public:
    MGLFunctionLoader(de::DynamicLibrary &library) : m_library(library)
    {
    }

    glw::GenericFuncType get(const char *name) const
    {
        return m_library.getFunction(name);
    }

private:
    de::DynamicLibrary &m_library;
};

glu::ApiType queryVersion(const glw::Functions &gl)
{
    int major = 0;
    int minor = 0;

    gl.getIntegerv(GL_MAJOR_VERSION, &major);
    gl.getIntegerv(GL_MINOR_VERSION, &minor);
    GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to query exact GL version");

    return glu::ApiType::core(major, minor);
}

class MGLRenderContext : public glu::RenderContext
{
public:
    MGLRenderContext(const glu::RenderConfig &config)
        : m_type(config.type)
        , m_renderTarget(0, 0, tcu::PixelFormat(0, 0, 0, 0), 0, 0, 0)
    {
        if (config.type.getAPI().getProfile() != glu::PROFILE_CORE)
            throw NotSupportedError("MGL only exposes the core profile");

        m_library = new de::DynamicLibrary(getMGLLibraryPath());

        {
            MGLFunctionLoader loader(*m_library);
            glu::initFunctions(&m_functions, &loader, config.type.getAPI());
        }

        if (!m_functions.getIntegerv || !m_functions.getError)
            throw ResourceError("libmoogl.dylib did not provide the basic GL entry points");

        {
            const glu::ApiType actualApi = queryVersion(m_functions);

            if (!contextSupports(glu::ContextType(actualApi, glu::ContextFlags(0)), config.type.getAPI()))
                throw NotSupportedError("MGL does not support the requested OpenGL version");
        }
    }

    ~MGLRenderContext(void)
    {
        delete m_library;
    }

    // Tests that reach for an entry point by name rather than through the
    // function table need this; without it they get a null pointer and throw.
    glw::GenericFuncType getProcAddress(const char *name) const
    {
        return m_library ? m_library->getFunction(name) : nullptr;
    }

    glu::ContextType getType(void) const
    {
        return m_type;
    }
    const glw::Functions &getFunctions(void) const
    {
        return m_functions;
    }
    const tcu::RenderTarget &getRenderTarget(void) const
    {
        return m_renderTarget;
    }
    void postIterate(void)
    {
    }

private:
    const glu::ContextType m_type;
    glw::Functions m_functions;
    RenderTarget m_renderTarget;
    de::DynamicLibrary *m_library = nullptr;
};

class MGLContextFactory : public glu::ContextFactory
{
public:
    MGLContextFactory(void) : glu::ContextFactory("mgl", "MGL headless Metal context (surfaceless, use fbo)")
    {
    }

    glu::RenderContext *createContext(const glu::RenderConfig &config, const tcu::CommandLine &,
                                      const glu::RenderContext *) const
    {
        return new MGLRenderContext(config);
    }
};

class MGLGLPlatform : public glu::Platform
{
public:
    MGLGLPlatform(void)
    {
        m_contextFactoryRegistry.registerFactory(new MGLContextFactory());
    }
};

class MGLPlatform : public tcu::Platform
{
public:
    MGLPlatform(void)
    {
    }

    const glu::Platform &getGLPlatform(void) const
    {
        return m_gluPlatform;
    }

private:
    MGLGLPlatform m_gluPlatform;
};

} // namespace
} // namespace tcu

tcu::Platform *createPlatform(void)
{
    return new tcu::MGLPlatform();
}
