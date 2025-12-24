#pragma once

#include <GL/gl.h>

namespace ui {

/**
 * @brief RAII wrapper for OpenGL texture handles.
 * Automatically deletes the texture when the handle goes out of scope.
 */
class GLTextureHandle {
public:
    GLTextureHandle() : m_id(0) {}
    
    explicit GLTextureHandle(GLuint id) : m_id(id) {}
    
    ~GLTextureHandle() {
        Reset();
    }

    // Prevent copying to avoid double deletion
    GLTextureHandle(const GLTextureHandle&) = delete;
    GLTextureHandle& operator=(const GLTextureHandle&) = delete;

    // Allow moving
    GLTextureHandle(GLTextureHandle&& other) noexcept : m_id(other.m_id) {
        other.m_id = 0;
    }

    GLTextureHandle& operator=(GLTextureHandle&& other) noexcept {
        if (this != &other) {
            Reset();
            m_id = other.m_id;
            other.m_id = 0;
        }
        return *this;
    }

    [[nodiscard]] GLuint Get() const { return m_id; }
    [[nodiscard]] bool IsValid() const { return m_id != 0; }
    
    operator GLuint() const { return m_id; }

    void Gen() {
        Reset();
        glGenTextures(1, &m_id);
    }

    void Reset() {
        if (m_id != 0) {
            glDeleteTextures(1, &m_id);
            m_id = 0;
        }
    }

private:
    GLuint m_id;
};

} // namespace ui
