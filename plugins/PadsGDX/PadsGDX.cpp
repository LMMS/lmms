/*
 * PadsGDX.cpp - sample player for pads
 *
 * Copyright (c) 2017 gi0e5b06
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


#include <QPainter>
#include <QBitmap>
#include <QDomDocument>
#include <QFileInfo>
#include <QDropEvent>
#include <samplerate.h>

#include "PadsGDX.h"
#include "PadsGDXView.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "Song.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "NotePlayHandle.h"
#include "interpolation.h"
#include "gui_templates.h"
#include "ToolTip.h"
#include "StringPairDrag.h"
#include "DataFile.h"
#include "embed.h"

extern "C" {
Plugin::Descriptor PLUGIN_EXPORT padsgdx_plugin_descriptor = {
	STRINGIFY(PLUGIN_NAME),
	"PadsGDX",
	QT_TRANSLATE_NOOP("pluginBrowser", "One sample per key. Intended for pads."),
	"gi0e5b06",
	0x0100,
	Plugin::Instrument,
	new PluginPixmapLoader("logo"),
	"wav,ogg,ds,spx,au,voc,aif,aiff,flac,raw",
	NULL
};
}

PadsGDX::PadsGDX(InstrumentTrack* track) :
	Instrument(track, &padsgdx_plugin_descriptor),
	m_currentKey(-1),
	m_checking(false),
	m_loading(false),
	m_nextPlayStartPoint(0),
	m_nextPlayBackwards(false)
{
	for (int i=0; i<128; i++) {
		m_sampleBuffer[i]=NULL;
		m_startPointModel[i]=NULL;
		m_endPointModel[i]=NULL;
		m_loopStartPointModel[i]=NULL;
		m_loopEndPointModel[i]=NULL;
		m_reverseModel[i]=NULL;
		m_loopModel[i]=NULL;
		m_stutterModel[i]=NULL;
	}
	if (instrumentTrack() && instrumentTrack()->baseNoteModel()) {
		setCurrentKey(instrumentTrack()->baseNoteModel()->value());
	} else {
        setCurrentKey(69);
	}
	setAudioFile("bassloops/briff01.ogg");
}

PadsGDX::~PadsGDX(){}

void PadsGDX::playNote(NotePlayHandle* nph, sampleFrame* buffer) {
    if (!nph || nph->key() < 0 || nph->key() > 127) {
		return;
	}
	int key = nph->key();
	//int base=instrumentTrack()->baseNoteModel()->value();
	//qInfo("PadsGDX::playNote origin=%d",nph->origin());
	//key+=69-base;

	if ((nph->origin() == NotePlayHandle::OriginMidiInput)) {
		setCurrentKey(key);
	}

	SampleBuffer* sample = m_sampleBuffer[key];
	//qInfo("PadsGDX::play key=%d base=%d s=%p",key,base,sample);
	if (!sample) {
		return;
	}

	const fpp_t frames = nph->framesLeftForCurrentPeriod();
	const f_cnt_t offset = nph->noteOffset();

	// Magic key - a frequency < 20 (say, the bottom piano note if using
	// a A4 base tuning) restarts the start point. The note is not actually
	// played.
	if (m_stutterModel[key]->value() == true && nph->frequency() < 20.0) {
		m_nextPlayStartPoint = sample->startFrame();
		m_nextPlayBackwards = false;
		return;
	}

	if (!nph->m_pluginData) {
		if (m_stutterModel[key]->value() == true && m_nextPlayStartPoint >= sample->endFrame()) {
			// Restart playing the note if in stutter mode, not in loop mode,
			// and we're at the end of the sample.
			m_nextPlayStartPoint = sample->startFrame();
			m_nextPlayBackwards = false;
		}
		// set interpolation mode for libsamplerate
		int srcmode = SRC_LINEAR;
		/*
		switch (m_interpolationModel[key]->value()) {
			case 0:
				srcmode = SRC_ZERO_ORDER_HOLD;
				break;
			case 1:
				srcmode = SRC_LINEAR;
				break;
			case 2:
				srcmode = SRC_SINC_MEDIUM_QUALITY;
				break;
		}
		*/
		nph->m_pluginData = new handleState(nph->hasDetuningInfo(), srcmode);
		((handleState*)nph->m_pluginData)->setFrameIndex(m_nextPlayStartPoint);
		((handleState*)nph->m_pluginData)->setBackwards(m_nextPlayBackwards);

		// debug code
		/*
		  qDebug("frames %d", sample->frames());
		  qDebug("startframe %d", sample->startFrame());
		  qDebug("nextPlayStartPoint %d", m_nextPlayStartPoint);
		*/
	}

	if (!nph->isFinished()) {
		if (sample->play(buffer + offset, (handleState*)nph->m_pluginData, frames, 440, //force nph->frequency(),
			static_cast<SampleBuffer::LoopMode>(m_loopModel[key]->value()))) {
			applyRelease(buffer, nph);
			instrumentTrack()->processAudioBuffer(buffer, frames + offset, nph);
			if (key==currentKey()) {
				emit isPlaying(((handleState*)nph->m_pluginData)->frameIndex());
			}
		} else {
			memset(buffer, 0, (frames + offset) * sizeof(sampleFrame));
			if (key == currentKey()) {
				emit isPlaying(0);
			}
		}
	} else {
		if (key == currentKey()) {
			emit isPlaying(0);
		}
	}

	if (m_stutterModel[key]->value() == true) {
		m_nextPlayStartPoint = ((handleState*)nph->m_pluginData)->frameIndex();
		m_nextPlayBackwards = ((handleState*)nph->m_pluginData)->isBackwards();
	}
}

