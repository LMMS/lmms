/*
 * Lv2Ports.h - Lv2 port classes definitions
 *
 * Copyright (c) 2019-2020 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#ifndef LMMS_LV2PORTS_H
#define LMMS_LV2PORTS_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lilv/lilv.h>
#include <memory>
#include <vector>

#include "Flags.h"
#include "LmmsTypes.h"
#include "PluginIssue.h"


namespace lmms
{

class SampleFrame;

struct ConnectPortVisitor;
using LV2_Evbuf = struct LV2_Evbuf_Impl;

namespace Lv2Ports {

/*
	port structs
*/
enum class Flow {
	Unknown,
	Input,
	Output
};

enum class Type {
	Unknown,
	Control,
	Audio,
	AtomSeq,
	Cv //!< TODO: unused, describe
};

//! Port visualization
//! @note All Lv2 audio ports are float, this is only the visualisation
enum class Vis {
	Generic, //!< nothing specific, a generic knob or slider shall be used
	Integer, //!< counter
	Enumeration, //!< selection from enumerated values
	Toggled //!< boolean widget
};

const char* toStr(Lv2Ports::Flow pf);
const char* toStr(Lv2Ports::Type pt);
const char* toStr(Lv2Ports::Vis pv);

struct ControlPortBase;
struct Control;
struct Audio;
struct Cv;
struct AtomSeq;
struct Unknown;

struct ConstVisitor
{
	virtual void visit(const Lv2Ports::ControlPortBase& ) {}
	virtual void visit(const Lv2Ports::Control& ) {}
	virtual void visit(const Lv2Ports::Audio& ) {}
	virtual void visit(const Lv2Ports::Cv& ) {}
	virtual void visit(const Lv2Ports::AtomSeq& ) {}
	virtual void visit(const Lv2Ports::Unknown& ) {}

	virtual ~ConstVisitor() = default;
};

struct Visitor
{
	virtual void visit(Lv2Ports::ControlPortBase& ) {}
	virtual void visit(Lv2Ports::Control& ) {}
	virtual void visit(Lv2Ports::Audio& ) {}
	virtual void visit(Lv2Ports::Cv& ) {}
	virtual void visit(Lv2Ports::AtomSeq& ) {}
	virtual void visit(Lv2Ports::Unknown& ) {}

	virtual ~Visitor() = default;
};

struct Meta
{
	Type m_type = Type::Unknown;
	Flow m_flow = Flow::Unknown;
	Vis m_vis = Vis::Generic;

	bool m_logarithmic = false;

	bool m_optional = false;
	bool m_used = true;

	std::vector<PluginIssue> get(const LilvPlugin* plugin, std::size_t portNum);

	float def() const { return m_def; }
	float min(sample_rate_t sr) const { return m_sampleRate ? sr * m_min : m_min; }
	float max(sample_rate_t sr) const { return m_sampleRate ? sr * m_max : m_max; }
private:
	float m_def = .0f, m_min = .0f, m_max = .0f;
	bool m_sampleRate = false;
};

struct PortBase : public Meta
{
	const LilvPort* m_port = nullptr;
	const LilvPlugin* m_plugin = nullptr;

	virtual void accept(Visitor& v) = 0;
	virtual void accept(ConstVisitor& v) const = 0;

	QString name() const;
	QString uri() const;

	virtual ~PortBase() = default;
};

template<typename Derived, typename Base>
struct VisitablePort : public Base
{
	void accept(Visitor& v) override { v.visit(static_cast<Derived&>(*this)); }
	void accept(ConstVisitor& v) const override { v.visit(static_cast<const Derived&>(*this)); }
};

struct ControlPortBase : public VisitablePort<ControlPortBase, PortBase>
{
	//! LMMS models
	//! Always up-to-date, except during runs
	std::unique_ptr<class AutomatableModel> m_connectedModel;

	//! Enumerate float values
	//! lv2 defines scale points as
	//! "single float Points (for control inputs)"
	std::vector<float> m_scalePointMap;
};

struct Control : public VisitablePort<Control, ControlPortBase>
{
	//! Data location which Lv2 plugins see
	//! Model values are being copied here every run
	//! Between runs, this data is not up-to-date
	float m_val;
};

struct Cv : public VisitablePort<Cv, ControlPortBase>
{
	//! Data location which Lv2 plugins see
	//! Model values are being copied here every run
	//! Between runs, this data is not up-to-date
	std::vector<float> m_buffer;
};

struct Audio : public VisitablePort<Audio, PortBase>
{
	Audio(std::size_t bufferSize, bool isSidechain);

	//! Copy buffer passed by LMMS into our ports
	//! @param channel channel index into each sample frame
	void copyBuffersFromCore(const SampleFrame* lmmsBuf,
		unsigned channel, fpp_t frames);
	//! Add buffer passed by LMMS into our ports, and halve the result
	//! @param channel channel index into each sample frame
	void averageWithBuffersFromCore(const SampleFrame* lmmsBuf,
		unsigned channel, fpp_t frames);
	//! Copy our ports into buffers passed by LMMS
	//! @param channel channel index into each sample frame
	void copyBuffersToCore(SampleFrame* lmmsBuf,
		unsigned channel, fpp_t frames) const;

	bool isSideChain() const { return m_sidechain; }
	bool isOptional() const { return m_optional; }
	bool mustBeUsed() const { return !isSideChain() && !isOptional(); }
	std::size_t bufferSize() const { return m_buffer.size(); }

private:
	//! the buffer where Lv2 reads/writes the data from/to
	std::vector<float> m_buffer;
	bool m_sidechain;

	// the only case when data of m_buffer may be referenced:
	friend struct lmms::ConnectPortVisitor;
};

struct AtomSeq : public VisitablePort<AtomSeq, PortBase>
{
	enum class FlagType
	{
		None = 0,
		Midi = 1
	};
	Flags<FlagType> flags = FlagType::None;

	struct Lv2EvbufDeleter
	{
		void operator()(LV2_Evbuf* n);
	};
	using AutoLv2Evbuf = std::unique_ptr<LV2_Evbuf, Lv2EvbufDeleter>;
	AutoLv2Evbuf m_buf;
};

struct Unknown : public VisitablePort<Unknown, PortBase>
{
};

/*
	port casts
*/
template<class Target>
struct DCastVisitor : public Visitor
{
	Target* m_result = nullptr;
	void visit(Target& tar) { m_result = &tar; }
};

template<class Target>
struct ConstDCastVisitor : public ConstVisitor
{
	const Target* m_result = nullptr;
	void visit(const Target& tar) { m_result = &tar; }
};

//! If you don't want to use a whole visitor, you can use dcast
template<class Target>
Target* dcast(PortBase* base)
{
	DCastVisitor<Target> vis;
	base->accept(vis);
	return vis.m_result;
}

//! const overload
template<class Target>
const Target* dcast(const PortBase* base)
{
	ConstDCastVisitor<Target> vis;
	base->accept(vis);
	return vis.m_result;
}

} // namespace Lv2Ports


} // namespace lmms

#endif // LMMS_HAVE_LV2

#endif // LMMS_LV2PORTS_H
