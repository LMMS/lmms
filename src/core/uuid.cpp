#include "uuid.h"
#include <QUuid>

namespace lmms
{


auto Uuid::NullUuid() -> uuid_t
{
	return QUuid{};
}


auto Uuid::RandomUuid() -> uuid_t
{
	return QUuid::createUuid();
}

auto Uuid::AsString(uuid_t const& uuid) -> std::string
{
	return uuid.toString().toStdString();
}

auto Uuid::FromString(std::string_view const& string) -> uuid_t
{
	return QUuid{QString::fromStdString(
			std::string{string})};
}

bool Uuid::IsValid(uuid_t const& uuid)
{
	return !uuid.isNull();
}


} // namespace lmms