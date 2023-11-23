/*
 * Voxpop.h - declaration of class Voxpop
 *             (voice instrument-plugin)
 * This is a sample palyer that also has a cue sheet of start,
 * and optionally end points with text titles.  There are 3
 * play modes by which users can trigger samples at these cue
 * points.
 *
 * Copyright (c) 2023 Teknopaul <teknopaul/at/fastmail.es>
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

#ifndef LMMS_VOXPOP_H
#define LMMS_VOXPOP_H

#include <QPixmap>
#include <QCheckBox>

#include "ComboBoxModel.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "SampleBuffer.h"
#include "Knob.h"
#include "Controls.h"


namespace lmms
{

const int VOXPOP_MAX_CUES = 99;
const QString VOXPOP_DEFAULT_TEXT = "";

namespace gui
{
class automatableButtonGroup;
class PluginView;
class InstrumentViewFixedSize;
class Knob;
class PixmapButton;
class ComboBox;
class VoxpopView;
}

class VoxpopHandleState : public SampleBuffer::handleState
{
public:
	VoxpopHandleState(bool varyingPitch = false, int interpolationMode = SRC_LINEAR) : handleState(varyingPitch, interpolationMode)
	{
		cue = 0;
	}
	VoxpopHandleState(int q, bool varyingPitch = false, int interpolationMode = SRC_LINEAR) : handleState(varyingPitch, interpolationMode)
	{
		cue = q;
	}
	int cue;
};

class Voxpop : public Instrument
{
	Q_OBJECT
public:
	Voxpop( InstrumentTrack * _instrument_track );

	enum class CueSelectionMode
	{
		Automation,
		Piano,
		Sequential
	};

	void playNote( NotePlayHandle * _n,
						sampleFrame * _working_buffer ) override;
	void deleteNotePluginData( NotePlayHandle * _n ) override;

	void saveSettings(QDomDocument& doc, QDomElement& elem) override;
	void loadSettings(const QDomElement& elem) override;

	void loadFile( const QString & _file ) override;
	void loadAudioFile( const QString & _file );
	void loadCuesheetFile( const QString & _file );

	QString nodeName() const override;

	QString * sampleText()
	{
		return m_cueCount > 0 ?  m_cueTexts[m_cueIndexModel.value()] : (QString *) &VOXPOP_DEFAULT_TEXT;
	}

	auto beatLen(NotePlayHandle* note) const -> int override;

	f_cnt_t desiredReleaseFrames() const override
	{
		return 128;
	}

	gui::PluginView* instantiateView( QWidget * _parent ) override;

public slots:
	void setAudioFile( const QString & _audio_file, bool _rename = true );
	void setCuesheetFile( const QString & _cuesheet_file, bool _rename = true );
	void resetStutter()
	{
		for (int i = 0 ; i < m_cueCount ; i++)
		{
			m_nextPlayStartPoint[i] = 0;
		}
	}
	void resetStutter(uint idx)
	{
		if (m_cueCount > 0 && idx <= m_cueCount )
		{
			m_nextPlayStartPoint[idx] = 0;
		}
	}
	void resetCueIndex()
	{
		if (m_cueCount > 0)
		{
			m_cueIndexModel.setValue( 0 );
			cueIndexChanged();
		}
	}
	void setCueIndex(int idx)
	{
		m_cueIndexModel.setValue( idx );
		cueIndexChanged();
	}

private slots:
	void ampModelChanged();
	void stutterModelChanged();
	void cueIndexChanged();
	void modeChanged();


signals:
	void cueChanged(int cue, QString * text);

private:
	using handleState = SampleBuffer::handleState;
	CueSelectionMode m_mode;

	BoolModel m_respectEndpointModel;
	FloatModel m_ampModel;
	IntModel m_cueIndexModel;
	mutable QReadWriteLock m_idxLock;
	BoolModel m_stutterModel;
	ComboBoxModel m_interpolationModel;
	ComboBoxModel m_modeModel;

	QString m_audioFile;
	QString m_cuesheetFile;
	int m_cueCount;
	SampleBuffer m_sampleBuffer;                 // used for view
	std::vector<SampleBuffer *> m_sampleBuffers;  // used to play
	std::vector<QString *> m_cueTexts;
	std::vector<f_cnt_t> m_cueOffsets;
	std::vector<f_cnt_t> m_nextPlayStartPoint;

	friend class gui::VoxpopView;
	
	bool reloadCuesheet();
	void deleteSamples(int count);
	f_cnt_t cuePointToFrames(QString field);

} ;


namespace gui
{

class VoxpopWaveView;


class VoxpopView : public gui::InstrumentViewFixedSize
{
	Q_OBJECT
public:
	VoxpopView( Instrument * _instrument, QWidget * _parent );
	virtual ~VoxpopView() = default;

	void newWaveView();

protected slots:
	void sampleUpdated();
	void openCuesheetFile();
	void openAudioFile();
	void cueSheetChanged();
	void cueChanged(int cue, QString * text);


protected:
	virtual void paintEvent( QPaintEvent * );
	void updateCuePoints();

private:
	virtual void modelChanged();

	static QPixmap * s_artwork;

	LedCheckBox * m_respectEnpointsCheckBox;
	QString * m_cuelabel;
	Knob * m_ampKnob;
	LcdSpinBox * m_cueIndexControl;

	gui::PixmapButton * m_openAudioFileButton;
	gui::PixmapButton * m_openCuesheetFileButton;

	PixmapButton * m_stutterButton;
	ComboBox * m_interpBox;
	ComboBox * m_modeBox;

	VoxpopWaveView * m_waveView;
} ;



class VoxpopWaveView : public QWidget
{
	Q_OBJECT
protected:
	virtual void paintEvent( QPaintEvent * _pe );


public:

public slots:
	void update()
	{
		updateGraph();
		QWidget::update();
	}

	void isPlaying( lmms::f_cnt_t _current_frame );


private:
	static const int s_padding = 2;

	SampleBuffer& m_sampleBuffer;
	QPixmap m_graph;
	f_cnt_t m_from;
	f_cnt_t m_to;

public:
	VoxpopWaveView( QWidget * _parent, int _w, int _h, SampleBuffer& buf );

private:
	void updateGraph();

} ;


} // namespace gui

} // namespace lmms

#endif // LMMS_VOXPOP_H
