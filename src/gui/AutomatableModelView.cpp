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
#include "AutomationNode.h"
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

	menu->addAction(QPixmap(),
		AutomatableModel::tr("add automation node"),
		amvSlots,
		SLOT(addSongAutomationNode()));
	menu->addAction(QPixmap(),
		AutomatableModel::tr("add automation node to new clip"),
		amvSlots,
		SLOT(addSongAutomationNodeAndClip()));
	menu->addAction(QPixmap(),
		AutomatableModel::tr("update closest automation node"),
		amvSlots,
		SLOT(updateSongNearestAutomationNode()));
	menu->addAction(QPixmap(),
		AutomatableModel::tr("remove closest automation node"),
		amvSlots,
		SLOT(removeSongNearestAutomationNode()));

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


void AutomatableModelViewSlots::addSongAutomationNode()
{
	// getting all the clips that have this model
	std::vector<AutomationClip*> clips = AutomationClip::clipsForModel(m_amv->modelUntyped());
	// selecting the track with the most clips connected to this model
	AutomationTrack* track = getCurrentAutomationTrack(&clips, true);
	// getting the clip before the current song time position
	AutomationClip* clip = getCurrentAutomationClip(track, true, false);

	// getting global song time
	TimePos timePos = static_cast<TimePos>(Engine::getSong()->getPlayPos());
	// account for the node's relative position inside clip
	timePos -= clip->startPosition();
	bool autoResize = clip->getAutoResize();

	clip->setAutoResize(true);
	// adding model value
	clip->recordValue(timePos, m_amv->modelUntyped()->getTrueValue());
	clip->setAutoResize(autoResize);
}
void AutomatableModelViewSlots::addSongAutomationNodeAndClip()
{
	std::vector<AutomationClip*> clips = AutomationClip::clipsForModel(m_amv->modelUntyped());
	AutomationTrack* track = getCurrentAutomationTrack(&clips, true);
	AutomationClip* clip = getCurrentAutomationClip(track, false, false);

	TimePos timePos = static_cast<TimePos>(Engine::getSong()->getPlayPos());

	if (clip != nullptr && clip->endPosition().getTicks() < timePos.getTicks())
	{
		AutomationClip* newClip = makeNewClip(track, timePos, true);
		// copying the progressionType of the clip before
		newClip->setProgressionType(clip->progressionType());
		timePos -= newClip->startPosition();
		bool autoResize = newClip->getAutoResize();

		newClip->setAutoResize(true);
		newClip->recordValue(timePos, m_amv->modelUntyped()->getTrueValue());
		newClip->setAutoResize(autoResize);
	}
	else
	{
		addSongAutomationNode();
	}
}
void AutomatableModelViewSlots::updateSongNearestAutomationNode()
{
	std::vector<AutomationClip*> clips = AutomationClip::clipsForModel(m_amv->modelUntyped());
	// getting the track without adding a new one if no track was found
	AutomationTrack* track = getCurrentAutomationTrack(&clips, false);
	// this needs to be checked because getCurrentAutomationTrack might give
	// a nullptr if it can not find and add a track
	if (track == nullptr) { return; }

	// getting nearest node position
	AutomationClip* nodeClip = nullptr;
	TimePos nodePos = getNearestAutomationNode(track, &nodeClip);
	if (nodeClip != nullptr)
	{
		// modifying its value
		nodeClip->recordValue(nodePos, m_amv->modelUntyped()->getTrueValue());
	}
}
void AutomatableModelViewSlots::removeSongNearestAutomationNode()
{
	std::vector<AutomationClip*> clips = AutomationClip::clipsForModel(m_amv->modelUntyped());
	// getting the track without adding a new one if no track was found
	AutomationTrack* track = getCurrentAutomationTrack(&clips, false);

	// this needs to be checked because getCurrentAutomationTrack might give
	// a nullptr if it can not find and add a track
	if (track == nullptr) { return; }

	AutomationClip* nodeClip = nullptr;
	TimePos nodePos = getNearestAutomationNode(track, &nodeClip);
	if (nodeClip != nullptr)
	{
		nodeClip->removeNode(nodePos);
		// if there is no node left, the automationClip will be deleted
		if (nodeClip->hasAutomation() == false)
		{
			delete nodeClip;
		}
	}
}
AutomationTrack* AutomatableModelViewSlots::getCurrentAutomationTrack(std::vector<AutomationClip*>* clips, bool canAddNewTrack)
{
	AutomationTrack* output = nullptr;
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
	else if (canAddNewTrack == true)
	{
		// adding new track
		output = new AutomationTrack(getGUI()->songEditor()->m_editor->model(), false);
	}
	return output;
}
AutomationClip* AutomatableModelViewSlots::getCurrentAutomationClip(AutomationTrack* track, bool canAddNewClip, bool searchAfter)
{
	AutomationClip* output = nullptr;
	std::vector<Clip*> trackClips = track->getClips();
	TimePos timePos = static_cast<TimePos>(Engine::getSong()->getPlayPos());
	

	bool tryAdding = false;
	if (trackClips.size() > 0)
	{
		// getting the closest clip that start before or after the global time position
		tick_t closestTime = -1;
		Clip* closestClip = nullptr;
		for (Clip* currentClip : trackClips)
		{
			tick_t currentTime = currentClip->startPosition().getTicks();
			if ((searchAfter == false && (currentTime > closestTime || closestTime < 0) && timePos.getTicks() > currentTime)
				|| (searchAfter == true && (currentTime < closestTime || closestTime < 0) && timePos.getTicks() <= currentTime))
			{
				closestTime = currentTime;
				closestClip = currentClip;
			}
		}

		// in some cases there could be no clips before or after the global time position
		// if this is the case, try adding a new one
		// (if this fails, return nullptr)
		if (closestClip == nullptr)
		{
			tryAdding = true;
		}
		else
		{
			output = dynamic_cast<AutomationClip*>(closestClip);
		}
	}
	else
	{
		tryAdding = true;
	}
	if (tryAdding == true && canAddNewClip == true)
	{
		// adding a new clip
		output = makeNewClip(track, timePos, true);
	}
	return output;
}
const TimePos AutomatableModelViewSlots::getNearestAutomationNode(AutomationTrack* track, AutomationClip** clipOut)
{
	TimePos output;
	AutomationClip* minClip = nullptr;
	int minDistance = -1;

	TimePos timePos = static_cast<TimePos>(Engine::getSong()->getPlayPos());
	// getting the clips before and after the global time position
	AutomationClip* clipBefore = getCurrentAutomationClip(track, false, false);
	AutomationClip* clipAfter = getCurrentAutomationClip(track, false, true);

	if (clipBefore != nullptr && clipBefore->hasAutomation() == true)
	{
		// getting nearest node
		// in the clip that starts before this
		for(AutomationClip::timeMap::const_iterator it = clipBefore->getTimeMap().begin(); it != clipBefore->getTimeMap().end(); ++it)
		{
			int curDistance = std::abs(static_cast<int>(POS(it) + clipBefore->startPosition().getTicks()) - static_cast<int>(timePos.getTicks()));
			if (curDistance < minDistance || minDistance < 0)
			{
				minDistance = curDistance;
				output = TimePos(POS(it));
				minClip = clipBefore;
			}
		}
	}
	if (clipAfter != nullptr && clipAfter->hasAutomation() == true)
	{
		// getting the nearest node
		// in the clip that starts after this
		int curDistance = static_cast<int>(POS(clipAfter->getTimeMap().begin()) + clipAfter->startPosition().getTicks()) - static_cast<int>(timePos.getTicks());
		if (curDistance < minDistance || minDistance < 0)
		{
			minDistance = curDistance;
			output = TimePos(POS(clipAfter->getTimeMap().begin()));
			minClip = clipAfter;
		}
	}

	*clipOut = minClip;

	return output;
}
AutomationClip* AutomatableModelViewSlots::makeNewClip(AutomationTrack* track, TimePos position, bool canSnap)
{
	if (canSnap == true)
	{
		// snapping to the bar before
		position.setTicks(position.getTicks() - position.getTickWithinBar(TimeSig(Engine::getSong()->getTimeSigModel())));
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
