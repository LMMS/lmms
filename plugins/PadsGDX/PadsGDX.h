/*
 * PadsGDX.h - sample player for pads
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


#ifndef PADS_GDX_H
#define PADS_GDX_H

#include <QObject>
//#include <QPixmap>

//#include "Plugin.h"
#include "Instrument.h"
//#include "InstrumentView.h"
#include "InstrumentTrack.h"
#include "MemoryManager.h"
#include "NotePlayHandle.h"
#include "SampleBuffer.h"
//#include "Knob.h"
//#include "PixmapButton.h"
//#include "AutomatableButton.h"
//#include "ComboBox.h"

class PadsGDX : public Instrument
{
	Q_OBJECT
	MM_OPERATORS

 public:
	PadsGDX(InstrumentTrack* _track);
	virtual ~PadsGDX();

	virtual void playNote(NotePlayHandle* _n, sampleFrame* _buffer);
	virtual void deleteNotePluginData(NotePlayHandle* _n);

	virtual void saveSettings(QDomDocument& _doc, QDomElement& _parent);
	virtual void loadSettings(const QDomElement& _this);

        virtual const QString audioFile();
	virtual void loadFile(const QString& _file);

	virtual QString nodeName() const;

	virtual int getBeatLen(NotePlayHandle* _n) const;

	virtual f_cnt_t desiredReleaseFrames() const
	{
		return 128;
	}

	virtual PluginView* instantiateView(QWidget * _parent);

 signals:
        //void dataChanged();
        void sampleUpdated();
        void isPlaying(f_cnt_t);
        void keyUpdated(int);

 public slots:
        virtual void setAudioFile(const QString& _audio_file,
                                  bool _rename=true);
        virtual void onSampleUpdated();
        virtual void onReverseModelChanged();
        //virtual void onAmpModelChanged();
        virtual void onStartPointChanged();
        virtual void onEndPointChanged();
        virtual void onLoopStartPointChanged();
        virtual void onLoopEndPointChanged();
        virtual void onPointChanged();
        virtual void onStutterModelChanged();

 private:
        void createKey(int _key,const QString& _fileName);
        void createKey(int _key,SampleBuffer* _sample);
        void destroyKey(int _key);
        int  currentKey();
        void setCurrentKey(int _key);
        SampleBuffer* currentSample();
        //void setCurrentSample(SampleBuffer* _sample);
        bool checkPointBounds(int _key);

	typedef SampleBuffer::handleState handleState;

        int           m_currentKey;
	SampleBuffer* m_sampleBuffer[128];
	//FloatModel* m_ampModel[128];
	FloatModel*   m_startPointModel[128];
	FloatModel*   m_endPointModel[128];
	FloatModel*   m_loopStartPointModel[128];
	FloatModel*   m_loopEndPointModel[128];
	BoolModel*    m_reverseModel[128];
	IntModel*     m_loopModel[128];
	BoolModel*    m_stutterModel[128];
        bool          m_checking;
        bool          m_loading;

	//ComboBoxModel m_interpolationModel;
	f_cnt_t       m_nextPlayStartPoint;
	bool          m_nextPlayBackwards;

	friend class PadsGDXView;

};



#endif
