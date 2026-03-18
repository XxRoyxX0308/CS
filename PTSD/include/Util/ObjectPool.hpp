#ifndef UTIL_OBJECT_POOL_HPP
#define UTIL_OBJECT_POOL_HPP

#include "pch.hpp" // IWYU pragma: export

namespace Util {

/**
 * @brief A fixed-size object pool for frequent allocation/deallocation.
 *
 * Pre-allocates a block of objects to avoid per-frame heap allocations.
 * Use for bullets, particles, temporary render data, etc.
 *
 * @tparam T The object type (must be default-constructible).
 *
 * @code
 * Util::ObjectPool<Bullet> bulletPool(100);
 *
 * // Acquire an object
 * Bullet* bullet = bulletPool.Acquire();
 * if (bullet) {
 *     bullet->Fire(position, direction);
 * }
 *
 * // Release when done
 * bulletPool.Release(bullet);
 * @endcode
 */
template <typename T>
class ObjectPool {
public:
    /**
     * @brief Create a pool with initial capacity.
     * @param capacity Maximum number of objects.
     */
    explicit ObjectPool(size_t capacity)
        : m_Capacity(capacity) {
        m_Objects.resize(capacity);
        m_Available.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            m_Available.push_back(&m_Objects[i]);
        }
    }

    ObjectPool(const ObjectPool &) = delete;
    ObjectPool &operator=(const ObjectPool &) = delete;

    ObjectPool(ObjectPool &&other) noexcept
        : m_Objects(std::move(other.m_Objects)),
          m_Available(std::move(other.m_Available)),
          m_Capacity(other.m_Capacity) {
        other.m_Capacity = 0;
    }

    ObjectPool &operator=(ObjectPool &&other) noexcept {
        if (this != &other) {
            m_Objects = std::move(other.m_Objects);
            m_Available = std::move(other.m_Available);
            m_Capacity = other.m_Capacity;
            other.m_Capacity = 0;
        }
        return *this;
    }

    /**
     * @brief Acquire an object from the pool.
     * @return Pointer to an available object, or nullptr if pool is exhausted.
     */
    T *Acquire() {
        if (m_Available.empty()) {
            return nullptr;
        }
        T *obj = m_Available.back();
        m_Available.pop_back();
        return obj;
    }

    /**
     * @brief Release an object back to the pool.
     * @param obj The object to release (must have been acquired from this pool).
     */
    void Release(T *obj) {
        if (obj && IsFromPool(obj)) {
            m_Available.push_back(obj);
        }
    }

    /**
     * @brief Reset all objects and make them available.
     */
    void ReleaseAll() {
        m_Available.clear();
        m_Available.reserve(m_Capacity);
        for (size_t i = 0; i < m_Capacity; ++i) {
            m_Available.push_back(&m_Objects[i]);
        }
    }

    /** @brief Get the total capacity. */
    size_t GetCapacity() const { return m_Capacity; }

    /** @brief Get the number of currently available objects. */
    size_t GetAvailableCount() const { return m_Available.size(); }

    /** @brief Get the number of objects currently in use. */
    size_t GetUsedCount() const { return m_Capacity - m_Available.size(); }

    /** @brief Check if pool is empty (all objects in use). */
    bool IsEmpty() const { return m_Available.empty(); }

    /** @brief Check if pool is full (no objects in use). */
    bool IsFull() const { return m_Available.size() == m_Capacity; }

private:
    bool IsFromPool(const T *obj) const {
        if (m_Objects.empty()) return false;
        const T *start = &m_Objects[0];
        const T *end = start + m_Capacity;
        return obj >= start && obj < end;
    }

    std::vector<T> m_Objects;
    std::vector<T *> m_Available;
    size_t m_Capacity;
};

/**
 * @brief A growable object pool that can expand when exhausted.
 *
 * @tparam T The object type.
 */
template <typename T>
class DynamicObjectPool {
public:
    /**
     * @brief Create a pool with initial capacity and growth factor.
     * @param initialCapacity Starting number of objects.
     * @param growthFactor How much to grow when exhausted (multiplier).
     */
    explicit DynamicObjectPool(size_t initialCapacity = 64,
                                float growthFactor = 2.0f)
        : m_GrowthFactor(growthFactor) {
        Grow(initialCapacity);
    }

    DynamicObjectPool(const DynamicObjectPool &) = delete;
    DynamicObjectPool &operator=(const DynamicObjectPool &) = delete;

    /**
     * @brief Acquire an object, growing the pool if necessary.
     * @return Pointer to an available object.
     */
    T *Acquire() {
        if (m_Available.empty()) {
            size_t newSize = static_cast<size_t>(
                static_cast<float>(m_TotalCapacity) * m_GrowthFactor);
            if (newSize == m_TotalCapacity) newSize++;
            Grow(newSize - m_TotalCapacity);
        }
        T *obj = m_Available.back();
        m_Available.pop_back();
        return obj;
    }

    /**
     * @brief Release an object back to the pool.
     */
    void Release(T *obj) {
        if (obj) {
            m_Available.push_back(obj);
        }
    }

    size_t GetTotalCapacity() const { return m_TotalCapacity; }
    size_t GetAvailableCount() const { return m_Available.size(); }
    size_t GetUsedCount() const { return m_TotalCapacity - m_Available.size(); }

private:
    void Grow(size_t additionalCapacity) {
        auto block = std::make_unique<std::vector<T>>(additionalCapacity);
        for (size_t i = 0; i < additionalCapacity; ++i) {
            m_Available.push_back(&(*block)[i]);
        }
        m_Blocks.push_back(std::move(block));
        m_TotalCapacity += additionalCapacity;
    }

    std::vector<std::unique_ptr<std::vector<T>>> m_Blocks;
    std::vector<T *> m_Available;
    size_t m_TotalCapacity = 0;
    float m_GrowthFactor;
};

} // namespace Util

#endif
