/*
 * VstEffectControls.cpp - controls for VST effect plugins
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QAction>
#include <QDomElement>
#include <QGridLayout>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>

#include "embed.h"
#include "CustomTextKnob.h"
#include "VstEffectControls.h"
#include "VstEffectControlDialog.h"
#include "VstEffect.h"
#include "VstPlugin.h"

#include "LocaleHelper.h"
#include "MainWindow.h"
#include "GuiApplication.h"
#include "SubWindow.h"
#include <QApplication>

namespace lmms
{


VstEffectControls::VstEffectControls( VstEffect * _eff ) :
	EffectControls( _eff ),
	m_effect( _eff ),
	m_subWindow( nullptr ),
	ctrHandle( nullptr ),
	lastPosInMenu (0),
	m_vstGuiVisible ( true )
//	m_presetLabel ( NULL )
{
}




VstEffectControls::~VstEffectControls()
{
	delete ctrHandle;
	ctrHandle = nullptr;
}




void VstEffectControls::loadSettings( const QDomElement & _this )
{
	//m_effect->closePlugin();
	//m_effect->openPlugin( _this.attribute( "plugin" ) );
	m_effect->m_pluginMutex.lock();
	if( m_effect->m_plugin != nullptr )
	{
		m_vstGuiVisible = _this.attribute( "guivisible" ).toInt();

		m_effect->m_plugin->loadSettings( _this );

		const QMap<QString, QString> & dump = m_effect->m_plugin->parameterDump();
		paramCount = dump.size();
		auto paramStr = std::array<char, 35>{};
		knobFModel.resize(paramCount);
		QStringList s_dumpValues;
		for( int i = 0; i < paramCount; i++ )
		{
			std::snprintf(paramStr.data(), paramStr.size(), "param%d", i);
			s_dumpValues = dump[paramStr.data()].split(":");

			knobFModel[i] = new FloatModel( 0.0f, 0.0f, 1.0f, 0.01f, this, QString::number(i) );
			knobFModel[i]->loadSettings(_this, paramStr.data());

			if( !( knobFModel[ i ]->isAutomated() ||
						knobFModel[ i ]->controllerConnection() ) )
			{
				knobFModel[ i ]->setValue(LocaleHelper::toFloat(s_dumpValues.at(2)));
				knobFModel[ i ]->setInitValue(LocaleHelper::toFloat(s_dumpValues.at(2)));
			}

			connect( knobFModel[i], &FloatModel::dataChanged, this,
				[this, i]() { setParameter( knobFModel[i] ); }, Qt::DirectConnection);
		}

	}
	m_effect->m_pluginMutex.unlock();
}




void VstEffectControls::setParameter( Model * action )
{
	int knobUNID = action->displayName().toInt();

	if ( m_effect->m_plugin != nullptr ) {
		m_effect->m_plugin->setParam( knobUNID, knobFModel[knobUNID]->value() );
	}
}




void VstEffectControls::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	_this.setAttribute( "plugin", m_effect->m_key.attributes["file"] );
	m_effect->m_pluginMutex.lock();
	if( m_effect->m_plugin != nullptr )
	{
		m_effect->m_plugin->saveSettings( _doc, _this );
		if (!knobFModel.empty()) {
			const QMap<QString, QString> & dump = m_effect->m_plugin->parameterDump();
			paramCount = dump.size();
			auto paramStr = std::array<char, 35>{};
			for( int i = 0; i < paramCount; i++ )
			{
				if (knobFModel[i]->isAutomated() || knobFModel[i]->controllerConnection()) {
					std::snprintf(paramStr.data(), paramStr.size(), "param%d", i);
					knobFModel[i]->saveSettings(_doc, _this, paramStr.data());
				}
			}
		}
	}
	m_effect->m_pluginMutex.unlock();
}




int VstEffectControls::controlCount()
{
	return m_effect->m_plugin != nullptr ? 1 : 0;
}



gui::EffectControlDialog* VstEffectControls::createView()
{
	auto dialog = new gui::VstEffectControlDialog( this );
	dialog->togglePluginUI( m_vstGuiVisible );
	return dialog;
}




void VstEffectControls::managePlugin()
{
	if ( m_effect->m_plugin != nullptr && m_subWindow == nullptr ) {
		auto tt = new gui::ManageVSTEffectView(m_effect, this);
		ctrHandle = (QObject *)tt;
	} else if (m_subWindow != nullptr) {
		if (m_subWindow->widget()->isVisible() == false ) {
			m_scrollArea->show();
			m_subWindow->show();
		} else {
			m_scrollArea->hide();
			m_subWindow->hide();
		}
	}
}





void VstEffectControls::savePreset()
{

	if ( m_effect->m_plugin != nullptr ) {
		m_effect->m_plugin->savePreset();
/*    		bool converted;
    		QString str = m_vi->m_plugin->currentProgramName().section("/", 0, 0);
     		if (str != "")
   			lastPosInMenu = str.toInt(&converted, 10) - 1;
		QWidget::update();*/
	}

}




