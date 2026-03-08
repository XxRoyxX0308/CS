#include "Core3D/Cubemap.hpp"
#include "Util/Logger.hpp"

#include "../../lib/stb/stb_image.h"

namespace Core3D {

Cubemap::Cubemap(const std::vector<std::string> &faces) {
    if (faces.size() != 6) {
        LOG_ERROR("Cubemap requires exactly 6 face images, got {}",
                  faces.size());
        return;
    }

    glGenTextures(1, &m_TextureId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureId);

    for (unsigned int i = 0; i < 6; ++i) {
        int width, height, nrChannels;
        unsigned char *data =
            stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 1) format = GL_RED;
            else if (nrChannels == 4) format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                         static_cast<GLint>(format), width, height, 0,
                         format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            LOG_ERROR("Cubemap face failed to load: '{}'", faces[i]);
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

Cubemap::Cubemap(Cubemap &&other) {
    m_TextureId = other.m_TextureId;
    other.m_TextureId = 0;
}

Cubemap::~Cubemap() {
    if (m_TextureId != 0) {
        glDeleteTextures(1, &m_TextureId);
    }
}

Cubemap &Cubemap::operator=(Cubemap &&other) {
    if (this != &other) {
        if (m_TextureId != 0) {
            glDeleteTextures(1, &m_TextureId);
        }
        m_TextureId = other.m_TextureId;
        other.m_TextureId = 0;
    }
    return *this;
}

void Cubemap::Bind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureId);
}

void Cubemap::Unbind() const {
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

} // namespace Core3D
