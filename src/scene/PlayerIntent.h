#pragma once

#include <glm/vec2.hpp>

namespace se::scene {

struct PlayerIntent {
    glm::vec2 moveInput{0.0f};
    glm::vec2 lookDelta{0.0f};
    bool running = false;
    bool toggleCamera = false;

    void accumulateFrame(const PlayerIntent& frameIntent) {
        moveInput = frameIntent.moveInput;
        running = frameIntent.running;
        lookDelta += frameIntent.lookDelta;
        toggleCamera = toggleCamera || frameIntent.toggleCamera;
    }

    void clearFrameEvents() {
        lookDelta = glm::vec2{0.0f};
        toggleCamera = false;
    }
};

}  // namespace se::scene