void VstEffectControls::updateMenu()
{

	// get all presets -
	if ( m_effect->m_plugin != nullptr )
	{
		m_effect->m_plugin->loadProgramNames();
		///QWidget::update();

     		QString str = m_effect->m_plugin->allProgramNames();

    		QStringList list1 = str.split("|");

     		QMenu * to_menu = m_selPresetButton->menu();
    		to_menu->clear();

     		for (int i = 0; i < list1.size(); i++) {
				auto presetAction = new QAction(this);
				connect(presetAction, SIGNAL(triggered()), this, SLOT(selPreset()));

        		presetAction->setText(QString("%1. %2").arg(QString::number(i+1), list1.at(i)));
        		presetAction->setData(i);
			if (i == lastPosInMenu) {
        			presetAction->setIcon(embed::getIconPixmap( "sample_file", 16, 16 ));
			} else  presetAction->setIcon(embed::getIconPixmap( "edit_copy", 16, 16 ));
			to_menu->addAction( presetAction );
     		}

	}

}




void VstEffectControls::openPreset()
{

	if ( m_effect->m_plugin != nullptr ) {
		m_effect->m_plugin->openPreset();
    		bool converted;
    		QString str = m_effect->m_plugin->currentProgramName().section("/", 0, 0);
     		if (str != "")
   			lastPosInMenu = str.toInt(&converted, 10) - 1;
		//QWidget::update();
	}

}




void VstEffectControls::rollPreset()
{

	if ( m_effect->m_plugin != nullptr ) {
		m_effect->m_plugin->rotateProgram( 1 );
    		bool converted;
    		QString str = m_effect->m_plugin->currentProgramName().section("/", 0, 0);
     		if (str != "")
   			lastPosInMenu = str.toInt(&converted, 10) - 1;
		//QWidget::update();
	}
}




void VstEffectControls::rolrPreset()
{

	if ( m_effect->m_plugin != nullptr ) {
		m_effect->m_plugin->rotateProgram( -1 );
    		bool converted;
    		QString str = m_effect->m_plugin->currentProgramName().section("/", 0, 0);
     		if (str != "")
   			lastPosInMenu = str.toInt(&converted, 10) - 1;
		//QWidget::update();
	}
}




void VstEffectControls::selPreset()
{
	auto action = qobject_cast<QAction*>(sender());
	if (action && m_effect->m_plugin != nullptr)
	{
		lastPosInMenu = action->data().toInt();
		m_effect->m_plugin->setProgram(lastPosInMenu);
		// QWidget::update();
	}
}




void VstEffectControls::paintEvent( QPaintEvent * )
{

}



