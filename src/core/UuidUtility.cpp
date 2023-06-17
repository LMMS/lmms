#include "UuidUtility.h"
#include <uuid.h>

namespace lmms
{

auto Uuid::NullUuid() -> uuid_t
{
	return uuid_t{};
}


auto Uuid::RandomUuid() -> uuid_t
{
	std::random_device rnd;
	auto seed = std::array<int, std::mt19937::state_size>{};
	std::generate(std::begin(seed), std::end(seed), std::ref(rnd));
	auto seq = std::seed_seq(std::begin(seed), std::end(seed));
	auto stdGenerator = std::mt19937(seq);

	auto gen = uuids::uuid_random_generator{stdGenerator};
	return gen();
}

auto Uuid::AsString(uuid_t const& uuid) -> std::string
{
	return uuids::to_string(uuid);
}

auto Uuid::FromString(std::string_view const& string) -> uuid_t
{
	auto id = uuids::uuid::from_string(string);
	if (!id.has_value())
	{
		throw std::invalid_argument("Malformed UUID.");
	}
	return id.value();
}

bool Uuid::IsValid(uuid_t const& uuid)
{
	return !uuid.is_nil();
}


} // namespace lmms