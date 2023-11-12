/*
 * UpgradeExtendedNoteRange.cpp - Upgrades the extended note range
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

#include "UpgradeExtendedNoteRange.h"

#include "Track.h"

#include <QDomElement>

#include <set>
#include <cassert>


namespace lmms
{

/**
 * @brief Used by the helper function that analyzes automation patterns.
 */
struct PatternAnalysisResult
{
	PatternAnalysisResult(bool hasBaseNoteAutomations, bool hasNonBaseNoteAutomations)
	{
		this->hasBaseNoteAutomations = hasBaseNoteAutomations;
		this->hasNonBaseNoteAutomations = hasNonBaseNoteAutomations;
	}
	bool hasBaseNoteAutomations;
	bool hasNonBaseNoteAutomations;
};

/**
 * @brief Helper function that checks for an automation pattern if it contains automation for
 * targets that are base notes and/or other targets.
 * @param automationPattern The automation pattern to be checked.
 * @param automatedBaseNoteIds A set of id of automated base notes that are used in the check.
 * @return A struct that contains the results.
 */
static PatternAnalysisResult analyzeAutomationPattern(QDomElement const & automationPattern, std::set<unsigned int> const & automatedBaseNoteIds)
{
	bool hasBaseNoteAutomations = false;
	bool hasNonBaseNoteAutomations = false;

		   // Iterate the objects. These contain the ids of the automated objects.
	QDomElement object = automationPattern.firstChildElement("object");
	while (!object.isNull())
	{
		unsigned int const id = object.attribute("id").toUInt();

			   // Check if the automated object is a base note.
		if (automatedBaseNoteIds.find(id) != automatedBaseNoteIds.end())
		{
			hasBaseNoteAutomations = true;
		}
		else
		{
			hasNonBaseNoteAutomations = true;
		}

		object = object.nextSiblingElement("object");
	}

	return PatternAnalysisResult(hasBaseNoteAutomations, hasNonBaseNoteAutomations);
}

static void fixNotePatterns(QDomNodeList & patterns)
{
	for (int i = 0; i < patterns.size(); ++i)
	{
		QDomNodeList notes = patterns.item(i).toElement().elementsByTagName("note");
		for (int j = 0; j < notes.size(); ++j)
		{
			QDomElement note = notes.item(j).toElement();
			if (note.hasAttribute("key"))
			{
				int const currentKey = note.attribute("key").toInt();
				note.setAttribute("key", currentKey + 12);
			}
		}
	}
}

static void fixInstrumentBaseNoteAndCollectIds(QDomElement & instrument, std::set<unsigned int> & automatedBaseNoteIds)
{
	// Raise the base note of every instrument by 12 to compensate for the change
	// of A4 key code from 57 to 69. This ensures that notes are labeled correctly.
	QDomElement instrumentParent = instrument.parentNode().toElement();

		   // Correct the base note of the instrument. Base notes which are automated are
		   // stored as elements. Non-automated base notes are stored as attributes.
	if (instrumentParent.hasAttribute("basenote"))
	{
		// TODO Base notes can have float values in the save file! This might need to be changed!
		int const currentBaseNote = instrumentParent.attribute("basenote").toInt();
		instrumentParent.setAttribute("basenote", currentBaseNote + 12);
	}
	else
	{
		// Check if the instrument track has an automated base note.
		// Correct the value of the base note and collect their ids while doing so.
		// The ids are used later to find the automations that automate these corrected base
		// notes so that we can correct the automation values as well.
		QDomNodeList baseNotes = instrumentParent.elementsByTagName("basenote");
		for (int j = 0; j < baseNotes.size(); ++j)
		{
			QDomElement baseNote = baseNotes.item(j).toElement();
			if (!baseNote.isNull())
			{
				if (baseNote.hasAttribute("value"))
				{
					// Base notes can have float values in the save file, e.g. if the file
					// is saved after a linear automation has run on the base note. Therefore
					// it is fixed here as a float even if the nominal values of base notes
					// are integers.
					float const value = baseNote.attribute("value").toFloat();
					baseNote.setAttribute("value", value + 12);
				}

					   // The ids of base notes are of type jo_id_t which are in fact uint32_t.
					   // So let's just use these here to save some casting.
				unsigned int const id = baseNote.attribute("id").toUInt();
				automatedBaseNoteIds.insert(id);
			}
		}
	}
}

/**
 * @brief Helper method that fixes the values and out values for an automation pattern.
 * @param automationPattern The automation pattern to be fixed.
 */
static void fixAutomationPattern(QDomElement & automationPattern)
{
	QDomElement time = automationPattern.firstChildElement("time");
	while (!time.isNull())
	{
		// Automation patterns can automate base notes as floats
		// so we read and correct them as floats here.
		float const value = time.attribute("value").toFloat();
		time.setAttribute("value", value + 12.);

			   // The method "upgrade_automationNodes" adds some attributes
			   // with the name "outValue". We have to correct these as well.
		float const outValue = time.attribute("outValue").toFloat();
		time.setAttribute("outValue", outValue + 12.);

		time = time.nextSiblingElement("time");
	};
}

