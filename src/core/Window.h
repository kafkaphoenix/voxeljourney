#pragma once
#include <GLFW/glfw3.h>

#include <string>
#include <string_view>

#include "Config.h"

namespace se::core {

class EventBus;

class Window {
public:
    Window(const Config::Window& config, const Config::Render& renderConfig, EventBus* eventBus);
    ~Window();

    static void pollEvents() { glfwPollEvents(); }
    static void waitEvents(double timeoutSeconds) { glfwWaitEventsTimeout(timeoutSeconds); }
    void swapBuffers() const;
    void setMode(WindowMode mode);
    void onFramebufferResize(int width, int height);
    void onKeyEvent(int key, int scancode, int action, int mods);
    void onMouseButtonEvent(int button, int action, int mods);
    void onMouseMove(double xpos, double ypos);
    void onScroll(double xoffset, double yoffset);
    void onFocusChange(bool focused);
    void onPositionChange(int xpos, int ypos);
    void onSizeChange(int width, int height);
    void onIconifyChange(bool minimized);
    void setStatsTitle(std::string title);
    void setVsync(bool enabled);

    [[nodiscard]] bool shouldClose() const;
    [[nodiscard]] bool isMinimized() const { return m_Minimized; }
    [[nodiscard]] bool isFocused() const { return m_Focused; }
    [[nodiscard]] WindowMode mode() const { return m_Mode; }
    [[nodiscard]] bool isVsync() const { return m_Vsync; }
    [[nodiscard]] std::string_view getBaseTitle() const { return m_BaseTitle; }
    [[nodiscard]] GLFWwindow* native() const { return m_Window; }

private:
    friend void windowGlDebugCallback(unsigned int source, unsigned int type, unsigned int id, unsigned int severity,
                                      int length, const char* message, const void* userParam);
    static void initGlfw();
    static void setupGlfwHints(const Config::Render& renderConfig);
    void createWindow(int width, int height, std::string_view title);
    void initGlad();
    void setupGlDebug();
    void setupCallbacks();
    void setupInitialFramebuffer(int width, int height);

    GLFWwindow* m_Window = nullptr;
    EventBus* m_EventBus = nullptr;
    int m_Width;
    int m_Height;
    int m_PosX;
    int m_PosY;
    bool m_Minimized = false;
    bool m_Focused = true;
    bool m_IgnoreSizeEvents = false;
    WindowMode m_LastMode = WindowMode::Windowed;
    std::string m_Title;
    std::string m_BaseTitle;
    bool m_Vsync;
    WindowMode m_Mode = WindowMode::Windowed;
    int m_LastFramebufferWidth = 0;
    int m_LastFramebufferHeight = 0;
};

}  // namespace se::core
