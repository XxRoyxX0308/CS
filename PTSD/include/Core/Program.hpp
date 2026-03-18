#ifndef CORE_PROGRAM_HPP
#define CORE_PROGRAM_HPP

#include "pch.hpp" // IWYU pragma: export

namespace Core {
/**
 * In OpenGL, programs are objects composed of multiple shaders files compiled
 * and linked together. A typical program would require at least a vertex shader
 * and a fragment shader. However, users could add more optional middle layers
 * such as geometry shaders or tesselation shaders.
 *
 * This class includes uniform location caching to avoid expensive
 * glGetUniformLocation calls during rendering.
 */
class Program {
public:
    Program(const std::string &vertexShaderFilepath,
            const std::string &fragmentShaderFilepath);
    Program(const Program &) = delete;
    Program(Program &&other);

    ~Program();

    Program &operator=(const Program &) = delete;
    Program &operator=(Program &&other);

    GLuint GetId() const { return m_ProgramId; }

    void Bind() const;
    void Unbind() const;

    void Validate() const;

    // ====== Uniform Location Caching ======

    /**
     * @brief Get cached uniform location (avoids glGetUniformLocation overhead).
     *
     * Caches the result on first call for each uniform name.
     * Returns -1 if uniform not found.
     *
     * @param name The uniform variable name in the shader.
     * @return The uniform location, or -1 if not found.
     */
    GLint GetUniformLocation(const std::string &name) const;

    /**
     * @brief Get cached uniform location (C-string overload).
     */
    GLint GetUniformLocation(const char *name) const;

    /**
     * @brief Clear the uniform location cache.
     *
     * Call this if you recompile/relink the program.
     */
    void ClearUniformCache();

    // ====== Uniform Setters (convenience methods) ======

    void SetUniform1i(const char *name, int value) const;
    void SetUniform1f(const char *name, float value) const;
    void SetUniform3f(const char *name, float x, float y, float z) const;
    void SetUniform3fv(const char *name, const float *value) const;
    void SetUniform4fv(const char *name, const float *value) const;
    void SetUniformMatrix4fv(const char *name, const float *value) const;

private:
    void CheckStatus() const;

    GLuint m_ProgramId;
    mutable std::unordered_map<std::string, GLint> m_UniformCache;
};
} // namespace Core
#endif
