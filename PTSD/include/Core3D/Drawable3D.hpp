#ifndef CORE3D_DRAWABLE3D_HPP
#define CORE3D_DRAWABLE3D_HPP

#include "pch.hpp" // IWYU pragma: export
#include "Core3D/Matrices3D.hpp"

namespace Core3D {

/**
 * @brief Abstract interface for all 3D renderable objects.
 *
 * Analogous to Core::Drawable for 2D. Any object that can be drawn in
 * a 3D scene must implement this interface.
 */
class Drawable3D {
public:
    virtual ~Drawable3D() = default;

    /**
     * @brief Draw this object with the given 3D matrices.
     * @param data The model/view/projection/normal matrices and camera pos.
     */
    virtual void Draw(const Core3D::Matrices3D &data) = 0;

    /**
     * @brief Get the axis-aligned bounding box dimensions.
     * @return vec3(width, height, depth) of the bounding box.
     */
    virtual glm::vec3 GetBoundingBox() const = 0;
};

} // namespace Core3D

#endif
