#ifndef LMMS_SHARED_RESOURCE_H
#define LMMS_SHARED_RESOURCE_H

#include <memory>
#include <type_traits>

namespace lmms {

/**
 * @brief Represents a shared, immutable, non-nullable resource.
 *
 * SharedResource is a object that represents a shared, immutable resource. It is non-null, meaning that the object can
 * always be acessed safely when needed. SharedResource also has a separate loaded state so that callers know when the
 * object has been initialized with actual data. This allows for distinguishing between:
 *  1. A resource that has not been loaded, but can still be accessed safely.
 *  2. A resource that has been loaded with actual data.
 *
 * @tparam T The type of resource. Must be default constructible.
 */
template <typename T>
	requires(std::is_default_constructible_v<T>)
class SharedResource
{
public:
	/**
	 * @brief Constructs an unloaded resource.
	 */
	SharedResource() = default;

	/**
	 * @brief Converting constructor for initializing a SharedResource with actual data.
	 * After construction, loaded() returns true.
	 *
	 * @param resource The resource to initialze with.
	 * @note If @a resource is null, an @c std::runtime_error will be thrown.
	 */
	SharedResource(std::unique_ptr<T> resource)
		: m_resource{std::move(resource)}
		, m_loaded{true}
	{
		if (!m_resource) { throw std::runtime_error{"Resource cannot be null"}; }
	}

	/**
	 * @brief Converting constructor for initializing a SharedResource with actual data.
	 * After construction, loaded() returns true.
	 *
	 * @param resource The resource to initialze with.
	 * @note If @a resource is null, an @c std::runtime_error will be thrown.
	 */
	SharedResource(std::shared_ptr<const T> resource)
		: m_resource{std::move(resource)}
		, m_loaded{true}
	{
		if (!m_resource) { throw std::runtime_error{"Resource cannot be null"}; }
	}

	/**
	 * @brief Dereference the shared resource.
	 * This is always a valid operation.
	 */
	auto operator*() const -> const T& { return *m_resource; }

	/**
	 * @brief Get a pointer to the shared resource.
	 * This is always a valid operation.
	 */
	auto get() const -> const T* { return m_resource.get(); }

	/**
	 * @brief Get a pointer to the shared resource.
	 * This is always a valid operation.
	 */
	auto operator->() const -> const T* { return m_resource.get(); }

	/**
	 * @brief Check to see if the shared resource has been loaded with actual data or not.
	 *
	 * @return true If the shared resource is initialized with actual data.
	 * @return false If the shared resource is not initialized with data, but default-constructed.
	 */
	auto loaded() const -> bool { return m_loaded; }

private:
	static auto empty() -> std::shared_ptr<const T>
	{
		static auto s_resource = std::make_shared<const T>();
		return s_resource;
	}

	std::shared_ptr<const T> m_resource = empty();
	bool m_loaded = false;
};
} // namespace lmms

#endif // LMMS_SHARED_RESOURCE_H