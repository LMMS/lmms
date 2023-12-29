/*
 * ClapState.cpp - Implements CLAP state extension
 *
 * Copyright (c) 2023 Dalton Messmer <messmer.dalton/at/gmail.com>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "ClapState.h"

#ifdef LMMS_HAVE_CLAP

#include <string>
#include <cstring>
#include <cassert>

#include "ClapInstance.h"
#include "base64.h"

namespace lmms
{

auto ClapState::hostExt() const -> const clap_host_state*
{
	static clap_host_state ext {
		&clapMarkDirty
	};
	return &ext;
}

auto ClapState::checkSupported(const clap_plugin_state& ext) -> bool
{
	return ext.load && ext.save;
}

auto ClapState::load(std::string_view base64) -> bool
{
	assert(ClapThreadCheck::isMainThread());

	if (!supported()) { return false; }

	class IStream
	{
	public:
		IStream(std::string_view base64)
			: m_state{QByteArray::fromBase64(QByteArray::fromRawData(base64.data(), base64.size()))}
		{
		}

		//! Implements clap_istream.read
		static auto clapRead(const clap_istream* stream, void* buffer, std::uint64_t size) -> std::int64_t
		{
			if (!stream || !buffer || size == 0) { return -1; }
			auto self = static_cast<IStream*>(stream->ctx);
			if (!self) { return -1; }

			const auto bytesLeft = self->m_state.size() - self->m_readPos;
			const auto readAmount = std::min<std::uint64_t>(bytesLeft, size);

			auto ptr = static_cast<char*>(buffer);
			std::memcpy(ptr, self->m_state.data() + self->m_readPos, readAmount);
			self->m_readPos += readAmount;

			return bytesLeft > size ? readAmount : 0;
		}
	private:
		QByteArray m_state; //!< unencoded state data
		std::uint64_t m_readPos = 0;
	} stream{base64};

	auto clapStream = clap_istream {
		&stream,
		&IStream::clapRead
	};

	if (!pluginExt()->load(plugin(), &clapStream))
	{
		if (auto h = fromHost(host()))
		{
			h->log(CLAP_LOG_WARNING, "Plugin failed to load its state");
		}
		return false;
	}

	m_dirty = false;

	return true;
}

auto ClapState::load() -> bool
{
	return load(encodedState());
}

auto ClapState::save() -> std::optional<std::string_view>
{
	assert(ClapThreadCheck::isMainThread());

	if (!supported()) { return std::nullopt; }

	struct OStream
	{
		//! Implements clap_ostream.write
		static auto clapWrite(const clap_ostream* stream, const void* buffer, std::uint64_t size) -> std::int64_t
		{
			if (!stream || !buffer) { return -1; }
			auto self = static_cast<OStream*>(stream->ctx);
			if (!self) { return -1; }
			if (size == 0) { return 0; }

			auto ptr = static_cast<const char*>(buffer);
			QString base64;
			base64::encode(ptr, size, base64);

			self->state += base64.toStdString();
			return size;
		}

		std::string state;
	} stream;

	auto clapStream = clap_ostream {
		&stream,
		&OStream::clapWrite
	};

	if (!pluginExt()->save(plugin(), &clapStream))
	{
		instance()->log(CLAP_LOG_WARNING, "Plugin failed to save its state");
		return std::nullopt;
	}

	m_state = stream.state;
	m_dirty = false;

	return encodedState();
}

void ClapState::clapMarkDirty(const clap_host* host)
{
	assert(ClapThreadCheck::isMainThread());
	auto h = fromHost(host);
	if (!h) { return; }
	auto& state = h->state();

	if (!state.supported())
	{
		h->log(CLAP_LOG_PLUGIN_MISBEHAVING, "Plugin called clap_host_state.set_dirty() but the plugin does not "
			"provide a complete clap_plugin_state interface.");
		return;
	}

	state.m_dirty = true;
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