static bool affected(QDomElement & instrument)
{
	assert(instrument.hasAttribute("name"));
	QString const name = instrument.attribute("name");

	return name == "zynaddsubfx" ||
		name  == "vestige" || name == "lv2instrument" ||
		name  == "carlapatchbay" || name == "carlarack";
}

static void fixTrack(QDomElement & track, std::set<unsigned int> & automatedBaseNoteIds)
{
	if (!track.hasAttribute("type"))
	{
		return;
	}

	Track::Type const trackType = static_cast<Track::Type>(track.attribute("type").toInt());

		   // BB tracks need special handling because they contain a track container of their own
	if (trackType == Track::Type::Pattern)
	{
		// Assuming that a BB track cannot contain another BB track here...
		QDomNodeList subTracks = track.elementsByTagName("track");
		for (int i = 0; i < subTracks.size(); ++i)
		{
			QDomElement subTrack = subTracks.item(i).toElement();
			assert (static_cast<Track::Type>(subTrack.attribute("type").toInt()) != Track::Type::Pattern);
			fixTrack(subTrack, automatedBaseNoteIds);
		}
	}
	else
	{
		QDomNodeList instruments = track.elementsByTagName("instrument");

		for (int i = 0; i < instruments.size(); ++i)
		{
			QDomElement instrument = instruments.item(i).toElement();

			fixInstrumentBaseNoteAndCollectIds(instrument, automatedBaseNoteIds);

				   // Raise the pitch of all notes in patterns assigned to instruments not affected
				   // by #1857 by an octave. This negates the base note change for normal instruments,
				   // but leaves the MIDI-based instruments sounding an octave lower, preserving their
				   // pitch in existing projects.
			if (!affected(instrument))
			{
				QDomNodeList patterns = track.elementsByTagName("pattern");
				fixNotePatterns(patterns);
			}
		}
	}
}

static void fixAutomationTracks(QDomElement & song, std::set<unsigned int> const & automatedBaseNoteIds)
{
	// Now fix all the automation tracks.
	QDomNodeList tracks = song.elementsByTagName("track");

		   // We have to collect the tracks that we need to duplicate and cannot do this in-place
		   // because if we did the iteration might never stop.
	std::vector<QDomElement> tracksToDuplicate;
	tracksToDuplicate.reserve(tracks.size());

		   // Iterate the tracks again. This time work on all automation tracks.
	for (int i = 0; i < tracks.size(); ++i)
	{
		QDomElement currentTrack = tracks.item(i).toElement();
		if (static_cast<Track::Type>(currentTrack.attribute("type").toInt()) != Track::Type::Automation)
		{
			continue;
		}

			   // Check each track for the types of automations it contains in its patterns.
		bool containsPatternsWithBaseNoteTargets = false;
		bool containsPatternsWithNonBaseNoteTargets = false;

		QDomElement automationPattern = currentTrack.firstChildElement("automationpattern");
		while (!automationPattern.isNull())
		{
			auto const analysis = analyzeAutomationPattern(automationPattern, automatedBaseNoteIds);
			containsPatternsWithBaseNoteTargets |= analysis.hasBaseNoteAutomations;
			containsPatternsWithNonBaseNoteTargets |= analysis.hasNonBaseNoteAutomations;

			automationPattern = automationPattern.nextSiblingElement("automationpattern");
		}

		if (!containsPatternsWithBaseNoteTargets)
		{
			// No base notes are automated by this automation track so we have nothing to do
			continue;
		}
		else
		{
			if (!containsPatternsWithNonBaseNoteTargets)
			{
				// Only base note targets. This means we can simply keep the track and fix it.
				automationPattern = currentTrack.firstChildElement("automationpattern");
				while (!automationPattern.isNull())
				{
					fixAutomationPattern(automationPattern);

					automationPattern = automationPattern.nextSiblingElement("automationpattern");
				}
			}
			else
			{
				// The automation track has automations for base notes and other targets in its patterns.
				// We will later need to duplicate/split the track.
				tracksToDuplicate.push_back(currentTrack);
			}
		}
	}

		   // Now fix the tracks that need duplication/splitting
	for (QDomElement & track : tracksToDuplicate)
	{
		// First clone the original track
		QDomNode cloneOfTrack = track.cloneNode();

			   // Now that we have the original and the clone we can manipulate both of them.
			   // The original track will keep only patterns without base note automations.
			   // Note: for the original track these might also be automation patterns without
			   // any targets. We will keep these because they might have been saved by the users
			   // like this.
		QDomElement automationPattern = track.firstChildElement("automationpattern");
		while (!automationPattern.isNull())
		{
			auto const analysis = analyzeAutomationPattern(automationPattern, automatedBaseNoteIds);
			if (!analysis.hasBaseNoteAutomations)
			{
				// This pattern has no base note automations. Leave it alone.
				automationPattern = automationPattern.nextSiblingElement("automationpattern");
			}
			else if (!analysis.hasNonBaseNoteAutomations)
			{
				// The pattern only has base note automations. Remove it completely as it would become empty.
				QDomElement patternToRemove = automationPattern;
				automationPattern = automationPattern.nextSiblingElement("automationpattern");
				track.removeChild(patternToRemove);
			}
			else
			{
				// The pattern itself is mixed. Remove the base note objects.
				QDomElement object = automationPattern.firstChildElement("object");
				while (!object.isNull())
				{
					unsigned int const id = object.attribute("id").toUInt();

					if (automatedBaseNoteIds.find(id) != automatedBaseNoteIds.end())
					{
						QDomElement objectToRemove = object;
						object = object.nextSiblingElement("object");
						automationPattern.removeChild(objectToRemove);
					}
					else
					{
						object = object.nextSiblingElement("object");
					}
				}

				automationPattern = automationPattern.nextSiblingElement("automationpattern");
			}
		}

			   // The clone will only keep non-empty patterns with base note automations
			   // and the values of the patterns will be corrected.
		automationPattern = cloneOfTrack.firstChildElement("automationpattern");
		while (!automationPattern.isNull())
		{
			auto const analysis = analyzeAutomationPattern(automationPattern, automatedBaseNoteIds);
			if (analysis.hasBaseNoteAutomations)
			{
				// This pattern has base note automations. Remove all other ones and fix the pattern.
				QDomElement object = automationPattern.firstChildElement("object");
				while (!object.isNull())
				{
					unsigned int const id = object.attribute("id").toUInt();

					if (automatedBaseNoteIds.find(id) == automatedBaseNoteIds.end())
					{
						QDomElement objectToRemove = object;
						object = object.nextSiblingElement("object");
						automationPattern.removeChild(objectToRemove);
					}
					else
					{
						object = object.nextSiblingElement("object");
					}
				}

				fixAutomationPattern(automationPattern);

				automationPattern = automationPattern.nextSiblingElement("automationpattern");
			}
			else
			{
				// The pattern has no base note automations. Remove it completely.
				QDomElement patternToRemove = automationPattern;
				automationPattern = automationPattern.nextSiblingElement("automationpattern");
				cloneOfTrack.removeChild(patternToRemove);
			}
		}
		track.parentNode().appendChild(cloneOfTrack);
	}
}