void PadsGDX::deleteNotePluginData(NotePlayHandle* nph) {
	delete (handleState*)nph->m_pluginData;
}


void PadsGDX::loadFile(const QString& file){
	setAudioFile(file);
}

QString PadsGDX::nodeName(void) const {
	return padsgdx_plugin_descriptor.name;
}

int PadsGDX::getBeatLen(NotePlayHandle *nph) const {
    if (!nph || nph->key() < 0 || nph->key() > 127) {
		return 0;
	}
    int key = nph->key();

	SampleBuffer* sample = m_sampleBuffer[key];
	if (!sample) {
		return 0;
	}

	const float freq_factor = BaseFreq / nph->frequency() *
		Engine::mixer()->processingSampleRate() / Engine::mixer()->baseSampleRate();

	return static_cast<int>(floorf((sample->endFrame() - sample->startFrame()) * freq_factor));
}

PluginView* PadsGDX::instantiateView(QWidget* parent) {
	PadsGDXView* view = new PadsGDXView(this, parent);
	view->doConnections();
	return view;
}

void PadsGDX::createKey(int key, const QString& file) {
	if (key < 0 || key > 127) {
		return;
	}
	createKey(key, new SampleBuffer(file));
}

void PadsGDX::createKey(int key, SampleBuffer* sample) {
	if (key < 0 || key > 127 || !sample) {
		return;
	}
	SampleBuffer* old = m_sampleBuffer[key];
	if (old) {
		destroyKey(key);
	}

	m_sampleBuffer[key]=sample;
	//m_ampModel(100, 0, 500, 1, this, tr("Amplify"));
	m_startPointModel    [key]=new FloatModel(0.f, 0.f, 1, 0.0000001f, this, tr("Start of sample"));
	m_endPointModel      [key]=new FloatModel(1.f, 0.f, 1, 0.0000001f, this, tr("End of sample"));
	m_loopStartPointModel[key]=new FloatModel(0.f, 0.f, 1, 0.0000001f, this, tr("Start of loop"));
	m_loopEndPointModel  [key]=new FloatModel(1.f, 0.f, 1, 0.0000001f, this, tr("End of loop"));
	m_reverseModel       [key]=new BoolModel (false, this, tr("Reverse sample"));
	m_loopModel          [key]=new IntModel  (0, 0, 2, this, tr("Loop mode"));
	m_stutterModel       [key]=new BoolModel (false, this, tr("Stutter"));
	//m_interpolationModel(this, tr("Interpolation mode"))

	connect(sample, SIGNAL(sampleUpdated()), this, SLOT(onSampleUpdated()));
	connect(m_reverseModel[key], SIGNAL(dataChanged()), this, SLOT(onReverseModelChanged()));
	connect(m_startPointModel[key], SIGNAL(dataChanged()), this, SLOT(onStartPointChanged()));
	connect(m_endPointModel[key], SIGNAL(dataChanged()), this, SLOT(onEndPointChanged()));
	connect(m_loopStartPointModel[key], SIGNAL(dataChanged()), this, SLOT(onLoopStartPointChanged()));
	connect(m_loopEndPointModel[key], SIGNAL(dataChanged()), this, SLOT(onLoopEndPointChanged()));
	connect(m_stutterModel[key], SIGNAL(dataChanged()), this, SLOT(onStutterModelChanged()));
	//connect(m_ampModel[key], SIGNAL(dataChanged()), this, SLOT(onAmpModelChanged()));

	//interpolation modes
	//m_interpolationModel.addItem(tr("None"));
	//m_interpolationModel.addItem(tr("Linear"));
	//m_interpolationModel.addItem(tr("Sinc"));
	//m_interpolationModel[key]->setValue(0);
}


