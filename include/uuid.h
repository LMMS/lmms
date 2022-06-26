#ifndef LMMS_UUID_H
#define LMMS_UUID_H

#include <string>

class QUuid;

namespace lmms
{


/**
 * @brief Utility class for creating UUIDs.
 * TODO When we move QT out of core, the uuid_t type must be changed from QUuid to the new one.
 */
class UUID
{
public:

	using uuid_t = QUuid;

	/**
	 * @brief Creates a null (invalid) UUID.
	 * @return The null UUID {00000000-0000-0000-0000-000000000000}
	 */
	static uuid_t NullUuid();

	/**
	 * @brief Creates a random UUID for usage in a distributed computing environment.
	 * The way it's created may be platform-dependent.
	 * @return A random UUID with only infinitesimal chance of collision.
	 */
	static uuid_t RandomUuid();

	/**
	 * @brief Create a string representation of the given UUID.
	 * @return A UUID string representation. No particular format is guaranteed.
	 */
	static std::string AsString(uuid_t const& uuid);


	/**
	 * @brief Create a UUID from a string. If the string is malformed, this might throw an
	 * exception or return the null UUID.
	 * @return A UUID that's equivalent to the given string representation,
	 * or possibly the null UUID.
	 */
	static uuid_t FromString(std::string_view const& string);


	/**
	 * @brief Returns whether the given UUID is non-null.
	 * @return true if the given UUID is valid.
	 */
	static bool IsValid(uuid_t const& uuid);

};


} // namespace lmms

#endif //LMMS_UUID_H
