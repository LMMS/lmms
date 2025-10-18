/*
 * AudioFileProcessor.cpp - instrument for using audio files
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AudioFileProcessorView.h"

#include <QFileInfo>
#include <QPainter>
#include <iostream>

#include "AudioFileProcessor.h"
#include "AudioFileProcessorWaveView.h"
#include "Clipboard.h"
#include "ComboBox.h"
#include "DataFile.h"
#include "FontHelper.h"
#include "PixmapButton.h"
#include "SampleLoader.h"
#include "Song.h"
#include "StringPairDrag.h"
#include "Track.h"

namespace lmms
{

namespace gui
{

AudioFileProcessorView::AudioFileProcessorView(Instrument* instrument,
							QWidget* parent) :
	InstrumentViewFixedSize(instrument, parent)
{
	m_openAudioFileButton = new PixmapButton(this);
	m_openAudioFileButton->setCursor(QCursor(Qt::PointingHandCursor));
	m_openAudioFileButton->move(227, 72);
	m_openAudioFileButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(
							"select_file"));
	m_openAudioFileButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(
							"select_file"));
	connect(m_openAudioFileButton, SIGNAL(clicked()),
					this, SLOT(openAudioFile()));
	m_openAudioFileButton->setToolTip(tr("Open sample"));

	m_reverseButton = new PixmapButton(this);
	m_reverseButton->setCheckable(true);
	m_reverseButton->move(164, 105);
	m_reverseButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(
							"reverse_on"));
	m_reverseButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(
							"reverse_off"));
	m_reverseButton->setToolTip(tr("Reverse sample"));

// loop button group

	auto m_loopOffButton = new PixmapButton(this);
	m_loopOffButton->setCheckable(true);
	m_loopOffButton->move(190, 105);
	m_loopOffButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(
							"loop_off_on"));
	m_loopOffButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(
							"loop_off_off"));
	m_loopOffButton->setToolTip(tr("Disable loop"));

	auto m_loopOnButton = new PixmapButton(this);
	m_loopOnButton->setCheckable(true);
	m_loopOnButton->move(190, 124);
	m_loopOnButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(
							"loop_on_on"));
	m_loopOnButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(
							"loop_on_off"));
	m_loopOnButton->setToolTip(tr("Enable loop"));

	auto m_loopPingPongButton = new PixmapButton(this);
	m_loopPingPongButton->setCheckable(true);
	m_loopPingPongButton->move(216, 124);
	m_loopPingPongButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(
							"loop_pingpong_on"));
	m_loopPingPongButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(
							"loop_pingpong_off"));
	m_loopPingPongButton->setToolTip(tr("Enable ping-pong loop"));

	m_loopGroup = new AutomatableButtonGroup(this);
	m_loopGroup->addButton(m_loopOffButton);
	m_loopGroup->addButton(m_loopOnButton);
	m_loopGroup->addButton(m_loopPingPongButton);

	m_stutterButton = new PixmapButton(this);
	m_stutterButton->setCheckable(true);
	m_stutterButton->move(164, 124);
	m_stutterButton->setActiveGraphic(PLUGIN_NAME::getIconPixmap(
								"stutter_on"));
	m_stutterButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap(
								"stutter_off"));
	m_stutterButton->setToolTip(
		tr("Continue sample playback across notes"));

	m_ampKnob = new Knob(KnobType::Bright26, this);
	m_ampKnob->setVolumeKnob(true);
	m_ampKnob->move(5, 108);
	m_ampKnob->setHintText(tr("Amplify:"), "%");

	m_startKnob = new AudioFileProcessorWaveView::knob(this);
	m_startKnob->move(50, 108);
	m_startKnob->setHintText(tr("Start point:"), "");

	m_endKnob = new AudioFileProcessorWaveView::knob(this);
	m_endKnob->move(130, 108);
	m_endKnob->setHintText(tr("End point:"), "");

	m_loopKnob = new AudioFileProcessorWaveView::knob(this);
	m_loopKnob->move(90, 108);
	m_loopKnob->setHintText(tr("Loopback point:"), "");

// interpolation selector
	m_interpBox = new ComboBox(this);
	m_interpBox->setGeometry(142, 62, 82, ComboBox::DEFAULT_HEIGHT);

// wavegraph
	m_waveView = 0;
	newWaveView();

	connect(castModel<AudioFileProcessor>(), SIGNAL(isPlaying(lmms::f_cnt_t)),
			m_waveView, SLOT(isPlaying(lmms::f_cnt_t)));

	qRegisterMetaType<lmms::f_cnt_t>("lmms::f_cnt_t");

	setAcceptDrops(true);
}

void AudioFileProcessorView::dragEnterEvent(QDragEnterEvent* dee)
{
	StringPairDrag::processDragEnterEvent(dee, {"samplefile"});
}

void AudioFileProcessorView::newWaveView()
{
	if (m_waveView)
	{
		delete m_waveView;
		m_waveView = 0;
	}
	m_waveView = new AudioFileProcessorWaveView(this, 245, 75, &castModel<AudioFileProcessor>()->sample(),
		dynamic_cast<AudioFileProcessorWaveView::knob*>(m_startKnob),
		dynamic_cast<AudioFileProcessorWaveView::knob*>(m_endKnob),
		dynamic_cast<AudioFileProcessorWaveView::knob*>(m_loopKnob));
	m_waveView->move(2, 172);
	
	m_waveView->show();
}

void AudioFileProcessorView::dropEvent(QDropEvent* de)
{
	const auto [type, value] = Clipboard::decodeMimeData(de->mimeData());

	if (type == "samplefile")
	{
		castModel<AudioFileProcessor>()->setAudioFile(value);
	}

	else if (type == QString("clip_%1").arg(static_cast<int>(Track::Type::Sample)))
	{
		DataFile dataFile(value.toUtf8());
		castModel<AudioFileProcessor>()->setAudioFile(dataFile.content().firstChild().toElement().attribute("src"));
	}
	else
	{
		de->ignore();
		return;
	}

	m_waveView->updateSampleRange();
	Engine::getSong()->setModified();
	de->accept();
}

void AudioFileProcessorView::paintEvent(QPaintEvent*)
{
	QPainter p(this);

	static auto s_artwork = PLUGIN_NAME::getIconPixmap("artwork");
	p.drawPixmap(0, 0, s_artwork);

	auto a = castModel<AudioFileProcessor>();

	QString file_name = "";

	int idx = a->sample().sampleFile().length();

	p.setFont(adjustedToPixelSize(font(), SMALL_FONT_SIZE));

	QFontMetrics fm(p.font());

	// simple algorithm for creating a text from the filename that
	// matches in the white rectangle
	while(idx > 0 &&
		fm.size(Qt::TextSingleLine, file_name + "...").width() < 210)
	{
		file_name = a->sample().sampleFile()[--idx] + file_name;
	}

	if (idx > 0)
	{
		file_name = "..." + file_name;
	}

	p.setPen(QColor(255, 255, 255));
	p.drawText(8, 99, file_name);
}

void AudioFileProcessorView::sampleUpdated()
{
	m_waveView->updateSampleRange();
	m_waveView->update();
	update();
}

void AudioFileProcessorView::openAudioFile()
{
	QString af = SampleLoader::openAudioFile();
	if (af.isEmpty()) { return; }

	castModel<AudioFileProcessor>()->setAudioFile(af);
	Engine::getSong()->setModified();
	m_waveView->updateSampleRange();
}

void AudioFileProcessorView::modelChanged()
{
	auto a = castModel<AudioFileProcessor>();
	connect(a, &AudioFileProcessor::sampleUpdated, this, &AudioFileProcessorView::sampleUpdated);
	m_ampKnob->setModel(&a->ampModel());
	m_startKnob->setModel(&a->startPointModel());
	m_endKnob->setModel(&a->endPointModel());
	m_loopKnob->setModel(&a->loopPointModel());
	m_reverseButton->setModel(&a->reverseModel());
	m_loopGroup->setModel(&a->loopModel());
	m_stutterButton->setModel(&a->stutterModel());
	m_interpBox->setModel(&a->interpolationModel());
	sampleUpdated();
}

} // namespace gui

} // namespace lmms
