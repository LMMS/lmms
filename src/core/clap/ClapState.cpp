/*
 * ClapState.cpp - Implements CLAP state extension
 *
 * Copyright (c) 2024 Dalton Messmer <messmer.dalton/at/gmail.com>
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
#include <clap/ext/state-context.h>

#include "ClapInstance.h"
#include "base64.h"

namespace lmms
{

static_assert(static_cast<std::uint32_t>(ClapState::Context::Preset) == CLAP_STATE_CONTEXT_FOR_PRESET);
static_assert(static_cast<std::uint32_t>(ClapState::Context::Duplicate) == CLAP_STATE_CONTEXT_FOR_DUPLICATE);
static_assert(static_cast<std::uint32_t>(ClapState::Context::Project) == CLAP_STATE_CONTEXT_FOR_PROJECT);

auto ClapState::initImpl() noexcept -> bool
{
	m_stateContext = static_cast<const clap_plugin_state_context*>(
		plugin()->get_extension(plugin(), CLAP_EXT_STATE_CONTEXT));

	if (m_stateContext && (!m_stateContext->load || !m_stateContext->save))
	{
		m_stateContext = nullptr;
		logger().log(CLAP_LOG_WARNING, "State context extension is not fully implemented");
		return true;
	}

	if (m_stateContext)
	{
		logger().log(CLAP_LOG_INFO, "State context extension is supported");
	}

	return true;
}

void ClapState::deinitImpl() noexcept
{
	m_stateContext = nullptr;
}

auto ClapState::hostExtImpl() const -> const clap_host_state*
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

auto ClapState::load(std::string_view base64, Context context) -> bool
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
			if (!stream || !buffer || size == 0) { return -1; } // error
			auto self = static_cast<IStream*>(stream->ctx);
			if (!self) { return -1; } // error

			const auto bytesLeft = self->m_state.size() - self->m_readPos;
			if (bytesLeft == 0) { return 0; } // end of file
			const auto readAmount = std::min<std::uint64_t>(bytesLeft, size);

			auto ptr = static_cast<char*>(buffer);
			std::memcpy(ptr, self->m_state.data() + self->m_readPos, readAmount);
			self->m_readPos += readAmount;

			return readAmount;
		}
	private:
		QByteArray m_state; //!< unencoded state data
		std::uint64_t m_readPos = 0;
	} stream{base64};

	const auto clapStream = clap_istream {
		&stream,
		&IStream::clapRead
	};

	const auto success = m_stateContext && context != Context::None
		? m_stateContext->load(plugin(), &clapStream, static_cast<std::uint32_t>(context))
		: pluginExt()->load(plugin(), &clapStream);

	if (!success)
	{
		if (auto h = fromHost(host()))
		{
			h->logger().log(CLAP_LOG_WARNING, "Plugin failed to load its state");
		}
		return false;
	}

	m_dirty = false;

	return true;
}

auto ClapState::load(Context context) -> bool
{
	return load(encodedState(), context);
}

auto ClapState::save(Context context) -> std::optional<std::string_view>
{
	assert(ClapThreadCheck::isMainThread());

	if (!supported()) { return std::nullopt; }

	struct OStream
	{
		//! Implements clap_ostream.write
		static auto clapWrite(const clap_ostream* stream, const void* buffer, std::uint64_t size) -> std::int64_t
		{
			if (!stream || !buffer) { return -1; } // error
			auto self = static_cast<OStream*>(stream->ctx);
			if (!self) { return -1; } // error
			if (size == 0) { return 0; }

			const auto ptr = static_cast<const char*>(buffer);

			self->state.reserve(self->state.size() + size);
			for (std::uint64_t idx = 0; idx < size; ++idx)
			{
				self->state.push_back(ptr[idx]);
			}

			return size;
		}

		std::vector<char> state;
	} stream;

	const auto clapStream = clap_ostream {
		&stream,
		&OStream::clapWrite
	};

	const auto success = m_stateContext && context != Context::None
		? m_stateContext->save(plugin(), &clapStream, static_cast<std::uint32_t>(context))
		: pluginExt()->save(plugin(), &clapStream);

	if (!success)
	{
		logger().log(CLAP_LOG_WARNING, "Plugin failed to save its state");
		return std::nullopt;
	}

	QString base64;
	base64::encode(stream.state.data(), stream.state.size(), base64);

	m_state = base64.toStdString();
	m_dirty = false;

	return encodedState();
}

void ClapState::clapMarkDirty(const clap_host* host)
{
	auto h = fromHost(host);
	if (!h) { return; }
	auto& state = h->state();

	if (!ClapThreadCheck::isMainThread())
	{
		h->logger().log(CLAP_LOG_PLUGIN_MISBEHAVING, "Called state.mark_dirty() from wrong thread");
	}

	if (!state.supported())
	{
		h->logger().log(CLAP_LOG_PLUGIN_MISBEHAVING, "Plugin called clap_host_state.set_dirty()"
			" but the plugin does not provide a complete clap_plugin_state interface.");
		return;
	}

	state.m_dirty = true;
}

} // namespace lmms

#endif // LMMS_HAVE_CLAP
