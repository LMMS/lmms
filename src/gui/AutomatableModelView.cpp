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

#include <list>
#include <utility>

#include <QMenu>
#include <QMouseEvent>

#include "AutomatableModelView.h"

#include "AutomationClip.h"
#include "AutomationNode.h"
#include "AutomationTrack.h"
#include "Clipboard.h"
#include "ControllerConnectionDialog.h"
#include "ControllerConnection.h"
#include "embed.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "StringPairDrag.h"
#include "Song.h"
#include "SongEditor.h"


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
		&AutomatableModelViewSlots::addSongAutomationNode);
	QMenu* automationMenu = menu->addMenu(QPixmap(), AutomatableModel::tr("more automation"));
	automationMenu->addAction(QPixmap(),
		AutomatableModel::tr("add automation node to new clip"),
		amvSlots,
		&AutomatableModelViewSlots::addSongAutomationNodeAndClip);
	automationMenu->addAction(QPixmap(),
		AutomatableModel::tr("update closest automation node"),
		amvSlots,
		&AutomatableModelViewSlots::updateSongNearestAutomationNode);
	automationMenu->addAction(QPixmap(),
		AutomatableModel::tr("remove closest automation node"),
		amvSlots,
		&AutomatableModelViewSlots::removeSongNearestAutomationNode);

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
	// selecting the track with the most clips connected to this model
	AutomationTrack* track = getCurrentAutomationTrackForModel(true);
	// getting the clip before the current song time position
	AutomationClip* clip = getCurrentAutomationClip(track, true, false);

	// getting global song time
	TimePos timePos = getCurrentPlayingPosition();
	// account for the node's relative position inside clip
	timePos -= clip->startPosition();

	// adding model value
	clip->recordValue(timePos, m_amv->modelUntyped()->getRawValueOrControllerValue());
}

void AutomatableModelViewSlots::addSongAutomationNodeAndClip()
{
	AutomationTrack* track = getCurrentAutomationTrackForModel(true);
	AutomationClip* clip = getCurrentAutomationClip(track, false, false);

	TimePos timePos = getCurrentPlayingPosition();

	if (clip && clip->endPosition().getTicks() < timePos.getTicks())
	{
		AutomationClip* newClip = makeNewClip(track, timePos, true);
		// copying the progressionType of the clip before
		newClip->setProgressionType(clip->progressionType());
		timePos -= newClip->startPosition();

		newClip->recordValue(timePos, m_amv->modelUntyped()->getRawValueOrControllerValue());
	}
	else
	{
		addSongAutomationNode();
	}
}

void AutomatableModelViewSlots::updateSongNearestAutomationNode()
{
	// getting the track without adding a new one if no track was found
	AutomationTrack* track = getCurrentAutomationTrackForModel(false);
	// this needs to be checked because getCurrentAutomationTrack might give
	// a nullptr if it can not find and add a track
	if (!track) { return; }

	// getting nearest node position
	AutomationNodeAtTimePos nodeClip = getNearestAutomationNode(track);
	if (nodeClip.m_clip)
	{
		// modifying its value
		nodeClip.m_clip->recordValue(nodeClip.m_position, m_amv->modelUntyped()->getRawValueOrControllerValue());
	}
}

