#ifndef CORE3D_MATRICES3D_HPP
#define CORE3D_MATRICES3D_HPP

#include "pch.hpp" // IWYU pragma: export

namespace Core3D {

/**
 * @brief 3D uniform buffer data passed to shaders via UBO.
 *
 * Layout matches std140; each mat4 = 64 bytes, vec4 = 16 bytes.
 * Total size: 4 * 64 + 16 = 272 bytes.
 */
struct Matrices3D {
    glm::mat4 m_Model{1.0f};
    glm::mat4 m_View{1.0f};
    glm::mat4 m_Projection{1.0f};
    glm::mat4 m_Normal{1.0f};    ///< transpose(inverse(model)) for lighting
    glm::vec4 m_CameraPos{0.0f}; ///< xyz = camera world position, w unused
};

} // namespace Core3D

#endif
