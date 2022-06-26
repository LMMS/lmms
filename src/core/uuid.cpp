#include "uuid.h"

namespace lmms
{


auto UUID::NullUuid() -> uuid_t
{
	return QUuid{};
}


auto UUID::RandomUuid() -> uuid_t
{
	return QUuid::createUuid();
}

auto UUID::AsString(uuid_t const& uuid) -> std::string
{
	return uuid.toString().toStdString();
}

bool UUID::IsValid(uuid_t const& uuid)
{
	return !uuid.isNull();
}


} // namespace lmms