void PadsGDX::destroyKey(int key) {
	if (key < 0 || key > 127 || !m_sampleBuffer[key]) {
		return;
	}
	SampleBuffer* old = m_sampleBuffer[key];
	m_sampleBuffer[key]=NULL;
	if (old) {
		disconnect(old, SIGNAL(sampleUpdated()), this, SLOT(onSampleUpdated()));
		delete old;
	}
	if (m_startPointModel    [key]) { delete m_startPointModel    [key]; m_startPointModel    [key]=NULL; }
	if (m_endPointModel      [key]) { delete m_endPointModel      [key]; m_endPointModel      [key]=NULL; }
	if (m_loopStartPointModel[key]) { delete m_loopStartPointModel[key]; m_loopStartPointModel[key]=NULL; }
	if (m_loopEndPointModel  [key]) { delete m_loopEndPointModel  [key]; m_loopEndPointModel  [key]=NULL; }
	if (m_reverseModel       [key]) { delete m_reverseModel       [key]; m_reverseModel       [key]=NULL; }
	if (m_loopModel          [key]) { delete m_loopModel          [key]; m_loopModel          [key]=NULL; }
	if (m_stutterModel       [key]) { delete m_stutterModel       [key]; m_stutterModel       [key]=NULL; }
}


int PadsGDX::currentKey() {
	return m_currentKey;
}


void PadsGDX::setCurrentKey(int key) {
	if (key < 0 || key > 127) {
		return;
	}

	if (key != m_currentKey) {
		//if (instrumentTrack()->baseNoteModel()->value()!=key)
		//instrumentTrack()->baseNoteModel()->setValue(key);
		m_currentKey = key;
		//qInfo("PadsGDX::setCurrentKey emit from %p keyUpdated",this);
		emit keyUpdated(key);
		//qInfo("PadsGDX::setCurrentKey emit dataChanged");
		//emit dataChanged();
		//qInfo("PadsGDX::setCurrentKey emit sampleUpdated");
		//emit sampleUpdated();
	}
}


SampleBuffer* PadsGDX::currentSample() {
        int key = currentKey();
		//instrumentTrack()->baseNoteModel()->value();
        //if (key < 0 || key > 127) return NULL;
        return m_sampleBuffer[key];
}


