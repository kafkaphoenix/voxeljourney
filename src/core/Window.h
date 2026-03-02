#pragma once
#include <GLFW/glfw3.h>

#include <string>
#include <string_view>

#include "Config.h"

namespace se::core {

class EventBus;

class Window {
   public:
    Window(int width, int height, std::string title, EventBus* eventBus);
    ~Window();

    void pollEvents() const;
    void waitEvents(double timeoutSeconds) const;
    void swapBuffers() const;
    bool shouldClose() const;
    bool isMinimized() const { return m_Minimized; }
    bool isFocused() const { return m_Focused; }
    void toggleFullscreen();
    void onFramebufferResize(int width, int height);
    void onKeyEvent(int key, int scancode, int action, int mods);
    void onMouseButtonEvent(int button, int action, int mods);
    void onMouseMove(double xpos, double ypos);
    void onScroll(double xoffset, double yoffset);
    void onWindowFocus(bool focused);
    void onWindowPos(int xpos, int ypos);
    void onWindowSize(int width, int height);
    void onWindowIconify(bool minimized);
    void setStatsTitle(std::string title);
    std::string_view getBaseTitle() const { return m_BaseTitle; }
    void setVsync(bool enabled);
    void applyConfig(const Config::Window& config);

    GLFWwindow* native() const { return m_Window; }

   private:
    friend void windowGlDebugCallback(unsigned int source, unsigned int type,
                                      unsigned int id, unsigned int severity,
                                      int length, const char* message, const void* userParam);
    void initGlfw();
    void setupGlfwHints();
    void createWindow(int width, int height, std::string_view title);
    void initGlad();
    void setupGlDebug();
    void setupCallbacks();
    void setupInitialFramebuffer(int width, int height);
    void setupInputMode();

    GLFWwindow* m_Window;
    EventBus* m_EventBus = nullptr;
    bool m_IsFullscreen = false;
    int m_WindowedWidth;
    int m_WindowedHeight;
    int m_WindowedPosX;
    int m_WindowedPosY;
    int m_LastFramebufferWidth = 0;
    int m_LastFramebufferHeight = 0;
    bool m_Minimized = false;
    bool m_Focused = true;
    std::string m_Title;
    std::string m_BaseTitle;
};

}  // namespace se::core
