/*
 * ResourceAction.h - header file for ResourceAction
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef _RESOURCE_ACTION_H
#define _RESOURCE_ACTION_H

class InstrumentTrack;
class trackContainer;
class ResourceItem;


/*! \brief The ResourceAction class provides centralized functionality for all actions
 * related to a ResourceItem.
 *
 * These actions are for example loading projects, samples, presets,
 * plugin-specific presets etc. Using this class we can avoid duplicated
 * functionality in ResourcePreviewer, ResourceBrowser, TrackContainerView,
 * InstrumentTrack & Co.
 */

class ResourceAction
{
public:
	/*! Lists all supported actions. */
	enum Actions
	{
		EditProperties,				/*!< Open a dialog to edit properties of the ResourceItem */
		LoadProject,				/*!< Load the project represented by the ResourceItem */
		LoadInNewTrackSongEditor,	/*!< Load preset, sample etc. in a new track in Song Editor */
		LoadInNewTrackBBEditor,		/*!< Load preset, sample etc. in a new track in BB Editor */
		LoadInActiveInstrumentTrack,/*!< Load preset, sample etc. in active instrument track */
		DownloadIntoCollection,		/*!< Download the resource into local collection */
		UploadToWWW,				/*!< Upload the resource to Web */
		DeleteLocalResource,		/*!< Delete local resource (=file) */
		ImportFile,					/*!< Try to import the resource via import filter plugins */
		NumActions
	} ;
	typedef Actions Action;

	/*! \brief Constructs a ResourceAction object.
	* \param item The ResourceItem the action is about
	* \param action An optional action from the Action enumeration used for the defaultTrigger() method
	*/
	ResourceAction( const ResourceItem * item,
				Action action = NumActions ) :
		m_action( action ),
		m_item( item )
	{
	}

	bool loadProject();
	bool loadByPlugin( InstrumentTrack * target );
	bool loadPreset( InstrumentTrack * target );
	bool importProject( trackContainer * target );

	/*! \brief Triggers the action passed to the constructor without any further options.
	 *
	 * Most actions can be triggered without any further information.
	 * This allows simple but powerful code constructs:
	 * \code
	 * ResourceAction( myItem, ResourceAction::LoadProject ).defaultTrigger();
	 * \endcode
	 * \return true if the operation succeeded, otherwise false.
	 */
	bool defaultTrigger();


private:
	Action m_action;
	const ResourceItem * m_item;

} ;


#endif
