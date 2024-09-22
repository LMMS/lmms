/*
 * Lv2ViewBase.cpp - base class for Lv2 plugin views
 *
 * Copyright (c) 2018-2024 Johannes Lorenz <jlsf2013$users.sourceforge.net, $=@>
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

#include "Lv2ViewBase.h"

#ifdef LMMS_HAVE_LV2

#include <QGridLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTimer>
#include <QLabel>
#include <lilv/lilv.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/data-access/data-access.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/port-props/port-props.h>

#include "AudioEngine.h"
#include "Controls.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "embed.h"
#include "FontHelper.h"
#include "lmms_math.h"
#include "Lv2ControlBase.h"
#include "Lv2Manager.h"
#include "Lv2Proc.h"
#include "Lv2Ports.h"
#include "MainWindow.h"
#include "SubWindow.h"


namespace lmms::gui
{


class Timer : public QTimer
{
public:
	explicit Timer(Lv2ViewProc* viewProc)
	: QTimer(viewProc)
	, m_viewProc(viewProc)
	{}
	
	void timerEvent(QTimerEvent*) override
	{
		if (m_viewProc) { m_viewProc->update(); }
	}

private:
	Lv2ViewProc* m_viewProc = nullptr;
};




void Lv2ViewProc::uiPortEvent(
	uint32_t port_index,
	uint32_t buffer_size,
	uint32_t protocol,
	const void* buffer)
{
#ifdef LMMS_HAVE_SUIL
	if (m_uiInstance)
	{
		suil_instance_port_event(m_uiInstance, port_index, buffer_size, protocol, buffer);
	}
	else
#endif
	{
		// LMMS UI is updated automatically
	}
}




void Lv2ViewProc::initUi()
{
	// Set initial control port values
	int i = 0;
	proc()->foreach_port([&i,this](Lv2Ports::PortBase* port){
		auto ctrl = Lv2Ports::dcast<Lv2Ports::Control>(port);
		if (ctrl)
		{
			//qDebug() << "Setting port" << i << "to" << ctrl->m_val;
			uiPortEvent(i, sizeof(float), 0, &ctrl->m_val);
		}
		i++;
	});
}




// write UI event to ui_events (to be read by plugin)
void Lv2ViewProc::writeToPlugin(uint32_t port_index,
	uint32_t buffer_size,
	uint32_t protocol,
	const void* buffer)
{
	Lv2Manager* mgr = Engine::getLv2Manager();
	
	if (protocol != 0 && protocol != m_atomEventTransfer)
	{
		qWarning(
			"UI write with unsupported protocol %u (%s), should be %u (%s) or 0",
			protocol,
			mgr->uridMap().unmap(protocol),
			m_atomEventTransfer,
			mgr->uridMap().unmap(m_atomEventTransfer)
			);
		return;
	}

	if (port_index >= proc()->portNum())
	{
		qWarning() << "UI write to out of range port index" << port_index;
		return;
	}

#if defined(LMMS_HAVE_SERD) && defined(LMMS_HAVE_SRATOM)
	if (lv2Dump && protocol == m_atomEventTransfer)
	{
		const LV2_Atom* atom = (const LV2_Atom*)buffer;
		mgr->uridMap().dump();
		std::unique_ptr<char, decltype(&std::free)> str(
			sratom_to_turtle(
				mgr->sratom, mgr->uridMap().unmapFeature(),
				"lmms:", nullptr, nullptr,
				atom->type, atom->size, LV2_ATOM_BODY_CONST(atom)),
			&std::free);
		qDebug("\n## UI => Plugin (%u bytes) ##\n%s\n", atom->size, str.get());
	}
#endif
	
	char buf[Lv2Proc::uiEventsBufsize()];
	Lv2UiControlChange* ev = (Lv2UiControlChange*)buf;
	ev->index = port_index;
	ev->protocol = protocol;
	ev->size = buffer_size;
	memcpy(ev + 1, buffer, buffer_size);
	m_uiEvents.write(buf, sizeof(Lv2UiControlChange) + buffer_size);
}




const char* Lv2ViewProc::hostUiTypeUri() { return LV2_UI__Qt5UI; }




bool Lv2ViewProc::calculateIsResizable() const
{
	if (!m_ui) { return false; }
	
	Lv2Manager* mgr = Engine::getLv2Manager();
	
	const LilvNode* s = lilv_ui_get_uri(m_ui);
	AutoLilvNode p = mgr->uri(LV2_CORE__optionalFeature);
	AutoLilvNode fs = mgr->uri(LV2_UI__fixedSize);
	AutoLilvNode nrs = mgr->uri(LV2_UI__noUserResize);
	
	AutoLilvNodes fs_matches  = mgr->findNodes(s, p.get(), fs.get());
	AutoLilvNodes nrs_matches = mgr->findNodes(s, p.get(), nrs.get());
	
	return !fs_matches && !nrs_matches;
}




std::tuple<const LilvUI*, const LilvNode*> Lv2ViewProc::selectPluginUi(LilvUIs* uis) const
{
#ifdef LMMS_HAVE_SUIL
	{
		// Try to find an embeddable UI
		AutoLilvNode hostUi = uri(hostUiTypeUri());
		qDebug() << "Searching for plugin UI matching Host UI" << hostUiTypeUri() << "...";

		LILV_FOREACH (uis, u, uis)
		{
			const LilvUI* pluginUi = lilv_uis_get(uis, u);
			const LilvNode* pluginUiType = nullptr;
			const bool supported = lilv_ui_is_supported(pluginUi, suil_ui_supported, hostUi.get(), &pluginUiType);
			if (supported)
			{
				if (pluginUiType)
				{
					qDebug() << "Found supported plugin UI" << lilv_node_as_uri(pluginUiType);
				}
				return std::make_tuple(pluginUi, pluginUiType);
			}
			else
			{
				qDebug() << "Skipping plugin UI" << lilv_node_as_uri(lilv_ui_get_uri(pluginUi)) << "- not supported";
			}
		}
		qDebug() << "... no matching plugin UI found. Defaulting to LMMS UI.";
	}
#endif
	return std::make_tuple(nullptr, nullptr);
}




Lv2ViewProc::Lv2ViewProc(QWidget* parent, Lv2Proc* proc, int colNum) :
	LinkedModelGroupView (parent, proc, colNum),
	m_uiEvents(Lv2Proc::uiMidiBufsize() * Lv2Proc::uiNBufferCycles())
#ifdef LMMS_HAVE_LV2_1_17_2
	, m_requestValue { this, [] (LV2UI_Feature_Handle handle,
								LV2_URID key,
								LV2_URID type,
								const LV2_Feature* const* features)
								-> LV2UI_Request_Value_Status
								{ return static_cast<Lv2ViewProc*>(handle)->
									requestValue(key, type, features); } }
#endif
{
#ifdef LMMS_HAVE_SUIL
	if (Lv2Manager::wantUi())
	{
		// User wants UI and we support it in general
		// Select a suiting UI type.
		// Note: It may be possible there is no suiting UI type.
		std::tie(m_ui, m_uiType) = selectPluginUi(proc->getUis());
	}
#endif

#ifdef LMMS_HAVE_SUIL
	if (m_ui)
	{
		//LinkedModelGroupView::hide();

		Lv2Manager* mgr = Engine::getLv2Manager();
		m_atomEventTransfer = mgr->uridMap().map(LV2_ATOM__eventTransfer);
		m_uiHost = suil_host_new(
			[](void* const h, uint32_t port, uint32_t bs, uint32_t pro, const void* buf)
			{
				static_cast<Lv2ViewProc*>(h)->writeToPlugin(port, bs, pro, buf);
			},
			[](void* const c, const char* symbol) -> uint32_t
			{
				std::size_t portId = static_cast<Lv2ViewProc*>(c)->proc()->getIdOfPort(symbol);
				return (portId == (std::size_t)-1 ? LV2UI_INVALID_PORT_INDEX : portId);
			},
			nullptr, nullptr);
		Q_ASSERT(m_uiHost);
		suil_host_set_touch_func(m_uiHost,
			[](void* controller, uint32_t port_index, bool grabbed)
			{
				static_cast<Lv2ViewProc*>(controller)->touch(port_index, grabbed);
			});
		
		const LV2_Feature parentFeature = {LV2_UI__parent, parent};
		const LV2_Feature instanceFeature = {
			LV2_INSTANCE_ACCESS_URI,
			lilv_instance_get_handle(proc->getInstanceForInstanceFeatureOnly())};
		const LV2_Feature dataFeature = {LV2_DATA_ACCESS_URI,
											proc->extdataFeature()};
#ifdef LMMS_HAVE_LV2_1_17_2
		const LV2_Feature requestValueFeature = {LV2_UI__requestValue,
												&m_requestValue};
#endif

		const LV2_Feature* uiFeatures[] = {
			proc->mapFeature(),
			proc->unmapFeature(),
			&instanceFeature,
			&dataFeature,
			&parentFeature,
			proc->optionsFeature(),
#ifdef LMMS_HAVE_LV2_1_17_2
			&requestValueFeature,
#endif
			nullptr};
		
		const char* bundleUri  = lilv_node_as_uri(lilv_ui_get_bundle_uri(m_ui));
		const char* binaryUri  = lilv_node_as_uri(lilv_ui_get_binary_uri(m_ui));
		auto bundlePath = AutoLilvPtr<char>(lilv_file_uri_parse(bundleUri, NULL));
		auto binaryPath = AutoLilvPtr<char>(lilv_file_uri_parse(binaryUri, NULL));

		m_uiInstance =
			suil_instance_new(m_uiHost,
				this,
				hostUiTypeUri(),
				proc->pluginUri(),
				lilv_node_as_uri(lilv_ui_get_uri(m_ui)),
				lilv_node_as_uri(m_uiType),
				bundlePath.get(),
				binaryPath.get(),
				uiFeatures);
		if (!m_uiInstance)  // TODO: fallback to default UI
		{
			qWarning() << "Failed to load plugin UI";
		}
		else
		{
			qDebug() << "Created new plugin UI" << lilv_node_as_uri(lilv_ui_get_uri(m_ui)) << lilv_node_as_uri(m_uiType);
		}

		m_uiInstanceWidget = static_cast<QWidget*>(suil_instance_get_widget(m_uiInstance));
		assert(m_uiInstanceWidget);
		//m_uiInstanceWidget->setParent(this);
		//layout()->addWidget(m_uiInstanceWidget);
		m_uiInstanceWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
		layout()->addWidget(m_uiInstanceWidget);
		layout()->setSizeConstraint(QLayout::SetNoConstraint);

		// connect UI ringbuffers
		// about the order: Lv2Proc can reject sending plugin events as long as
		//                  its UI reader is not connected
		proc->connectToPluginEvents(m_pluginEventsReader);
		proc->connectUiEventsReaderTo(m_uiEvents);

		// <--- This marks the entry point after which the UI can be used

		initUi();  // send all control values to UI once

		m_isResizable = calculateIsResizable();
		if (m_isResizable)
		{
			const QSizePolicy expanding(QSizePolicy::Expanding, QSizePolicy::Expanding);
			m_uiInstanceWidget->setSizePolicy(expanding);
			setSizePolicy(expanding);
			parent->setSizePolicy(expanding);
		}
		qDebug() << "Plugin UI" << (m_isResizable ? "is" : "is not") << "resizable";

		m_timer = new Timer(this);
		m_timer->start(1000 / lv2UiRefreshRate());
	}
	else
#endif // LMMS_HAVE_SUIL
	{
		class SetupTheWidget : public Lv2Ports::ConstVisitor
		{
		public:
			QWidget* m_parent; // input
			const LilvNode* m_commentUri; // input
			Control* m_control = nullptr; // output
			void visit(const Lv2Ports::Control& port) override
			{
				if (port.m_flow == Lv2Ports::Flow::Input)
				{
					using PortVis = Lv2Ports::Vis;
	
					switch (port.m_vis)
					{
						case PortVis::Generic:
							m_control = new KnobControl(m_parent);
							break;
						case PortVis::Integer:
						{
							sample_rate_t sr = Engine::audioEngine()->outputSampleRate();
							auto pMin = port.min(sr);
							auto pMax = port.max(sr);
							int numDigits = std::max(numDigitsAsInt(pMin), numDigitsAsInt(pMax));
							m_control = new LcdControl(numDigits, m_parent);
							break;
						}
						case PortVis::Enumeration:
							m_control = new ComboControl(m_parent);
							break;
						case PortVis::Toggled:
							m_control = new CheckControl(m_parent);
							break;
					}
					m_control->setText(port.name());
	
					AutoLilvNodes props(lilv_port_get_value(
						port.m_plugin, port.m_port, m_commentUri));
					LILV_FOREACH(nodes, itr, props.get())
					{
						const LilvNode* nod = lilv_nodes_get(props.get(), itr);
						m_control->topWidget()->setToolTip(lilv_node_as_string(nod));
						break;
					}
				}
			}
		};
	
		AutoLilvNode commentUri = uri(LILV_NS_RDFS "comment");
		proc->foreach_port(
			[this, &commentUri](const Lv2Ports::PortBase* port)
			{
				if (!lilv_port_has_property(port->m_plugin, port->m_port,
											uri(LV2_PORT_PROPS__notOnGUI).get()))
				{
					SetupTheWidget setup;
					setup.m_parent = this;
					setup.m_commentUri = commentUri.get();
					port->accept(setup);
	
					if (setup.m_control)
					{
						addControl(setup.m_control,
							lilv_node_as_string(lilv_port_get_symbol(
								port->m_plugin, port->m_port)),
							port->name().toUtf8().data(),
							false);
					}
				}
			}); // proc->foreach_port
	} // ! m_ui
}




Lv2ViewProc::~Lv2ViewProc()
{
	if (m_timer) { m_timer->stop(); }
}




QSize Lv2ViewProc::uiWidgetSize() const
{
#ifdef LMMS_HAVE_SUIL
	return m_uiInstanceWidget ? m_uiInstanceWidget->size() : QSize();
#else
	return QSize();
#endif
}




void Lv2ViewProc::update()
{
	if (!m_pluginEventsReader) { return; }

	// Emit UI events
	Lv2UiControlChange ev;
	const size_t space = m_pluginEventsReader->read_space();
	
	for (size_t i = 0; i + sizeof(ev) < space; i += sizeof(ev) + ev.size)
	{
		// Read event header to get the size
		if (space - i < sizeof(ev)) { break; }
		m_pluginEventsReader->read(sizeof(ev)).copy((char*)&ev, sizeof(ev));
		
		// Resize read buffer if necessary
		m_uiEventBuf.resize(ev.size);
		void* const buf = m_uiEventBuf.data();
		
		// Read event body
		if (space - i < ev.size)
		{
			qWarning() << "error: Error reading from UI ring buffer";
			break;
		}
		m_pluginEventsReader->read(ev.size).copy((char*)buf, ev.size);

#if defined(LMMS_HAVE_SERD) && defined(LMMS_HAVE_SRATOM)
		if (lv2Dump && ev.protocol == m_atomEventTransfer)
		{
			Lv2Manager* mgr = Engine::getLv2Manager();
			// Dump event in Turtle to the console
			const LV2_Atom* atom = (LV2_Atom*)buf;
			std::unique_ptr<char, decltype(&std::free)> str(
				sratom_to_turtle(
					mgr->ui_sratom, mgr->uridMap().unmapFeature(),
					"lmms:", nullptr, nullptr,
					atom->type, atom->size, LV2_ATOM_BODY(atom)),
				&std::free);
			qDebug("\n## Plugin => UI (%u bytes) ##\n%s\n", atom->size, str.get());
		}
#endif

		uiPortEvent(ev.index, ev.size, ev.protocol, buf);

		if (lv2Dump && ev.protocol == 0)
		{
			qDebug() << proc()->portname(ev.index).toLocal8Bit().data()
				<< " = " << *static_cast<float*>(buf);
		}

	}
}





AutoLilvNode Lv2ViewProc::uri(const char* uriStr)
{
	return Engine::getLv2Manager()->uri(uriStr);
}




#ifdef LMMS_HAVE_LV2_1_17_2
LV2UI_Request_Value_Status Lv2ViewProc::requestValue(
	LV2_URID /*key*/,
	LV2_URID /*type*/,
	const LV2_Feature* const* /*features*/)
{
	// This requires the patch extension
	// - which LMMS does not support at the moment
	return LV2UI_REQUEST_VALUE_ERR_UNSUPPORTED;
}
#endif