/*
void PadsGDX::setCurrentSample(SampleBuffer* sample) {
        int key = currentKey();
		//instrumentTrack()->baseNoteModel()->value();
        //qInfo("PadsGDX::setCurrentSample key=%d sample=%p", key, sample);
        //if (key < 0 || key > 127) return;
        SampleBuffer* old = m_sampleBuffer[key];
        m_sampleBuffer[key] = sample;
        if (old) delete old;
        //qInfo("PadsGDX::setCurrentSample emit dataChanged");
        emit dataChanged();
        destroyKey(key);
        createKey(key,sample);
}
*/

const QString PadsGDX::audioFile() {
	SampleBuffer* sample = currentSample();
	return (sample ? sample->audioFile() : "");
}

void PadsGDX::setAudioFile(const QString& file, bool rename) {
	SampleBuffer* sample=currentSample();
	//qInfo("PadsGDX::setAudioFile f=%s s=%p",qPrintable(file),sample);
	if (!sample) {
		/*
		sample = new SampleBuffer(file);
		connect(sample, SIGNAL(sampleUpdated()), this, SLOT(onSampleUpdated()));
		setCurrentSample(sample);
		*/
		int key = currentKey();
		destroyKey(key);
		createKey(key, file);
		emit dataChanged();
		onSampleUpdated();
	} else {
		sample->setAudioFile(file);
	}
	/*
	// is current channel-name equal to previous-filename??
	if (rename &&
		(instrumentTrack()->name() ==
		QFileInfo(sample->audioFile()).fileName() ||
		sample->audioFile().isEmpty()))
	{
		// then set it to new one
		instrumentTrack()->setName(QFileInfo(file).fileName());
	}
	// else we don't touch the track-name, because the user named it self
	*/
}

void PadsGDX::onSampleUpdated() {
	if (m_loading) {
		return;
	}
	//qInfo("PadsGDX::onSampleUpdated");
	onPointChanged();
	emit sampleUpdated();
}


void PadsGDX::onReverseModelChanged() {
	if (m_loading) {
		return;
	}

	int key = currentKey();
	SampleBuffer* sample = currentSample();

	if (sample) {
		sample->setReversed(m_reverseModel[key]->value());
		m_nextPlayStartPoint=sample->startFrame();
		m_nextPlayBackwards = false;
		emit dataChanged();
	}
}

/*
void PadsGDX::onAmpModelChanged(void) {
        if (m_loading) return;
        SampleBuffer* sample=currentSample();

        if (sample)
        {
                sample->setAmplification(m_ampModel[key]->value()/100.0f);
        }
}
*/

void PadsGDX::onStutterModelChanged() {
	if (m_loading) {
		return;
	}

	SampleBuffer* sample=currentSample();

	if (!sample) {
		m_nextPlayStartPoint=sample->startFrame();
		m_nextPlayBackwards = false;
	}
}