void AutomatableModelViewSlots::removeSongNearestAutomationNode()
{
	// getting the track without adding a new one if no track was found
	AutomationTrack* track = getCurrentAutomationTrackForModel(false);

	// this needs to be checked because getCurrentAutomationTrack might give
	// a nullptr if it can not find and add a track
	if (!track) { return; }

	AutomationNodeAtTimePos nodeClip = getNearestAutomationNode(track);
	if (nodeClip.m_clip)
	{
		nodeClip.m_clip->removeNode(nodeClip.m_position);
		// if there is no node left, the automationClip will be deleted
		if (nodeClip.m_clip->hasAutomation() == false)
		{
			// this is safe because clips remove themself from tracks
			delete nodeClip.m_clip;
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

		// track*, how many clips are on that track (that are connected to this model)
		std::list<std::pair<AutomationTrack*, size_t>> trackList;
		for (size_t i = 0; i < clips->size(); i++)
		{
			bool found = false;
			// search this track inside the existing tracks
			for (std::pair<AutomationTrack*, size_t>& j : trackList)
			{
				// if the track already is in trackList
				if (j.first == (*clips)[i]->getTrack())
				{
					found = true;
					j.second++;
					break;
				}
			}
			if (!found)
			{
				trackList.push_back(std::make_pair(dynamic_cast<AutomationTrack*>((*clips)[i]->getTrack()), 1));
			}
		}

		size_t matchedModelCount = 0;
		AutomationTrack* matchedTrack = nullptr;
		// find a track where all clips are all connected to this model
		for (std::pair<AutomationTrack*, size_t>& j : trackList)
		{
			// if all clips on that track are connected to this model
			bool isOnlyThatModel = static_cast<size_t>(j.first->numOfClips()) == j.second;
			if (matchedModelCount < j.second && isOnlyThatModel)
			{
				matchedTrack = j.first;
				matchedModelCount = j.second;
			}
		}

		output = matchedTrack;
	}
	if (canAddNewTrack && output == nullptr)
	{
		// adding new track
		output = new AutomationTrack(getGUI()->songEditor()->m_editor->model(), false);
		output->setName(m_amv->modelUntyped()->displayName() + " " + tr("automation"));
	}
	return output;
}

AutomationClip* AutomatableModelViewSlots::getCurrentAutomationClip(AutomationTrack* track, bool canAddNewClip, bool searchAfter)
{
	AutomationClip* output = nullptr;
	const std::vector<Clip*>& trackClips = track->getClips();
	TimePos timePos = getCurrentPlayingPosition();
	
	bool tryAdding = false;
	if (trackClips.size() > 0)
	{
		// getting the closest clip that start before or after the global time position
		tick_t closestTime = -1;
		Clip* closestClip = nullptr;
		for (Clip* currentClip : trackClips)
		{
			tick_t currentTime = currentClip->startPosition().getTicks();
			bool smallerCheck = currentTime > closestTime || closestTime < 0;
			bool biggerCheck = currentTime < closestTime || closestTime < 0;
			bool searchBeforeCheck = !searchAfter && smallerCheck && timePos.getTicks() > currentTime;
			bool searchAfterCheck = searchAfter && biggerCheck && timePos.getTicks() <= currentTime;

			if (searchBeforeCheck || searchAfterCheck)
			{
				closestTime = currentTime;
				closestClip = currentClip;
			}
		}

		// in some cases there could be no clips before or after the global time position
		// if this is the case, try adding a new one
		// (if this fails, return nullptr)
		if (!closestClip)
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
	if (tryAdding && canAddNewClip)
	{
		// adding a new clip
		output = makeNewClip(track, timePos, true);
	}
	return output;
}

const AutomatableModelViewSlots::AutomationNodeAtTimePos AutomatableModelViewSlots::getNearestAutomationNode(AutomationTrack* track)
{
	AutomationNodeAtTimePos output;
	output.m_clip = nullptr;
	int minDistance = -1;

	TimePos timePos = getCurrentPlayingPosition();
	// getting the clips before and after the global time position
	AutomationClip* clipBefore = getCurrentAutomationClip(track, false, false);
	AutomationClip* clipAfter = getCurrentAutomationClip(track, false, true);

	if (clipBefore && clipBefore->hasAutomation())
	{
		// getting nearest node
		// in the clip that starts before this
		for (auto it = clipBefore->getTimeMap().begin(); it != clipBefore->getTimeMap().end(); ++it)
		{
			int curDistance = std::abs(static_cast<int>(POS(it) + clipBefore->startPosition().getTicks()) - static_cast<int>(timePos.getTicks()));
			if (curDistance < minDistance || minDistance < 0)
			{
				minDistance = curDistance;
				output.m_position = TimePos(POS(it));
				output.m_clip = clipBefore;
			}
		}
	}
	if (clipAfter && clipAfter->hasAutomation())
	{
		// getting the nearest node
		// in the clip that starts after this
		int curDistance = static_cast<int>(POS(clipAfter->getTimeMap().begin()) + clipAfter->startPosition().getTicks()) - static_cast<int>(timePos.getTicks());
		if (curDistance < minDistance || minDistance < 0)
		{
			minDistance = curDistance;
			output.m_position = TimePos(POS(clipAfter->getTimeMap().begin()));
			output.m_clip = clipAfter;
		}
	}

	return output;
}

AutomationClip* AutomatableModelViewSlots::makeNewClip(AutomationTrack* track, TimePos position, bool canSnap)
{
	if (canSnap)
	{
		// snapping to the bar before
		position.setTicks(position.getTicks() - position.getTickWithinBar(TimeSig(Engine::getSong()->getTimeSigModel())));
	}
	AutomationClip* output = dynamic_cast<AutomationClip*>(track->createClip(position));
	// connect to model
	output->addObject(m_amv->modelUntyped(), true);
	return output;
}

AutomationTrack* AutomatableModelViewSlots::getCurrentAutomationTrackForModel(bool canAddNewTrack)
{
	// getting all the clips that have this model
	std::vector<AutomationClip*> clips = AutomationClip::clipsForModel(m_amv->modelUntyped());
	// getting the track that has the most clips connected to this model
	return getCurrentAutomationTrack(&clips, canAddNewTrack);
}

TimePos AutomatableModelViewSlots::getCurrentPlayingPosition()
{
	return static_cast<TimePos>(Engine::getSong()->getPlayPos());
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