void Lv2ViewProc::touch(uint32_t portIndex, bool grabbed)
{
	qDebug() << "touch:" << portIndex << grabbed;
}




Lv2ViewBase::Lv2ViewBase(QWidget* meAsWidget, Lv2ControlBase *ctrlBase) :
	m_helpWindowEventFilter(this)
{
	auto grid = new QGridLayout(meAsWidget);

	auto btnBox = new QHBoxLayout();
	if (/* DISABLES CODE */ (false))
	{
		m_reloadPluginButton = new QPushButton(QObject::tr("Reload Plugin"),
			meAsWidget);
		btnBox->addWidget(m_reloadPluginButton, 0);
	}

	if (/* DISABLES CODE */ (false)) // TODO: check if the plugin has the UI extension
	{
		m_toggleUIButton = new QPushButton(QObject::tr("Show GUI"),
											meAsWidget);
		m_toggleUIButton->setCheckable(true);
		m_toggleUIButton->setChecked(false);
		m_toggleUIButton->setIcon(embed::getIconPixmap("zoom"));
		m_toggleUIButton->setFont(adjustedToPixelSize(m_toggleUIButton->font(), SMALL_FONT_SIZE));
		btnBox->addWidget(m_toggleUIButton, 0);
	}
	btnBox->addStretch(1);

	meAsWidget->setAcceptDrops(true);

	// note: the lifetime of C++ objects ends after the top expression in the
	// expression syntax tree, so the AutoLilvNode gets freed after the function
	// has been called
	AutoLilvNodes props(lilv_plugin_get_value(ctrlBase->getPlugin(),
				uri(LILV_NS_RDFS "comment").get()));
	LILV_FOREACH(nodes, itr, props.get())
	{
		const LilvNode* node = lilv_nodes_get(props.get(), itr);
		auto infoLabel = new QLabel(QString(lilv_node_as_string(node)).trimmed() + "\n");
		infoLabel->setWordWrap(true);
		infoLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

		m_helpButton = new QPushButton(QObject::tr("Help"));
		m_helpButton->setCheckable(true);
		btnBox->addWidget(m_helpButton);

		m_helpWindow = getGUI()->mainWindow()->addWindowedWidget(infoLabel);
		m_helpWindow->setSizePolicy(QSizePolicy::Expanding,
									QSizePolicy::Expanding);
		m_helpWindow->installEventFilter(&m_helpWindowEventFilter);
		m_helpWindow->setAttribute(Qt::WA_DeleteOnClose, false);
		m_helpWindow->hide();

		break;
	}

	if (m_reloadPluginButton || m_toggleUIButton || m_helpButton)
	{
		grid->addLayout(btnBox, Rows::ButtonRow, 0, 1, m_colNum);
	}
	else { delete btnBox; }

	m_procView = new Lv2ViewProc(meAsWidget, ctrlBase->control(0), m_colNum);
	grid->addWidget(m_procView, Rows::ProcRow, 0);
}




