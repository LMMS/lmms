/*
 * UpgradeTo13Misc.cpp
 *   Functor for upgrading data files BLURB
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

#include "datafile/UpgradeTo13Misc.h"

#include "LocaleHelper.h"


namespace lmms
{


void UpgradeTo13NoHiddenClipNames::upgrade()
{
	QDomNodeList tracks = elementsByTagName("track");

	auto clearDefaultNames = [](QDomNodeList clips, QString trackName)
	{
		for (int j = 0; j < clips.size(); ++j)
		{
			QDomElement clip = clips.item(j).toElement();
			QString clipName = clip.attribute("name", "");
			if (clipName == trackName) { clip.setAttribute("name", ""); }
		}
	};

	for (int i = 0; i < tracks.size(); ++i)
	{
		QDomElement track = tracks.item(i).toElement();
		QString trackName = track.attribute("name", "");

		QDomNodeList instClips = track.elementsByTagName("pattern");
		QDomNodeList autoClips = track.elementsByTagName("automationpattern");
		QDomNodeList bbClips = track.elementsByTagName("bbtco");

		clearDefaultNames(instClips, trackName);
		clearDefaultNames(autoClips, trackName);
		clearDefaultNames(bbClips, trackName);
	}
}


void UpgradeTo13AutomationNodes::upgrade()
{
	QDomNodeList autoPatterns = elementsByTagName("automationpattern");

	// Go through all automation patterns
	for (int i = 0; i < autoPatterns.size(); ++i)
	{
		QDomElement autoPattern = autoPatterns.item(i).toElement();

		// On each automation pattern, get all <time> elements
		QDomNodeList times = autoPattern.elementsByTagName("time");

		// Loop through all <time> elements and change what we need
		for (int j=0; j < times.size(); ++j)
		{
			QDomElement el = times.item(j).toElement();

			float value = LocaleHelper::toFloat(el.attribute("value"));

			// inValue will be equal to "value" and outValue will
			// be set to the same
			el.setAttribute("outValue", value);
		}
	}
}


/** \brief TripleOscillator switched to using high-quality, alias-free oscillators by default
 *
 * Older projects were made without this feature and would sound differently if loaded
 * with the new default setting. This upgrade routine preserves their old behavior.
 */
void UpgradeTo13DefaultTripleOscillatorHQ::upgrade()
{
	QDomNodeList tripleoscillators = elementsByTagName("tripleoscillator");
	for (int i = 0; !tripleoscillators.item(i).isNull(); i++)
	{
		for (int j = 1; j <= 3; j++)
		{
			// Only set the attribute if it does not exist (default template has it but reports as 1.2.0)
			if (tripleoscillators.item(i).toElement().attribute("useWaveTable" + QString::number(j)) == "")
			{
				tripleoscillators.item(i).toElement().setAttribute("useWaveTable" + QString::number(j), 0);
			}
		}
	}
}


void UpgradeTo13SampleAndHold::upgrade()
{
	QDomNodeList elements = m_document.elementsByTagName("lfocontroller");
	for (int i = 0; i < elements.length(); ++i)
	{
		if (elements.item(i).isNull()) { continue; }
		auto e = elements.item(i).toElement();
		// Correct old random wave LFO speeds
		if (e.attribute("wave").toInt() == 6)
		{
			e.setAttribute("speed",0.01f);
		}
	}
}


} // namespace lmms
