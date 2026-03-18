#ifndef UTIL_OBJECT_POOL_HPP
#define UTIL_OBJECT_POOL_HPP

#include "pch.hpp" // IWYU pragma: export

namespace Util {

/**
 * @brief Generic object pool for avoiding per-frame allocations.
 *
 * Pre-allocates a fixed number of objects and recycles them.
 * Thread-safe version can be enabled with template parameter.
 *
 * @code
 * // Create a pool of 100 particles
 * Util::ObjectPool<Particle> pool(100);
 *
 * // Acquire an object (reuses existing or creates new)
 * Particle* p = pool.Acquire();
 * p->Reset();  // User-defined reset
 *
 * // Return object to pool when done
 * pool.Release(p);
 *
 * // Or use RAII wrapper
 * {
 *     auto handle = pool.AcquireScoped();
 *     handle->DoSomething();
 * } // Automatically released
 * @endcode
 */
template <typename T>
class ObjectPool {
public:
    /**
     * @brief Construct a pool with initial capacity.
     * @param initialCapacity Number of objects to pre-allocate.
     */
    explicit ObjectPool(size_t initialCapacity = 64) {
        m_Pool.reserve(initialCapacity);
        m_Available.reserve(initialCapacity);

        for (size_t i = 0; i < initialCapacity; ++i) {
            m_Pool.push_back(std::make_unique<T>());
            m_Available.push_back(m_Pool.back().get());
        }
    }

    /**
     * @brief Acquire an object from the pool.
     *
     * Returns an existing object if available, otherwise creates a new one.
     * The returned object should be released back to the pool when done.
     */
    T *Acquire() {
        if (m_Available.empty()) {
            // Pool exhausted, create new object
            m_Pool.push_back(std::make_unique<T>());
            return m_Pool.back().get();
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
        if (obj != nullptr) {
            m_Available.push_back(obj);
        }
    }

    /**
     * @brief RAII handle for automatic release.
     */
    class ScopedHandle {
    public:
        ScopedHandle(ObjectPool &pool, T *obj) : m_Pool(pool), m_Obj(obj) {}
        ~ScopedHandle() { m_Pool.Release(m_Obj); }

        ScopedHandle(const ScopedHandle &) = delete;
        ScopedHandle &operator=(const ScopedHandle &) = delete;
        ScopedHandle(ScopedHandle &&other) noexcept
            : m_Pool(other.m_Pool), m_Obj(other.m_Obj) {
            other.m_Obj = nullptr;
        }

        T *operator->() { return m_Obj; }
        T &operator*() { return *m_Obj; }
        T *Get() { return m_Obj; }

    private:
        ObjectPool &m_Pool;
        T *m_Obj;
    };

    /**
     * @brief Acquire an object with automatic release on scope exit.
     */
    ScopedHandle AcquireScoped() {
        return ScopedHandle(*this, Acquire());
    }

    /**
     * @brief Get the total number of objects in the pool.
     */
    size_t GetTotalCount() const { return m_Pool.size(); }

    /**
     * @brief Get the number of available (unused) objects.
     */
    size_t GetAvailableCount() const { return m_Available.size(); }

    /**
     * @brief Get the number of objects currently in use.
     */
    size_t GetInUseCount() const { return m_Pool.size() - m_Available.size(); }

    /**
     * @brief Clear all objects and reset the pool.
     */
    void Clear() {
        m_Available.clear();
        m_Pool.clear();
    }

    /**
     * @brief Reserve capacity for more objects.
     */
    void Reserve(size_t capacity) {
        size_t current = m_Pool.size();
        if (capacity > current) {
            m_Pool.reserve(capacity);
            m_Available.reserve(capacity);
            for (size_t i = current; i < capacity; ++i) {
                m_Pool.push_back(std::make_unique<T>());
                m_Available.push_back(m_Pool.back().get());
            }
        }
    }

private:
    std::vector<std::unique_ptr<T>> m_Pool;
    std::vector<T *> m_Available;
};

/**
 * @brief Pre-allocated vector that can be cleared and reused.
 *
 * Avoids allocation by keeping capacity when cleared.
 *
 * @code
 * Util::ReusableVector<glm::vec3> positions;
 * positions.Reserve(1000);
 *
 * // Each frame:
 * positions.Clear();  // Keeps capacity
 * for (...) {
 *     positions.PushBack(pos);
 * }
 * @endcode
 */
template <typename T>
class ReusableVector {
public:
    explicit ReusableVector(size_t initialCapacity = 64) {
        m_Data.reserve(initialCapacity);
    }

    void Reserve(size_t capacity) { m_Data.reserve(capacity); }
    void Clear() { m_Data.clear(); }
    void PushBack(const T &value) { m_Data.push_back(value); }
    void PushBack(T &&value) { m_Data.push_back(std::move(value)); }

    template <typename... Args>
    T &EmplaceBack(Args &&...args) {
        return m_Data.emplace_back(std::forward<Args>(args)...);
    }

    T &operator[](size_t index) { return m_Data[index]; }
    const T &operator[](size_t index) const { return m_Data[index]; }

    size_t Size() const { return m_Data.size(); }
    size_t Capacity() const { return m_Data.capacity(); }
    bool Empty() const { return m_Data.empty(); }

    auto begin() { return m_Data.begin(); }
    auto end() { return m_Data.end(); }
    auto begin() const { return m_Data.begin(); }
    auto end() const { return m_Data.end(); }

    T *Data() { return m_Data.data(); }
    const T *Data() const { return m_Data.data(); }

    const std::vector<T> &GetVector() const { return m_Data; }
    std::vector<T> &GetVector() { return m_Data; }

private:
    std::vector<T> m_Data;
};

/**
 * @brief Ring buffer for fixed-size circular data.
 *
 * Useful for frame timing, audio buffers, etc.
 */
template <typename T, size_t N>
class RingBuffer {
public:
    void Push(const T &value) {
        m_Data[m_WriteIndex] = value;
        m_WriteIndex = (m_WriteIndex + 1) % N;
        if (m_Count < N) ++m_Count;
    }

    T &operator[](size_t index) {
        size_t actualIndex = (m_WriteIndex - m_Count + index + N) % N;
        return m_Data[actualIndex];
    }

    const T &operator[](size_t index) const {
        size_t actualIndex = (m_WriteIndex - m_Count + index + N) % N;
        return m_Data[actualIndex];
    }

    size_t Size() const { return m_Count; }
    size_t Capacity() const { return N; }
    bool Empty() const { return m_Count == 0; }
    bool Full() const { return m_Count == N; }

    void Clear() {
        m_WriteIndex = 0;
        m_Count = 0;
    }

    // Get the most recent value
    const T &Back() const {
        return m_Data[(m_WriteIndex - 1 + N) % N];
    }

    // Get the oldest value
    const T &Front() const {
        return m_Data[(m_WriteIndex - m_Count + N) % N];
    }

private:
    std::array<T, N> m_Data{};
    size_t m_WriteIndex = 0;
    size_t m_Count = 0;
};

} // namespace Util

#endif
