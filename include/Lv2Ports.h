/*
 * Lv2Ports.h - Lv2 port definitions
 *
 * Copyright (c) 2019-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
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

#ifndef LV2PORTS_H
#define LV2PORTS_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_LV2

#include <lilv/lilv.h>
#include <memory>
#include <vector>

#include "lmms_basics.h"
#include "PluginIssue.h"

struct ConnectPorts;

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
	Event, //!< TODO: unused, describe
	Cv //!< TODO: unused, describe
};

//! Port visualization
//! @note All Lv2 audio ports are float, this is only the visualisation
enum class Vis {
	None,
	Integer,
	Enumeration,
	Toggled
};

const char* toStr(Lv2Ports::Flow pf);
const char* toStr(Lv2Ports::Type pt);
const char* toStr(Lv2Ports::Vis pv);


struct ConstVisitor
{
#define CAN_VISIT(clss) \
virtual void visit(const struct clss& ) {}

	CAN_VISIT(ControlPortBase)
	CAN_VISIT(Control)
	CAN_VISIT(Audio)
	CAN_VISIT(Cv)
	CAN_VISIT(Unknown)
	virtual ~ConstVisitor();
#undef CAN_VISIT
};

struct Visitor
{
#define CAN_VISIT(clss) \
virtual void visit(clss& ) {}

	CAN_VISIT(ControlPortBase)
	CAN_VISIT(Control)
	CAN_VISIT(Audio)
	CAN_VISIT(Cv)
	CAN_VISIT(Unknown)
	virtual ~Visitor();
#undef CAN_VISIT
};

struct Meta
{
	Type m_type = Type::Unknown;
	Flow m_flow = Flow::Unknown;
	Vis m_vis = Vis::None;

	float m_def = .0f, m_min = .0f, m_max = .0f;
	bool m_optional = false;
	bool m_used = true;

	std::vector<PluginIssue> get(const LilvPlugin* plugin, unsigned portNum);
};

struct PortBase : public Meta
{
	const LilvPort* m_port = nullptr;
	const LilvPlugin* m_plugin = nullptr;

	virtual void accept(Visitor& v) = 0;
	virtual void accept(ConstVisitor& v) const = 0;

	QString name() const;

	virtual ~PortBase();
};

#define IS_PORT_TYPE \
void accept(Visitor& v) override { v.visit(*this); } \
void accept(ConstVisitor& v) const override { v.visit(*this); }

struct ControlPortBase : public PortBase
{
	IS_PORT_TYPE

	//! LMMS models
	//! Always up-to-date, except during runs
	std::unique_ptr<class AutomatableModel> m_connectedModel;

	//! Enumerate float values
	//! lv2 defines scale points as
	//! "single float Points (for control inputs)"
	std::vector<float> m_scalePointMap;
};

struct Control : public ControlPortBase
{
	IS_PORT_TYPE

	//! Data location which Lv2 plugins see
	//! Model values are being copied here every run
	//! Between runs, this data is not up-to-date
	float m_val;
};

struct Cv : public ControlPortBase
{
	IS_PORT_TYPE

	//! Data location which Lv2 plugins see
	//! Model values are being copied here every run
	//! Between runs, this data is not up-to-date
	std::vector<float> m_buffer;
};

struct Audio : public PortBase
{
	IS_PORT_TYPE

	Audio(std::size_t bufferSize, bool isSidechain);

	//! Copy buffer passed by LMMS into our ports
	void copyBuffersFromCore(const sampleFrame *lmmsBuf,
		unsigned channel, fpp_t frames);
	//! Add buffer passed by LMMS into our ports, and halve the result
	void addBuffersFromCore(const sampleFrame *lmmsBuf,
		unsigned channel, fpp_t frames);
	//! Copy our ports into buffers passed by LMMS
	void copyBuffersToCore(sampleFrame *lmmsBuf,
		unsigned channel, fpp_t frames) const;

	bool isSideChain() const { return m_sidechain; }
	std::size_t bufferSize() const { return m_buffer.size(); }

private:
	std::vector<float> m_buffer;
	bool m_sidechain;

	// the only case when data of m_buffer may be referenced:
	friend struct ::ConnectPorts;
};

struct Unknown : public PortBase
{
	IS_PORT_TYPE
};
#undef IS_PORT_TYPE

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

#endif // LMMS_HAVE_LV2
#endif // LV2PORTS_H