namespace gui
{


ManageVSTEffectView::ManageVSTEffectView( VstEffect * _eff, VstEffectControls * m_vi ) :
	m_effect( _eff )
{
#if QT_VERSION < 0x50C00
	// Workaround for a bug in Qt versions below 5.12,
	// where argument-dependent-lookup fails for QFlags operators
	// declared inside a namepsace.
	// This affects the Q_DECLARE_OPERATORS_FOR_FLAGS macro in Instrument.h
	// See also: https://codereview.qt-project.org/c/qt/qtbase/+/225348

	using ::operator|;
#endif

	m_vi2 = m_vi;
	widget = new QWidget();
        m_vi->m_scrollArea = new QScrollArea( widget );
	l = new QGridLayout( widget );

	m_vi->m_subWindow = getGUI()->mainWindow()->addWindowedWidget(nullptr, Qt::SubWindow |
			Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
	m_vi->m_subWindow->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	m_vi->m_subWindow->setFixedSize( 960, 300);
	m_vi->m_subWindow->setWidget(m_vi->m_scrollArea);
	m_vi->m_subWindow->setWindowTitle( _eff->m_plugin->name() + tr( " - VST parameter control" ) );
	m_vi->m_subWindow->setWindowIcon( PLUGIN_NAME::getIconPixmap( "logo" ) );
	m_vi->m_subWindow->setAttribute(Qt::WA_DeleteOnClose, false);


	l->setContentsMargins( 20, 10, 10, 10 );
	l->setVerticalSpacing( 10 );
	l->setHorizontalSpacing( 23 );

	m_syncButton = new QPushButton( tr( "VST sync" ), widget );
	connect( m_syncButton, SIGNAL( clicked() ), this,
							SLOT( syncPlugin() ) );

	l->addWidget( m_syncButton, 0, 0, 1, 2, Qt::AlignLeft );

	m_displayAutomatedOnly = new QPushButton( tr( "Automated" ), widget );
	connect( m_displayAutomatedOnly, SIGNAL( clicked() ), this,
							SLOT( displayAutomatedOnly() ) );

	l->addWidget( m_displayAutomatedOnly, 0, 1, 1, 2, Qt::AlignLeft );


	m_closeButton = new QPushButton( tr( "    Close    " ), widget );
	connect( m_closeButton, SIGNAL( clicked() ), this,
							SLOT( closeWindow() ) );

	l->addWidget( m_closeButton, 0, 2, 1, 7, Qt::AlignLeft );


	for( int i = 0; i < 10; i++ )
	{
		l->addItem( new QSpacerItem( 68, 45, QSizePolicy::Fixed, QSizePolicy::Fixed ), 0, i );
	}

	const QMap<QString, QString> & dump = m_effect->m_plugin->parameterDump();
	m_vi->paramCount = dump.size();

	vstKnobs = new CustomTextKnob *[ m_vi->paramCount ];

	bool hasKnobModel = true;
	if (m_vi->knobFModel.empty())
	{
		m_vi->knobFModel.resize(m_vi->paramCount);
		hasKnobModel = false;
	}

	auto paramStr = std::array<char, 35>{};
	QStringList s_dumpValues;

	for( int i = 0; i < m_vi->paramCount; i++ )
	{
		std::snprintf(paramStr.data(), paramStr.size(), "param%d", i);
		s_dumpValues = dump[paramStr.data()].split(":");

		const auto & description = s_dumpValues.at(1);

		auto knob = new CustomTextKnob(KnobType::Bright26, description.left(15), widget, description);
		knob->setDescription(description + ":");
		vstKnobs[i] = knob;

		if( !hasKnobModel )
		{
			std::snprintf(paramStr.data(), paramStr.size(), "%d", i);
			m_vi->knobFModel[i] = new FloatModel(LocaleHelper::toFloat(s_dumpValues.at(2)),
					0.0f, 1.0f, 0.01f, _eff, paramStr.data());
		}

		FloatModel * model = m_vi->knobFModel[i];
		connect( model, &FloatModel::dataChanged, this,
			[this, model]() { setParameter( model ); }, Qt::DirectConnection);
		vstKnobs[ i ] ->setModel( model );
	}
	syncParameterText();

	int i = 0;
	for( int lrow = 1; lrow < ( int( m_vi->paramCount / 10 ) + 1 ) + 1; lrow++ )
	{
		for( int lcolumn = 0; lcolumn < 10; lcolumn++ )
		{
			if( i < m_vi->paramCount )
			{
				l->addWidget( vstKnobs[i], lrow, lcolumn, Qt::AlignCenter );
			}
			i++;
		}
	}

	l->setRowStretch( ( int( m_vi->paramCount / 10 ) + 1 ), 1 );
	l->setColumnStretch( 10, 1 );

	widget->setLayout(l);
	widget->setAutoFillBackground(true);

	m_vi->m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	m_vi->m_scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_vi->m_scrollArea->setPalette( QApplication::palette( m_vi->m_scrollArea ) );
	m_vi->m_scrollArea->setMinimumHeight( 64 );

	m_vi->m_scrollArea->setWidget( widget );

	m_vi->m_subWindow->show();
}




void ManageVSTEffectView::closeWindow()
{
	m_vi2->m_subWindow->hide();
}




void ManageVSTEffectView::syncPlugin()
{
	auto paramStr = std::array<char, 35>{};
	QStringList s_dumpValues;
	const QMap<QString, QString> & dump = m_effect->m_plugin->parameterDump();

	for( int i = 0; i < m_vi2->paramCount; i++ )
	{
		// only not automated knobs are synced from VST
		// those auto-setted values are not jurnaled, tracked for undo / redo
		if( !( m_vi2->knobFModel[ i ]->isAutomated() ||
					m_vi2->knobFModel[ i ]->controllerConnection() ) )
		{
			std::snprintf(paramStr.data(), paramStr.size(), "param%d", i);
			s_dumpValues = dump[paramStr.data()].split(":");
			float f_value = LocaleHelper::toFloat(s_dumpValues.at(2));
			m_vi2->knobFModel[ i ]->setAutomatedValue( f_value );
			m_vi2->knobFModel[ i ]->setInitValue( f_value );
		}
	}
	syncParameterText();
}



void ManageVSTEffectView::displayAutomatedOnly()
{
	bool isAuto = QString::compare( m_displayAutomatedOnly->text(), tr( "Automated" ) ) == 0;

	for( int i = 0; i< m_vi2->paramCount; i++ )
	{

		if( !( m_vi2->knobFModel[ i ]->isAutomated() ||
					m_vi2->knobFModel[ i ]->controllerConnection() ) )
		{
			if( vstKnobs[ i ]->isVisible() == true  && isAuto )
			{
				vstKnobs[ i ]->hide();
				m_displayAutomatedOnly->setText( "All" );
			} else {
				vstKnobs[ i ]->show();
				m_displayAutomatedOnly->setText( "Automated" );
			}
		}
 	}
}




void ManageVSTEffectView::setParameter( Model * action )
{
	int knobUNID = action->displayName().toInt();

	if ( m_effect->m_plugin != nullptr ) {
		m_effect->m_plugin->setParam( knobUNID, m_vi2->knobFModel[knobUNID]->value() );
		syncParameterText();
	}
}

void ManageVSTEffectView::syncParameterText()
{
	m_effect->m_plugin->loadParameterLabels();
	m_effect->m_plugin->loadParameterDisplays();

	QString paramLabelStr   = m_effect->m_plugin->allParameterLabels();
	QString paramDisplayStr = m_effect->m_plugin->allParameterDisplays();

	QStringList paramLabelList;
	QStringList paramDisplayList;

	for( int i = 0; i < paramLabelStr.size(); )
	{
		const int length = paramLabelStr[i].digitValue();
		paramLabelList.append(paramLabelStr.mid(i + 1, length));
		i += length + 1;
	}

	for( int i = 0; i < paramDisplayStr.size(); )
	{
		const int length = paramDisplayStr[i].digitValue();
		paramDisplayList.append(paramDisplayStr.mid(i + 1, length));
		i += length + 1;
	}

	for( int i = 0; i < paramLabelList.size(); ++i )
	{
		vstKnobs[i]->setValueText(paramDisplayList[i] + ' ' + paramLabelList[i]);
	}
}



ManageVSTEffectView::~ManageVSTEffectView()
{
	if (!m_vi2->knobFModel.empty())
	{
		for( int i = 0; i < m_vi2->paramCount; i++ )
		{
			delete m_vi2->knobFModel[ i ];
			delete vstKnobs[ i ];
		}
	}

	if( vstKnobs != nullptr )
	{
		delete [] vstKnobs;
		vstKnobs = nullptr;
	}

	m_vi2->knobFModel.clear();

	if( m_vi2->m_scrollArea != nullptr )
	{
		delete m_vi2->m_scrollArea;
		m_vi2->m_scrollArea = nullptr;
	}

	if( m_vi2->m_subWindow != nullptr )
	{
		m_vi2->m_subWindow->setAttribute( Qt::WA_DeleteOnClose );
		m_vi2->m_subWindow->close();

		if( m_vi2->m_subWindow != nullptr )
		{
			delete m_vi2->m_subWindow;
		}
		m_vi2->m_subWindow = nullptr;
	}
	//delete m_vi2->m_subWindow;
	//m_vi2->m_subWindow = NULL;
}


} // namespace gui

} // namespace lmms
