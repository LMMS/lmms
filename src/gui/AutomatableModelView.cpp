/*
 * AutomatableModelView.cpp - implementation of AutomatableModelView
 *
 * Copyright (c) 2011-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QMenu>
#include <QMouseEvent>

#include "AutomatableModelView.h"
#include "AutomationClip.h"
#include "ControllerConnectionDialog.h"
#include "ControllerConnection.h"
#include "embed.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "StringPairDrag.h"
#include "Clipboard.h"
#include "Engine.h"

#include "AutomationEditor.h"


namespace lmms::gui
{

static float floatFromClipboard(bool* ok=nullptr);

AutomatableModelView::AutomatableModelView( Model* model, QWidget* _this ) :
	ModelView( model, _this ),
	m_conversionFactor( 1.0 )
{
	widget()->setAcceptDrops( true );
	widget()->setCursor( QCursor( embed::getIconPixmap( "hand" ), 3, 3 ) );
}

void AutomatableModelView::addDefaultActions( QMenu* menu )
{
	AutomatableModel* model = modelUntyped();

	auto amvSlots = new AutomatableModelViewSlots(this, menu);

	menu->addAction( embed::getIconPixmap( "reload" ),
						AutomatableModel::tr( "&Reset (%1%2)" ).
							arg( model->initValue<float>() * m_conversionFactor ).
							arg( m_unit ),
						model, SLOT(reset()));

	menu->addSeparator();
	menu->addAction( embed::getIconPixmap( "edit_copy" ),
						AutomatableModel::tr( "&Copy value (%1%2)" ).
							arg( model->value<float>() * m_conversionFactor ).
							arg( m_unit ),
						amvSlots, SLOT(copyToClipboard()));

	bool canPaste = true;
	const float valueToPaste = floatFromClipboard(&canPaste);
	const QString pasteDesc = canPaste ?
					AutomatableModel::tr( "&Paste value (%1%2)").
						arg( valueToPaste ).
						arg( m_unit )
					: AutomatableModel::tr( "&Paste value");
	QAction* pasteAction = menu->addAction( embed::getIconPixmap( "edit_paste" ),
						pasteDesc, amvSlots, SLOT(pasteFromClipboard()));
	pasteAction->setEnabled(canPaste);

	menu->addSeparator();

	menu->addAction(embed::getIconPixmap("automation"),
		AutomatableModel::tr("add automation point"),
		amvSlots,
		SLOT(addSongAutomationPoint()));
	menu->addAction(embed::getIconPixmap("automation"),
		AutomatableModel::tr("add automation point to new clip"),
		amvSlots,
		SLOT(addSongAutomationPointAndClip()));
	menu->addAction(embed::getIconPixmap("automation"),
		AutomatableModel::tr("remove closest automation point"),
		amvSlots,
		SLOT(editSongGlobalAutomation()));

	menu->addSeparator();

	menu->addAction( embed::getIconPixmap( "automation" ),
						AutomatableModel::tr( "Edit song-global automation" ),
							amvSlots,
							SLOT(editSongGlobalAutomation()));

	menu->addAction( QPixmap(),
						AutomatableModel::tr( "Remove song-global automation" ),
						amvSlots,
						SLOT(removeSongGlobalAutomation()));

	menu->addSeparator();

	if( model->hasLinkedModels() )
	{
		menu->addAction( embed::getIconPixmap( "edit-delete" ),
							AutomatableModel::tr( "Remove all linked controls" ),
							amvSlots, SLOT(unlinkAllModels()));
		menu->addSeparator();
	}

	QString controllerTxt;
	if( model->controllerConnection() )
	{
		Controller* cont = model->controllerConnection()->getController();
		if( cont )
		{
			controllerTxt = AutomatableModel::tr( "Connected to %1" ).arg( cont->name() );
		}
		else
		{
			controllerTxt = AutomatableModel::tr( "Connected to controller" );
		}

		QMenu* contMenu = menu->addMenu( embed::getIconPixmap( "controller" ), controllerTxt );

		contMenu->addAction( embed::getIconPixmap( "controller" ),
								AutomatableModel::tr("Edit connection..."),
								amvSlots, SLOT(execConnectionDialog()));
		contMenu->addAction( embed::getIconPixmap( "cancel" ),
								AutomatableModel::tr("Remove connection"),
								amvSlots, SLOT(removeConnection()));
	}
	else
	{
		menu->addAction( embed::getIconPixmap( "controller" ),
							AutomatableModel::tr("Connect to controller..."),
							amvSlots, SLOT(execConnectionDialog()));
	}
}




void AutomatableModelView::setModel( Model* model, bool isOldModelValid )
{
	ModelView::setModel( model, isOldModelValid );
}




// Unsets the current model by setting a dummy empty model. The dummy model is marked as
// "defaultConstructed", so the next call to setModel will delete it.
void AutomatableModelView::unsetModel()
{
	if (dynamic_cast<FloatModelView*>(this))
	{
		setModel(new FloatModel(0, 0, 0, 1, nullptr, QString(), true));
	}
	else if (dynamic_cast<IntModelView*>(this))
	{
		setModel(new IntModel(0, 0, 0, nullptr, QString(), true));
	}
	else if (dynamic_cast<BoolModelView*>(this))
	{
		setModel(new BoolModel(false, nullptr, QString(), true));
	}
	else
	{
		ModelView::unsetModel();
	}
}




void AutomatableModelView::mousePressEvent( QMouseEvent* event )
{
	if( event->button() == Qt::LeftButton && event->modifiers() & Qt::ControlModifier )
	{
		new gui::StringPairDrag( "automatable_model", QString::number( modelUntyped()->id() ), QPixmap(), widget() );
		event->accept();
	}
	else if( event->button() == Qt::MiddleButton )
	{
		modelUntyped()->reset();
	}
}


void AutomatableModelView::setConversionFactor( float factor )
{
	if( factor != 0.0 )
	{
		m_conversionFactor = factor;
	}
}


float AutomatableModelView::getConversionFactor()
{
	return m_conversionFactor;
}


AutomatableModelViewSlots::AutomatableModelViewSlots( AutomatableModelView* amv, QObject* parent ) :
	QObject(),
	m_amv( amv )
{
	connect( parent, SIGNAL(destroyed()), this, SLOT(deleteLater()), Qt::QueuedConnection );
}




void AutomatableModelViewSlots::execConnectionDialog()
{
	// TODO[pg]: Display a dialog with list of controllers currently in the song
	// in addition to any system MIDI controllers
	AutomatableModel* m = m_amv->modelUntyped();

	m->displayName();
	gui::ControllerConnectionDialog d( getGUI()->mainWindow(), m );

	if( d.exec() == 1 )
	{
		// Actually chose something
		if( d.chosenController() )
		{
			// Update
			if( m->controllerConnection() )
			{
				m->controllerConnection()->setController( d.chosenController() );
			}
			// New
			else
			{
				auto cc = new ControllerConnection(d.chosenController());
				m->setControllerConnection( cc );
				//cc->setTargetName( m->displayName() );
			}
		}
		// no controller, so delete existing connection
		else
		{
			removeConnection();
		}
	}
}




void AutomatableModelViewSlots::removeConnection()
{
	AutomatableModel* m = m_amv->modelUntyped();

	if( m->controllerConnection() )
	{
		delete m->controllerConnection();
		m->setControllerConnection( nullptr );
	}
}


void AutomatableModelViewSlots::addSongAutomationPoint()
{
	std::vector<AutomationClip*> clips = AutomationClip::clipsForModel(m_amv->modelUntyped());
	AutomationTrack* track = getCurrentAutomationTrack(&clips);
	AutomationClip* clip = getCurrentAutomationClip(track, true);

	TimePos timePos = static_cast<TimePos>(Engine::getSong()->getPlayPos());// getGUI()->songEditor()->m_editor->currentPosition();
	qDebug("timepos: %d", timePos.getTicks());
	timePos -= clip->startPosition();
	bool autoResize = clip->getAutoResize();

	clip->setAutoResize(true);
	clip->recordValue(timePos, m_amv->modelUntyped()->getTrueValue());
	clip->setAutoResize(autoResize);
}
void AutomatableModelViewSlots::addSongAutomationPointAndClip()
{
	std::vector<AutomationClip*> clips = AutomationClip::clipsForModel(m_amv->modelUntyped());
	AutomationTrack* track = getCurrentAutomationTrack(&clips);
	AutomationClip* clip = getCurrentAutomationClip(track, true);

	TimePos timePos = static_cast<TimePos>(Engine::getSong()->getPlayPos());// getGUI()->songEditor()->m_editor->currentPosition();
	qDebug("timepos: %d", timePos.getTicks());

	if (clip != nullptr && clip->endPosition().getTicks() < timePos.getTicks())
	{
		AutomationClip* newClip = makeNewClip(track, timePos, true);
		timePos -= newClip->startPosition();
		bool autoResize = newClip->getAutoResize();

		newClip->setAutoResize(true);
		newClip->recordValue(timePos, m_amv->modelUntyped()->getTrueValue());
		newClip->setAutoResize(autoResize);
	}
	else
	{
		addSongAutomationPoint();
	}
}
void AutomatableModelViewSlots::removeSongNearestAutomationPoint()
{

	std::vector<AutomationClip*> clips = AutomationClip::clipsForModel(m_amv->modelUntyped());
	AutomationTrack* track = getCurrentAutomationTrack(&clips);
	AutomationClip* clip = getCurrentAutomationClip(track, true);


	tick_t minDistance = 0;
	QMap<int, AutomationNode> timeMap;
	for(QMap<int, AutomationNode>::iterator it = timeMap.begin();
				it != timeMap.end(); ++it )
	{
		//if ()
		// Copies the automation node (in/out values and in/out tangents)
		//timeMap[POS(it)] = it.value();
		// Sets the node's clip to this one
		//timeMap[POS(it)].setClip(this);
	}
}
AutomationTrack* AutomatableModelViewSlots::getCurrentAutomationTrack(std::vector<AutomationClip*>* clips)
{
	AutomationTrack* output = nullptr;
	// getting all of the clips that are connected to this model
	//std::vector<AutomationClip*> clips = AutomationClip().clipsForModel(m_model.data());
	if (clips->size() > 0)
	{
		// selecting the track with the most amount of clips
		// connected to this model
		AutomationTrack* maxTrack = dynamic_cast<AutomationTrack*>((*clips)[0]->getTrack());
		int maxTrackCount = 1;
		for (size_t i = 1; i < clips->size(); i++)
		{
			int currentCount = 0;
			for (size_t j = 0; j < clips->size(); j++)
			{
				if ((*clips)[i]->getTrack() == (*clips)[j]->getTrack())
				{
					currentCount++;
				}
			}
			if (maxTrackCount < currentCount)
			{
				maxTrackCount = currentCount;
				maxTrack = dynamic_cast<AutomationTrack*>((*clips)[i]->getTrack());;
			}
		}
		output = maxTrack;
	}
	else
	{
		// adding new track
		output = new AutomationTrack(getGUI()->songEditor()->m_editor->model(), false);
	}
	return output;
}
AutomationClip* AutomatableModelViewSlots::getCurrentAutomationClip(AutomationTrack* track, bool canAddNewClip)
{
	AutomationClip* output = nullptr;
	std::vector<Clip*> trackClips = track->getClips();
	TimePos timePos = static_cast<TimePos>(Engine::getSong()->getPlayPos());
	if (trackClips.size() > 0)
	{
		// getting the closest clip
		tick_t closestTime = trackClips[0]->startPosition().getTicks();
		int closestClipLocation = 0;
		for (size_t i = 1; i < trackClips.size(); i++)
		{
			if (trackClips[i]->startPosition().getTicks() > closestTime && timePos.getTicks() > trackClips[i]->startPosition().getTicks())
			{
				closestTime = trackClips[i]->startPosition().getTicks();
				closestClipLocation = i;
			}
		}
		output = dynamic_cast<AutomationClip*>(trackClips[closestClipLocation]);
	}
	else if (canAddNewClip == true)
	{
		// adding a new clip
		output = makeNewClip(track, timePos, true);
	}
	return output;
}
AutomationClip* AutomatableModelViewSlots::makeNewClip(AutomationTrack* track, TimePos position, bool canSnap)
{
	if (canSnap == true)
	{
		position.setTicks(position.getTicks() - position.getTickWithinBar(TimeSig(Engine::getSong()->getTimeSigModel())));
		//position.setTicks(position.getTicks() - position.getTicks() % TimePos::ticksPerBar());
	}
	AutomationClip* output = dynamic_cast<AutomationClip*>(track->createClip(position));
	// connect to model
	output->addObject(m_amv->modelUntyped(), true);
	return output;
}

void AutomatableModelViewSlots::editSongGlobalAutomation()
{
	getGUI()->automationEditor()->open(
				AutomationClip::globalAutomationClip(m_amv->modelUntyped())
	);
}



void AutomatableModelViewSlots::removeSongGlobalAutomation()
{
	delete AutomationClip::globalAutomationClip( m_amv->modelUntyped() );
}


void AutomatableModelViewSlots::unlinkAllModels()
{
	m_amv->modelUntyped()->unlinkAllModels();
}

void AutomatableModelViewSlots::copyToClipboard()
{
	// For copyString() and MimeType enum class
	using namespace Clipboard;

	copyString( QString::number( m_amv->value<float>() * m_amv->getConversionFactor() ), MimeType::Default );
}

void AutomatableModelViewSlots::pasteFromClipboard()
{
	bool isNumber = false;
	const float number = floatFromClipboard(&isNumber);
	if (isNumber) {
		m_amv->modelUntyped()->setValue(number / m_amv->getConversionFactor());
	}
}

/// Attempt to parse a float from the clipboard
static float floatFromClipboard(bool* ok)
{
	// For getString() and MimeType enum class
	using namespace Clipboard;

	return getString( MimeType::Default ).toFloat(ok);
}


} // namespace lmms::gui