bool PadsGDX::checkPointBounds(int key) {
	if (key < 0 || key > 127 || m_loading || m_checking || m_sampleBuffer[key]) {
		return false;
	}

	SampleBuffer* sample = m_sampleBuffer[key]; Q_UNUSED(sample);

	m_checking = true;
	setJournalling(false);

	float old_start     = m_startPointModel    [key]->value();
	float old_end       = m_endPointModel      [key]->value();
	float old_loopStart = m_loopStartPointModel[key]->value();
	float old_loopEnd   = m_loopEndPointModel  [key]->value();

	const float MINSZ = 0.0001f;

	// check if start is over end and swap values if so
	if (m_startPointModel[key]->value() > m_endPointModel[key]->value()) {
		float tmp = m_endPointModel[key]->value();
		m_endPointModel[key]->setValue(m_startPointModel[key]->value());
		m_startPointModel[key]->setValue(tmp);
	}

	//m_startPointModel[key]->setValue(qBound(0.f,m_startPointModel[key]->value(), 1.f));
	//m_endPointModel[key]->setValue(qBound(0.f,m_endPointModel[key]->value(), 1.f));

	if (qAbs(m_startPointModel[key]->value()-m_endPointModel[key]->value()) < MINSZ) {
		m_endPointModel[key]->setValue(m_startPointModel[key]->value() + MINSZ);
		m_startPointModel[key]->setValue(m_endPointModel[key]->value() - MINSZ);
		//m_startPointModel[key]->setValue(qBound(0.f,m_startPointModel[key]->value(), 1.f));
		//m_endPointModel[key]->setValue(qBound(0.f,m_endPointModel[key]->value(), 1.f));
	}

	// check if start is over end and swap values if so
	if (m_loopStartPointModel[key]->value() > m_loopEndPointModel[key]->value()) {
		float tmp = m_loopEndPointModel[key]->value();
		m_loopEndPointModel[key]->setValue(m_loopStartPointModel[key]->value());
		m_loopStartPointModel[key]->setValue(tmp);
	}

	m_loopStartPointModel[key]->setValue(qBound(
		m_startPointModel[key]->value(),
		m_loopStartPointModel[key]->value(),
		m_endPointModel[key]->value()
	));

	m_loopEndPointModel[key]->setValue(qBound(
		m_startPointModel[key]->value(),
		m_loopEndPointModel[key]->value(),
		m_endPointModel[key]->value()
	));

	if (qAbs(m_loopStartPointModel[key]->value() - m_loopEndPointModel[key]->value()) < MINSZ) {
		m_loopEndPointModel[key]->setValue(m_loopStartPointModel[key]->value() + MINSZ);
		m_loopStartPointModel[key]->setValue(m_loopEndPointModel[key]->value() - MINSZ);

		m_loopStartPointModel[key]->setValue(qBound(
			m_startPointModel[key]->value(),
			m_loopStartPointModel[key]->value(),
			m_endPointModel[key]->value()
		));

		m_loopEndPointModel[key]->setValue(qBound(
			m_startPointModel[key]->value(),
			m_loopEndPointModel[key]->value(),
			m_endPointModel[key]->value()
		));
	}

	setJournalling(true);
	m_checking = false;

	return	(old_start     != m_startPointModel    [key]->value()) ||
			(old_end       != m_endPointModel      [key]->value()) ||
			(old_loopStart != m_loopStartPointModel[key]->value()) ||
			(old_loopEnd   != m_loopEndPointModel  [key]->value());
}


void PadsGDX::onStartPointChanged(void) {
        onPointChanged();
}


void PadsGDX::onEndPointChanged() {
        onPointChanged();
}


void PadsGDX::onLoopStartPointChanged() {
        onPointChanged();
}

void PadsGDX::onLoopEndPointChanged() {
        onPointChanged();
}

void PadsGDX::onPointChanged() {
	int key = currentKey();;
	if (m_loading || key < 0 || key > 127 || !currentSample()) {
		return;
	}

	checkPointBounds(key);
	SampleBuffer* sample = currentSample();
	//qInfo("PadsGDX::onPointChanged sample=%p", sample);

	const int lastFrame = sample->frames() - 1;
	f_cnt_t f_start     = static_cast<f_cnt_t>(m_startPointModel    [key]->value() * lastFrame);
	f_cnt_t f_end       = static_cast<f_cnt_t>(m_endPointModel      [key]->value() * lastFrame);
	f_cnt_t f_loopStart = static_cast<f_cnt_t>(m_loopStartPointModel[key]->value() * lastFrame);
	f_cnt_t f_loopEnd   = static_cast<f_cnt_t>(m_loopEndPointModel  [key]->value() * lastFrame);

	m_nextPlayStartPoint = f_start;
	m_nextPlayBackwards  = false;

	sample->setAllPointFrames(f_start, f_end, f_loopStart, f_loopEnd);
	//qInfo("POINTS s=%d e=%d ls=%d le=%d",f_start,f_end,f_loopStart,f_loopEnd);
	emit dataChanged();
}