UpgradeExtendedNoteRange::UpgradeExtendedNoteRange(QDomElement & domElement) :
	m_domElement(domElement)
{
}

void UpgradeExtendedNoteRange::upgrade()
{
	QDomElement song = m_domElement.firstChildElement("song");
	while (!song.isNull())
	{
		// This set will later contain all ids of automated base notes. They are
		// used to find out which automation patterns must to be corrected, i.e. to
		// check if an automation pattern has one or more base notes as its target.
		std::set<unsigned int> automatedBaseNoteIds;

		QDomElement trackContainer = song.firstChildElement("trackcontainer");
		while (!trackContainer.isNull())
		{
			QDomElement track = trackContainer.firstChildElement("track");
			while (!track.isNull())
			{
				fixTrack(track, automatedBaseNoteIds);

				track = track.nextSiblingElement("track");
			}

			trackContainer = trackContainer.nextSiblingElement("trackcontainer");
		}

		fixAutomationTracks(song, automatedBaseNoteIds);

		song = song.nextSiblingElement("song");
	};

	if (m_domElement.elementsByTagName("song").item(0).isNull())
	{
		// Dealing with a preset, not a song
		QDomNodeList presets = m_domElement.elementsByTagName("instrumenttrack");
		if (presets.item(0).isNull()) { return; }
		QDomElement preset = presets.item(0).toElement();
		// Common correction for all instrument presets (make base notes match the new MIDI range).
		// NOTE: Many older presets do not have any basenote defined, assume they were "made for 57".
		// (Specifying a default like this also happens to fix a FileBrowser bug where previews of presets
		// with undefined basenote always play with the basenote inherited from previously played preview.)
		int oldBase = preset.attribute("basenote", "57").toInt();
		preset.setAttribute("basenote", oldBase + 12);
		// Extra correction for Zyn, VeSTige, LV2 and Carla (to preserve the original buggy behavior).
		QDomNodeList instruments = presets.item(0).toElement().elementsByTagName("instrument");
		if (instruments.isEmpty()) { return; }
		QDomElement instrument = instruments.item(0).toElement();
		if (affected(instrument))
		{
			preset.setAttribute("basenote", preset.attribute("basenote").toInt() + 12);
		}
	}
}

} // namespace lmms
