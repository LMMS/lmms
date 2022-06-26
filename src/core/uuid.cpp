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

auto UUID::FromString(std::string_view const& string) -> uuid_t
{
	return QUuid::fromString(
		QString::fromStdString(
			std::string{string}));
}

bool UUID::IsValid(uuid_t const& uuid)
{
	return !uuid.isNull();
}


} // namespace lmms