void PadsGDX::saveSettings(QDomDocument& doc, QDomElement& element) {
	QDomElement samples = doc.createElement("samples");
	element.appendChild(samples);
	samples.setAttribute("key", currentKey());
	for (int i = 0; i < 128; i++) {
		SampleBuffer* sample = m_sampleBuffer[i];
		if (!sample) {
			continue;
		}

		QDomElement e = doc.createElement("sample");
		samples.appendChild(e);
		e.setAttribute("key", i);
		QString file=sample->audioFile();
		if (file.isEmpty()) {
			QString s;
			e.setAttribute("data", sample->toBase64(s));
		} else {
			file=SampleBuffer::tryToMakeRelative(file);
			e.setAttribute("src", file);
		}

		m_reverseModel        [i]->saveSettings(doc, e, "reversed");
		m_loopModel           [i]->saveSettings(doc, e, "looped");
		//m_ampModel          [i]->saveSettings(doc, e, "amp");
		m_stutterModel        [i]->saveSettings(doc, e, "stutter");
		//m_interpolationModel[i]->saveSettings(doc, e, "interp");
		m_startPointModel     [i]->saveSettings(doc, e, "start");
		m_endPointModel       [i]->saveSettings(doc, e, "end");
		m_loopStartPointModel [i]->saveSettings(doc, e, "loopstart");
		m_loopEndPointModel   [i]->saveSettings(doc, e, "loopend");
	}
}




void PadsGDX::loadSettings(const QDomElement & element) {
	m_loading = true;
	QDomNode samples = element.firstChildElement("samples");
	if (samples.isNull() || !samples.isElement()) {
		return;
	}
	QDomElement e=samples.firstChildElement("sample");
	while (!e.isNull()) {
		int i=e.attribute("key").toInt();
		if (i < 0 || i > 127) {
			//qInfo("PadsGDX::loadSettings invalid key=%d",i);
			e = e.nextSibling().toElement();
			continue;
		}

		destroyKey(i);

		QString file = e.attribute("src");
		QString data = e.attribute("data");

		if (!file.isEmpty()) {
			QString p = SampleBuffer::tryToMakeAbsolute(file);
			if (!QFileInfo(p).exists()) {
				QString message = tr("Sample not found: %1").arg(file);
				Engine::getSong()->collectError(message);
				e = e.nextSibling().toElement();
				continue;
			}
		} else {
			if (data.isEmpty()) {
				e = e.nextSibling().toElement();
				continue;
			}
		}

		SampleBuffer* sample=NULL;
		if (!file.isEmpty()) {
			sample = new SampleBuffer(file);
		} else {
			if (!data.isEmpty()) {
				sample = new SampleBuffer();
				sample->loadFromBase64(data);
			} else {
				e=e.nextSibling().toElement();
				continue;
			}
		}

		createKey(i, sample);

		m_reverseModel        [i]->loadSettings(element, "reversed");
		m_loopModel           [i]->loadSettings(element, "looped");
		//m_ampModel          [i]->loadSettings(element, "amp");
		m_stutterModel        [i]->loadSettings(element, "stutter");
		//m_interpolationModel[i]->loadSettings(element, "interp");
		m_startPointModel     [i]->loadSettings(element, "start");
		m_endPointModel       [i]->loadSettings(element, "end");
		m_loopStartPointModel [i]->loadSettings(element, "loopStart");
		m_loopEndPointModel   [i]->loadSettings(element, "loopEnd");

		/*
		if (!m_sampleBuffer[i]) {
			m_sampleBuffer[i] = sample;
			connect(sample, SIGNAL(sampleUpdated()), this, SLOT(onSampleUpdated()));
		}
		*/

		e = e.nextSibling().toElement();
	}

	m_loading = false;
	int k = samples.toElement().attribute("key").toInt();
	if (k != currentKey()) {
		setCurrentKey(k);
	} else {
		onSampleUpdated();
		onPointChanged();
	}
}


extern "C" {
// necessary for getting instance out of shared lib
Plugin* PLUGIN_EXPORT lmms_plugin_main(Model *, void* data) {
	return new PadsGDX(static_cast<InstrumentTrack*>(data));
}
}