Lv2ViewBase::~Lv2ViewBase() {
	closeHelpWindow();
	// TODO: hide UI if required
}




void Lv2ViewBase::toggleUI()
{
}




void Lv2ViewBase::toggleHelp(bool visible)
{
	if (m_helpWindow)
	{
		if (visible) { m_helpWindow->show(); m_helpWindow->raise(); }
		else { m_helpWindow->hide(); }
	}
}




void Lv2ViewBase::closeHelpWindow()
{
	if (m_helpWindow) { m_helpWindow->close(); }
}




void Lv2ViewBase::modelChanged(Lv2ControlBase *ctrlBase)
{
	// reconnect models
	if (m_toggleUIButton)
	{
		m_toggleUIButton->setChecked(ctrlBase->hasGui());
	}

	LinkedModelGroupsView::modelChanged(ctrlBase);
}




AutoLilvNode Lv2ViewBase::uri(const char *uriStr)
{
	return Engine::getLv2Manager()->uri(uriStr);
}




void Lv2ViewBase::onHelpWindowClosed()
{
	m_helpButton->setChecked(true);
}




HelpWindowEventFilter::HelpWindowEventFilter(Lv2ViewBase* viewBase) :
	m_viewBase(viewBase) {}




bool HelpWindowEventFilter::eventFilter(QObject* , QEvent* event)
{
	if (event->type() == QEvent::Close)
	{
		m_viewBase->m_helpButton->setChecked(false);
		return true;
	}
	return false;
}


} // namespace lmms::gui

#endif // LMMS_HAVE_LV2
