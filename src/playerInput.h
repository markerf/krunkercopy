#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/geometric.hpp>

class PlayerInput {
public:
    glm::vec3 position;
    glm::vec3 velocity;
    float jumpImpulse = 0.0f;

    PlayerInput(GLFWwindow* win)
        : window(win),
          position(0.0f, 10.0f, 5.0f),
          velocity(0.0f, 0.0f, 0.0f),
          worldUp(0.0f, 1.0f, 0.0f),
          yaw(-90.0f),
          pitch(0.0f),
          acccoef(3.0f),
          movementSpeed(7.0f),
          mouseSensitivity(0.1f),
          firstMouse(true),
          onground(false),
          spaceWasPressed(false),
          coyoteTime(0.0f),
          jumpTimer(0.0f)
    {
        front = glm::vec3(0.0f, 0.0f, -1.0f);
        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void update(float deltaTime) {
        processKeyboard(deltaTime);
        processMouse();
    }

    void updatepos(glm::vec3 newpos)     { position  = newpos;  }
    void updatevel(glm::vec3 newvel)     { velocity  = newvel;  }
    void updateground(bool ground)       { onground  = ground;  }

    glm::vec3 getPosition() const { return position; }
    glm::vec3 getVelocity() const { return velocity; }

    glm::mat4 getMVPMatrix() const {
        glm::mat4 view        = glm::lookAt(position, position + front, up);
        glm::mat4 perspective = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 10000.0f);
        return perspective * view;
    }

private:
    GLFWwindow* window;

    glm::vec3 front, right, up, worldUp;

    float yaw, pitch;
    float acccoef;
    float movementSpeed;
    float mouseSensitivity;
    float lastX, lastY;

    bool firstMouse;
    bool onground;
    bool spaceWasPressed;

    // coyote time — lets you jump briefly after walking off a ledge
    float coyoteTime;
    const float coyoteMax = 0.1f;

    // jump timer — spreads jump force over multiple frames so it feels smooth
    float jumpTimer;
    const float jumpDuration = 0.80f;
    const float jumpForce    = 6.0f;   // tune this for jump height

    void processKeyboard(float deltaTime) {
        // --- friction ---
        velocity *= 0.85f;

        float acc = acccoef * deltaTime;

        // --- horizontal movement ---
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            velocity += front * acc;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            velocity -= front * acc;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            velocity -= right * acc;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            velocity += right * acc;

        // --- horizontal speed cap (leaves Y alone) ---
        glm::vec2 horizontalVel(velocity.x, velocity.z);
        if (glm::length(horizontalVel) > movementSpeed) {
            horizontalVel = glm::normalize(horizontalVel) * movementSpeed;
            velocity.x = horizontalVel.x;
            velocity.z = horizontalVel.y;
        }

        // --- coyote time ---
        if (onground)
            coyoteTime = coyoteMax;
        else
            coyoteTime -= deltaTime;

        bool canJump = coyoteTime > 0.0f;

        // --- jump (one shot, not every frame) ---
        bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spacePressed && !spaceWasPressed && canJump)
            jumpTimer = jumpDuration;
        spaceWasPressed = spacePressed;

        // --- apply jump force gradually over jumpDuration ---
        if (jumpTimer > 0.0f) {
            float ratio = jumpTimer / jumpDuration; // 1.0 → 0.0 as timer runs out
            velocity.y += jumpForce * ratio * deltaTime;
            jumpTimer  -= deltaTime;
        }

        // --- gravity ---
        // fall faster than rising for snappier feel
        velocity.y -= 3.0f  * deltaTime;

        // --- crouch ---
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            velocity -= up * acc;

        // --- wireframe toggle ---
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    void processMouse() {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = (xpos - lastX) * mouseSensitivity;
        float yoffset = (lastY - ypos) * mouseSensitivity;
        lastX = xpos;
        lastY = ypos;

        yaw   += xoffset;
        pitch += yoffset;
        if (pitch >  89.0f) pitch =  89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        front = glm::normalize(direction);
        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
    }
};

