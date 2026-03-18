#include "Core/Program.hpp"

#include "Core/Shader.hpp"

#include "Util/Logger.hpp"

namespace Core {
Program::Program(const std::string &vertexShaderFilepath,
                 const std::string &fragmentShaderFilepath) {
    m_ProgramId = glCreateProgram();

    Shader vertex(vertexShaderFilepath, Shader::Type::VERTEX);
    Shader fragment(fragmentShaderFilepath, Shader::Type::FRAGMENT);

    glAttachShader(m_ProgramId, vertex.GetShaderId());
    glAttachShader(m_ProgramId, fragment.GetShaderId());

    glLinkProgram(m_ProgramId);

    CheckStatus();

    glDetachShader(m_ProgramId, vertex.GetShaderId());
    glDetachShader(m_ProgramId, fragment.GetShaderId());
}

Program::Program(Program &&other) {
    m_ProgramId = other.m_ProgramId;
    m_UniformCache = std::move(other.m_UniformCache);
    other.m_ProgramId = 0;
}

Program::~Program() {
    glDeleteProgram(m_ProgramId);
}

Program &Program::operator=(Program &&other) {
    m_ProgramId = other.m_ProgramId;
    m_UniformCache = std::move(other.m_UniformCache);
    other.m_ProgramId = 0;

    return *this;
}

void Program::Bind() const {
    glUseProgram(m_ProgramId);
}

void Program::Unbind() const {
    glUseProgram(0);
}

void Program::Validate() const {
    GLint status = GL_FALSE;

    glValidateProgram(m_ProgramId);
    glGetProgramiv(m_ProgramId, GL_VALIDATE_STATUS, &status);
    if (status != GL_TRUE) {
        int infoLogLength;
        glGetProgramiv(m_ProgramId, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<char> message(infoLogLength + 1);
        glGetProgramInfoLog(m_ProgramId, infoLogLength, nullptr,
                            message.data());

        LOG_ERROR("Validation Failed:");
        LOG_ERROR("{}", message.data());
    }
}

// ====== Uniform Location Caching ======

GLint Program::GetUniformLocation(const std::string &name) const {
    auto it = m_UniformCache.find(name);
    if (it != m_UniformCache.end()) {
        return it->second;
    }

    GLint location = glGetUniformLocation(m_ProgramId, name.c_str());
    m_UniformCache[name] = location;
    return location;
}

GLint Program::GetUniformLocation(const char *name) const {
    return GetUniformLocation(std::string(name));
}

void Program::ClearUniformCache() {
    m_UniformCache.clear();
}

// ====== Uniform Setters ======

void Program::SetUniform1i(const char *name, int value) const {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniform1i(loc, value);
}

void Program::SetUniform1f(const char *name, float value) const {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniform1f(loc, value);
}

void Program::SetUniform3f(const char *name, float x, float y, float z) const {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniform3f(loc, x, y, z);
}

void Program::SetUniform3fv(const char *name, const float *value) const {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniform3fv(loc, 1, value);
}

void Program::SetUniform4fv(const char *name, const float *value) const {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniform4fv(loc, 1, value);
}

void Program::SetUniformMatrix4fv(const char *name, const float *value) const {
    GLint loc = GetUniformLocation(name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, value);
}

void Program::CheckStatus() const {
    GLint status = GL_FALSE;

    glGetProgramiv(m_ProgramId, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        int infoLogLength;
        glGetProgramiv(m_ProgramId, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<char> message(infoLogLength + 1);
        glGetProgramInfoLog(m_ProgramId, infoLogLength, nullptr,
                            message.data());

        LOG_ERROR("Failed to Link Program:");
        LOG_ERROR("{}", message.data());
    }
}
} // namespace Core
