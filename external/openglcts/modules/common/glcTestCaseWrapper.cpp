/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 The Khronos Group Inc.
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
 */ /*!
 * \file
 * \brief OpenGL Test Case Wrapper.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCaseWrapper.hpp"
#include "gluStateReset.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <cstdlib>

namespace deqp
{

using tcu::TestLog;

// Bring-up escape hatch: when GLCTS_TOLERATE_STATE_RESET_ERRORS=1 a failing
// glu::resetState is logged and the run keeps going instead of terminating.
// One broken enum would otherwise stop the whole suite after the first test.
// Off by default, so conformance runs behave exactly as upstream.
static bool tolerateStateResetErrors(void)
{
    static const bool s_tolerate = []() {
        const char *env = getenv("GLCTS_TOLERATE_STATE_RESET_ERRORS");
        return env && env[0] == '1';
    }();

    return s_tolerate;
}

static void drainGLErrors(Context &context)
{
    const glw::Functions &gl = context.getRenderContext().getFunctions();

    for (int i = 0; i < 64 && gl.getError() != GL_NO_ERROR; i++)
        ;
}

TestCaseWrapper::TestCaseWrapper(Context &context) : m_testCtx(context.getTestContext()), m_context(context)
{
}

TestCaseWrapper::~TestCaseWrapper(void)
{
}

bool TestCaseWrapper::initTestCase(tcu::TestCase *testCase)
{
    TestLog &log = m_testCtx.getLog();
    bool success = false;

    try
    {
        // Clear state to defaults
        glu::resetState(m_context.getRenderContext(), m_context.getContextInfo());
    }
    catch (const std::exception &e)
    {
        log << e;

        if (!tolerateStateResetErrors())
        {
            log << TestLog::Message << "Error in state reset, test program will terminate." << TestLog::EndMessage;
            return false;
        }

        log << TestLog::Message << "Error in state reset, continuing anyway." << TestLog::EndMessage;
        drainGLErrors(m_context);
    }

    try
    {
        testCase->init();
        success = true;
    }
    catch (const std::bad_alloc &)
    {
        DE_ASSERT(!success);
        m_testCtx.setTestResult(QP_TEST_RESULT_RESOURCE_ERROR, "Failed to allocate memory in test case init");
    }
    catch (const tcu::ResourceError &e)
    {
        DE_ASSERT(!success);
        m_testCtx.setTestResult(QP_TEST_RESULT_RESOURCE_ERROR, "Resource error in test case init");
        log << e;
    }
    catch (const tcu::NotSupportedError &e)
    {
        DE_ASSERT(!success);
        m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
        log << e;
    }
    catch (const tcu::InternalError &e)
    {
        DE_ASSERT(!success);
        m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Internal error in test case init");
        log << e;
    }
    catch (const tcu::Exception &e)
    {
        DE_ASSERT(!success);
        log << e;
    }

    if (!success)
    {
        if (m_testCtx.getTestResult() == QP_TEST_RESULT_LAST)
            m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Error in test case init");
        return false;
    }

    return true;
}

bool TestCaseWrapper::deinitTestCase(tcu::TestCase *testCase)
{
    TestLog &log = m_testCtx.getLog();

    try
    {
        testCase->deinit();
    }
    catch (const tcu::Exception &e)
    {
        log << e;

        if (!tolerateStateResetErrors())
        {
            log << TestLog::Message << "Error in test case deinit, test program will terminate." << TestLog::EndMessage;
            return false;
        }

        log << TestLog::Message << "Error in test case deinit, continuing anyway." << TestLog::EndMessage;
        drainGLErrors(m_context);
    }

    try
    {
        // Clear state to defaults
        glu::resetState(m_context.getRenderContext(), m_context.getContextInfo());
    }
    catch (const std::exception &e)
    {
        log << e;

        if (!tolerateStateResetErrors())
        {
            log << TestLog::Message << "Error in state reset, test program will terminate." << TestLog::EndMessage;
            return false;
        }

        log << TestLog::Message << "Error in state reset, continuing anyway." << TestLog::EndMessage;
        drainGLErrors(m_context);
    }

    return true;
}

tcu::TestNode::IterateResult TestCaseWrapper::iterateTestCase(tcu::TestCase *testCase)
{
    // Iterate the sub-case.
    TestLog &log                               = m_testCtx.getLog();
    tcu::TestCase::IterateResult iterateResult = tcu::TestCase::STOP;

    try
    {
        iterateResult = testCase->iterate();
    }
    catch (const std::bad_alloc &)
    {
        m_testCtx.setTestResult(QP_TEST_RESULT_RESOURCE_ERROR, "Failed to allocate memory during test execution");
    }
    catch (const tcu::ResourceError &e)
    {
        log << e;
        m_testCtx.setTestResult(QP_TEST_RESULT_RESOURCE_ERROR, "Resource error during test execution");
    }
    catch (const tcu::NotSupportedError &e)
    {
        log << e;
        m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
    }
    catch (const tcu::InternalError &e)
    {
        log << e;
        m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Internal error in test execution");
    }
    catch (const tcu::Exception &e)
    {
        log << e;
        m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Error in test execution");
    }

    // Clear buffers
    const glw::Functions &gl = m_context.getRenderContext().getFunctions();
    gl.clearColor(0.f, 0.f, 0.f, 1.f);
    gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    tcu::TestCase::IterateResult result = iterateResult;

    // Call implementation specific post-iterate routine (usually handles native events and swaps buffers)
    try
    {
        m_context.getRenderContext().postIterate();
    }
    catch (const std::exception &e)
    {
        m_testCtx.getLog() << e;
        m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Error in context post-iteration routine");
        return tcu::TestNode::STOP;
    }

    return result;
}

} // namespace deqp
