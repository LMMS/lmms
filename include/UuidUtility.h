#ifndef LMMS_UUIDUTILITY_H
#define LMMS_UUIDUTILITY_H

#include <string>

/* NOTE: You're wondering why I can't directly include the library header here?
 * The answer is the RemoteVstPlugin target, which is categorized as an External
 * Project in cmake, and thus doesn't see the same include directories as the
 * rest of LMMS.
 * Thus, we only predeclare the UUID type here so clients of lmms_basics.h still
 * have access to this utility class, and then include <uuid.h> where needed.
 * If you have a better solution to this problem, please submit a pull request!
*/
namespace uuids
{
	class uuid;
}

namespace lmms
{


/**
 * @brief Utility class for creating UUIDs.
 */
class Uuid
{
public:
	using uuid_t = uuids::uuid;

	/**
	 * @brief Creates a null (invalid) UUID.
	 * @return The null UUID {00000000-0000-0000-0000-000000000000}
	 */
	static uuid_t NullUuid();

	/**
	 * @brief Creates a random UUID using a standard library random number
	 * generator.
	 * @return A random UUID with only infinitesimal chance of collision.
	 */
	static uuid_t RandomUuid();

	/**
	 * @brief Create a string representation of the given UUID.
	 * @return A UUID string representation. No particular format is guaranteed.
	 */
	static std::string AsString(uuid_t const& uuid);


	/**
	 * @brief Create a UUID from a string. Throws std::invalid_argument if
	 * the string is malformed.
	 * @return A UUID that's equivalent to the given string representation.
	 */
	static uuid_t FromString(std::string_view const& string);


	/**
	 * @brief Returns whether the given UUID is non-null.
	 * @return true if the given UUID is valid.
	 */
	static bool IsValid(uuid_t const& uuid);

};


} // namespace lmms

#endif //LMMS_UUIDUTILITY_H
