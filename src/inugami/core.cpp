/*******************************************************************************
 * Inugami - An OpenGL framework designed for rapid game development
 * Version: 0.3.0
 * https://github.com/DBRalir/Inugami
 *
 * Copyright (c) 2012 Jeramy Harrison <dbralir@gmail.com>
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *
 *  3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#include "core.hpp"

#include "camera.hpp"
#include "exception.hpp"
#include "interface.hpp"
#include "loaders.hpp"
#include "shaderprogram.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>

#include "../meta.hpp"

namespace Inugami {

int Core::numCores = 0;

namespace {

void errorCB(int i, const char* str)
{
    logger->log<0>("Error ", i, ": ", str);
}

}

class CoreException : public Exception
{
public:
    CoreException() = delete;

    CoreException(Core* c, std::string error)
        : core(c)
        , err("")
    {
        err += "Core Exception: ";
        err += error;
    }

    const char* what() const noexcept override
    {
        return err.c_str();
    }

    Core* core;
    std::string err;
};

Core::RenderParams::RenderParams()
    : width(-1)
    , height(-1)
    , fullscreen(false)
    , vsync(true)
    , fsaaSamples(0)
{}

Core::Core(const RenderParams &params)
    : running(false)
    , iface(nullptr)

    , callbacks()

    , frameStartTime(0.f)
    , frameRateStack(10, 0.0)
    , frStackIterator(frameRateStack.begin())

    , rparams(params)

    , windowTitle("Inugami")
    , windowTitleShowFPS(false)
    , window(nullptr)

    , viewProjection(1.f)

    , shader()
{
    if (numCores == 0)
    {
        glfwSetErrorCallback(errorCB);
        glfwInit();
    }

    GLFWmonitor* monitor = nullptr;

    if (rparams.width == -1 || rparams.height == -1)
    {
        if (rparams.fullscreen)
        {
            monitor = glfwGetPrimaryMonitor();
            auto vidmode = glfwGetVideoMode(monitor);
            rparams.width = vidmode->width;
            rparams.height = vidmode->height;
        }
        else
        {
            rparams.width = 800;
            rparams.height = 600;
        }
    }

    glfwWindowHint(GLFW_SAMPLES, rparams.fsaaSamples);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    window = glfwCreateWindow(
          rparams.width
        , rparams.height
        , windowTitle.c_str()
        , monitor
        , nullptr
    );

    if (!window)
    {
        throw CoreException(this, "Failed to open window.");
    }

    activate();

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        throw CoreException(this, "Failed to initialize GLEW.");
    }

    if (rparams.vsync) glfwSwapInterval(1);
    else glfwSwapInterval(0);

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_FLAT);

    glClearColor(0.3, 0.3, 0.3, 0.0);

    glClearDepth(1);
    glDepthFunc(GL_LEQUAL);

    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    shader = Shader(ShaderProgram::fromDefault());

    iface = std::unique_ptr<Interface>(new Interface(window)); //! @todo make_unique

    ++numCores;
}

Core::~Core()
{
    activate();

    glfwDestroyWindow(window);

    if (--numCores == 0)
    {
        glfwTerminate();
    }
}

void Core::activate() const
{
    glfwMakeContextCurrent(window);
}

void Core::deactivate() const
{
    glfwMakeContextCurrent(nullptr);
}

void Core::beginFrame()
{
    activate();

    *frStackIterator = getInstantFrameRate();
    ++frStackIterator;

    if (frStackIterator == end(frameRateStack))
    {
        frStackIterator = begin(frameRateStack);
    }

    frameStartTime = glfwGetTime();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    getShader().bind();

    //Title
    if (windowTitleShowFPS)
    {
        std::stringstream ss;
        ss << windowTitle << " (" << std::fixed << std::setprecision(2) << getAverageFrameRate() << " FPS)";
        glfwSetWindowTitle(window, ss.str().c_str());
    }
}

void Core::endFrame()
{
    glfwSwapBuffers(window);
}

double Core::getInstantFrameRate() const
{
    return 1.0/(glfwGetTime() - frameStartTime);
}

double Core::getAverageFrameRate() const
{
    double sum = 0;
    for (auto&& i : frameRateStack) sum += i;
    return sum/10.0;
}

void Core::applyCam(const Camera& in, bool clearDepth)
{
    activate();

    //Cull faces
    if (in.cullFaces) glEnable (GL_CULL_FACE);
    else              glDisable(GL_CULL_FACE);

    if (in.depthTest) glEnable (GL_DEPTH_TEST);
    else              glDisable(GL_DEPTH_TEST);

#ifndef INU_NO_SHADERS
    getShader().uniform("projectionMatrix").set(in.getProjection());
    getShader().uniform("viewMatrix"      ).set(in.getView()      );
    getShader().uniform("modelMatrix"     ).set(glm::mat4(1.f)    );
#endif // INU_NO_SHADERS

    viewProjection = in.getProjection()*in.getView();

    if (clearDepth) glClear(GL_DEPTH_BUFFER_BIT);
}

void Core::modelMatrix(const Mat4& in)
{
    activate();

#ifndef INU_NO_SHADERS
    getShader().uniform("modelMatrix").set(in               );
    getShader().uniform("MVP"        ).set(viewProjection*in);
#else
    auto modelmat = viewProjection * in;
    glLoadMatrixf(&modelmat[0][0]);
#endif // INU_NO_SHADERS
}

void Core::setWindowTitle(const char* text, bool showFPS)
{
    windowTitle = text;
    windowTitleShowFPS = showFPS;
    glfwSetWindowTitle(window, text);
}

void Core::setWindowTitle(std::string const& text, bool showFPS)
{
    windowTitle = text;
    windowTitleShowFPS = showFPS;
    glfwSetWindowTitle(window, text.c_str());
}

const Core::RenderParams& Core::getParams() const
{
    return rparams;
}

void Core::addCallback(std::function<void()> func, double freq)
{
    callbacks.push_back({func, freq, glfwGetTime()});
}

void Core::clearCallbacks()
{
    callbacks.clear();
}

void Core::go()
{
    running = true;

    double currentTime = 0.0;
    double deltaTime = 0.0;

    while (running)
    {
        for (auto& cb : callbacks)
        {
            if (cb.freq < 0.0)
            {
                cb.func();
                continue;
            }

            currentTime = glfwGetTime();
            deltaTime = (currentTime - cb.last)*cb.freq;

            if (deltaTime >= 1.0)
            {
                cb.last = currentTime;
                cb.func();
            }

            if (!running) break;
        }
    }
}

const Shader& Core::getShader() const
{
    return shader;
}

void Core::setShader(const Shader& in)
{
    shader = in;
    shader.bind();
}

int Core::getWindowAttrib(int param) const
{
    return glfwGetWindowAttrib(window, param);
}

bool Core::shouldClose() const
{
    return glfwWindowShouldClose(window);
}

} // namespace Inugami
