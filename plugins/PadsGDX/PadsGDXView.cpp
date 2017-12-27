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


#include <QObject>
#include <QDir>
#include <QPixmap>
#include <QPainter>
#include <QRegExp>

#include "debug.h"
#include "embed.h"

#include "PadsGDX.h"
#include "PadsGDXView.h"
#include "PadsGDXWaveView.h"

#include "Engine.h"
#include "Song.h"
#include "ToolTip.h"


QPixmap * PadsGDXView::s_artwork = NULL;


PadsGDXView::PadsGDXView( Instrument * _instrument, QWidget * _parent ) :
	InstrumentView( _instrument, _parent )
{
	if( s_artwork==NULL )
	{
		s_artwork=new QPixmap(PLUGIN_NAME::getIconPixmap("artwork"));
	}

	m_startKnob = new PadsGDXWaveView::knob( this );
	m_startKnob->move(6,126);
	m_startKnob->setHintText( tr( "Startpoint:" ), "" );
	m_startKnob->setWhatsThis
                (tr( "With this knob you can set the point where "
                     "PadsGDX should begin playing your sample. " ));

	m_loopStartKnob = new PadsGDXWaveView::knob( this );
	m_loopStartKnob->move(46,126);
	m_loopStartKnob->setHintText( tr( "Loopback point:" ), "" );
	m_loopStartKnob->setWhatsThis
		(tr( "With this knob you can set the point where "
                     "the loop starts. " ));

	m_loopEndKnob = new PadsGDXWaveView::knob( this );
	m_loopEndKnob->move(86,126);
	m_loopEndKnob->setHintText( tr( "Loopback point:" ), "" );
	m_loopEndKnob->setWhatsThis
		(tr( "With this knob you can set the point where "
                     "the loop ends. " ));

	m_endKnob = new PadsGDXWaveView::knob( this );
	m_endKnob->move(126,126);
	m_endKnob->setHintText( tr( "Endpoint:" ), "" );
	m_endKnob->setWhatsThis
		(tr( "With this knob you can set the point where "
                     "PadsGDX should stop playing your sample. " ));

        /*
	m_ampKnob = new Knob( knobBright_26, this );
	m_ampKnob->setVolumeKnob( true );
	m_ampKnob->move(6,6);
	m_ampKnob->setHintText( tr( "Amplify:" ), "%" );
	m_ampKnob->setWhatsThis
		(tr( "With this knob you can set the amplify ratio. When you "
                     "set a value of 100% your sample isn't changed. "
                     "Otherwise it will be amplified up or down (your "
                     "actual sample-file isn't touched!)" ));
        */

        // interpolation selector
        /*
	m_interpBox = new ComboBox( this );
	m_interpBox->setGeometry(46,6,82,22);
	m_interpBox->setFont( pointSize<8>( m_interpBox->font() ) );
        m_interpBox->hide();
        */

	PixmapButton * m_loopOffButton = new PixmapButton( this );
	m_loopOffButton->setCheckable( true );
	m_loopOffButton->move(46,46);
	m_loopOffButton->setActiveGraphic  (PLUGIN_NAME::getIconPixmap("loop_off_on" ));
	m_loopOffButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("loop_off_off"));
	ToolTip::add( m_loopOffButton, tr( "Disable loop" ) );
	m_loopOffButton->setWhatsThis
		(tr( "This button disables looping. "
                     "The sample plays only once from start to end. " ));


	PixmapButton * m_loopOnButton = new PixmapButton( this );
	m_loopOnButton->setCheckable( true );
	m_loopOnButton->move(66,46);
	m_loopOnButton->setActiveGraphic  (PLUGIN_NAME::getIconPixmap("loop_on_on" ));
	m_loopOnButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("loop_on_off"));
	ToolTip::add( m_loopOnButton, tr( "Enable loop" ) );
	m_loopOnButton->setWhatsThis
		(tr( "This button enables forwards-looping. "
                     "The sample loops between the end point and the loop point." ));

	PixmapButton * m_loopPingPongButton = new PixmapButton( this );
	m_loopPingPongButton->setCheckable( true );
	m_loopPingPongButton->move(86,46);
	m_loopPingPongButton->setActiveGraphic  (PLUGIN_NAME::getIconPixmap("loop_pingpong_on" ));
	m_loopPingPongButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("loop_pingpong_off"));
	ToolTip::add( m_loopPingPongButton, tr( "Enable loop" ) );
	m_loopPingPongButton->setWhatsThis
                (tr( "This button enables ping-pong-looping. "
                     "The sample loops backwards and forwards between the end point "
                     "and the loop point." ));

	m_loopGroup = new automatableButtonGroup( this );
	m_loopGroup->addButton( m_loopOffButton );
	m_loopGroup->addButton( m_loopOnButton );
	m_loopGroup->addButton( m_loopPingPongButton );

	m_reverseButton = new PixmapButton( this );
	m_reverseButton->setCheckable( true );
	m_reverseButton->move(116,46);
	m_reverseButton->setActiveGraphic  (PLUGIN_NAME::getIconPixmap("reverse_on" ));
	m_reverseButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("reverse_off"));
	ToolTip::add( m_reverseButton, tr( "Reverse sample" ) );
	m_reverseButton->setWhatsThis
                (tr( "If you enable this button, the whole sample is reversed. "
                     "This is useful for cool effects, e.g. a reversed "
                     "crash." ));

	m_stutterButton = new PixmapButton( this );
	m_stutterButton->setCheckable( true );
	m_stutterButton->move(146,46);
	m_stutterButton->setActiveGraphic  (PLUGIN_NAME::getIconPixmap("stutter_on" ));
	m_stutterButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("stutter_off"));
	ToolTip::add( m_stutterButton, tr( "Continue sample playback across notes" ) );
	m_stutterButton->setWhatsThis
		(tr( "Enabling this option makes the sample continue playing "
                     "across different notes - if you change pitch, or the note "
                     "length stops before the end of the sample, then the next "
                     "note played will continue where it left off. To reset the "
                     "playback to the start of the sample, insert a note at the bottom "
                     "of the keyboard (< 20 Hz)" ));

	m_openAudioFileButton = new PixmapButton( this );
	m_openAudioFileButton->setCursor( QCursor( Qt::PointingHandCursor ) );
	m_openAudioFileButton->move(46,106);
	m_openAudioFileButton->setActiveGraphic  (PLUGIN_NAME::getIconPixmap("select_file"));
	m_openAudioFileButton->setInactiveGraphic(PLUGIN_NAME::getIconPixmap("select_file"));
	connect( m_openAudioFileButton, SIGNAL( clicked() ),
                 this, SLOT( openAudioFile() ) );
	ToolTip::add( m_openAudioFileButton, tr( "Open other sample" ) );
	m_openAudioFileButton->setWhatsThis
                (tr( "Click here, if you want to open another audio-file. "
                     "A dialog will appear where you can select your file. "
                     "Settings like looping-mode, start and end-points, "
                     "amplify-value, and so on are not reset. So, it may not "
                     "sound like the original sample." ));

        // wavegraph
	m_waveView=new PadsGDXWaveView(this,245,75);
	m_waveView->move(2,172);
        m_waveView->setKnobs(dynamic_cast<PadsGDXWaveView::knob*>(m_startKnob    ),
                             dynamic_cast<PadsGDXWaveView::knob*>(m_endKnob      ),
                             dynamic_cast<PadsGDXWaveView::knob*>(m_loopStartKnob),
                             dynamic_cast<PadsGDXWaveView::knob*>(m_loopEndKnob  ));

	setAcceptDrops( true );

        updateWaveView(true);
}




PadsGDXView::~PadsGDXView()
{
	if(m_waveView)
	{
		delete m_waveView;
		m_waveView=NULL;
	}
}


void PadsGDXView::doConnections()
{
        PadsGDX* a=castModel<PadsGDX>();
        //qInfo("PadsGDXView::PadsGDXView connect from %p to %p",a,this);

	DEBUG_CONNECT( a, SIGNAL(isPlaying(f_cnt_t)),
                       m_waveView, SLOT(onPlaying(f_cnt_t)) );

	DEBUG_CONNECT( a, SIGNAL( keyUpdated(int)),
                       this, SLOT( onKeyUpdated(int)) );
        //DEBUG_CONNECT( a, &PadsGDX::keyUpdated,
        //             this, &PadsGDXView::onKeyUpdated );

	DEBUG_DISCONNECT( a, SIGNAL( dataChanged() ),
                          this, SLOT( update() ) );
	DEBUG_CONNECT( a, SIGNAL( dataChanged() ),
                       this, SLOT( onModelChanged() ) );

	DEBUG_CONNECT( a, SIGNAL(sampleUpdated( )),
                       this, SLOT(onSampleUpdated()) );

	//DEBUG_CONNECT( a, SIGNAL(keyUpdated(int)),
        //                 qApp, SLOT(aboutQt()) );
}


void PadsGDXView::onKeyUpdated(int _key)
{
        qInfo("PadsGDXView::onKeyUpdated");
        onModelChanged();
        onSampleUpdated();
}


void PadsGDXView::onModelChanged()
{
        //qInfo("PadsGDXView::onModelChanged");
	PadsGDX* a=castModel<PadsGDX>();
        int key=(a ? a->currentKey() : -1);
        if(key<0 || key>127 || !a->currentSample())
        {
                //qInfo("PadsGDXView::onModelChanged key not set");
                m_startKnob    ->setModel(new FloatModel(0,0,0,1,NULL,"",true));
                m_endKnob      ->setModel(new FloatModel(0,0,0,1,NULL,"",true));
                m_loopStartKnob->setModel(new FloatModel(0,0,0,1,NULL,"",true));
                m_loopEndKnob  ->setModel(new FloatModel(0,0,0,1,NULL,"",true));
                m_reverseButton->setModel(new BoolModel (false  ,NULL,"",true));
                m_loopGroup    ->setModel(new IntModel  (0,0,0  ,NULL,"",true));
                m_stutterButton->setModel(new BoolModel (false  ,NULL,"",true));
        }
        else
        {
                m_startKnob    ->setModel(a->m_startPointModel     [key]);
                m_endKnob      ->setModel(a->m_endPointModel       [key]);
                m_loopStartKnob->setModel(a->m_loopStartPointModel [key]);
                m_loopEndKnob  ->setModel(a->m_loopEndPointModel   [key]);
                m_reverseButton->setModel(a->m_reverseModel        [key]);
                m_loopGroup    ->setModel(a->m_loopModel           [key]);
                m_stutterButton->setModel(a->m_stutterModel        [key]);
                //m_ampKnob      ->setModel( &a->m_ampModel            );
                //m_interpBox    ->setModel( &a->m_interpolationModel  );
        }

        updateWaveView(false);

        //SampleBuffer* sample=a->currentSample();
        //if(sample)
        //      CHECKED_CONNECT( sample, SIGNAL( sampleUpdated() ),
        //               this, SLOT( sampleUpdated() ) );
}


void PadsGDXView::onSampleUpdated()
{
        //qInfo("PadsGDXView::onSampleUpdated");
        updateWaveView(true);
}


void PadsGDXView::updateWaveView(bool _full)
{
        //qInfo("PadsGDXView::updateWaveView");

        SampleBuffer* sample=castModel<PadsGDX>()->currentSample();

	if(sample!=m_waveView->sample())
        {
                m_waveView->setSample(sample);

                if(sample)
                {
                        //m_startKnob     ->show();
                        //m_endKnob       ->show();
                        //m_loopStartKnob ->show();
                        //m_loopEndKnob   ->show();
                        m_waveView      ->show();
                }
                else
                {
                        //m_startKnob     ->hide();
                        //m_endKnob       ->hide();
                        //m_loopStartKnob ->hide();
                        //m_loopEndKnob   ->hide();
                        m_waveView      ->hide();
                }
        }

        if(_full) m_waveView->fullUpdate();
        else      m_waveView->update();
        update();
}




void PadsGDXView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( _dee->mimeData()->hasFormat( StringPairDrag::mimeType() ) )
	{
		QString txt = _dee->mimeData()->data(StringPairDrag::mimeType() );
		if(txt.section(':',0,0)==QString("tco_%1").arg(Track::SampleTrack))
		{
			_dee->acceptProposedAction();
		}
		else
                if(txt.section(':',0,0)=="samplefile")
		{
			_dee->acceptProposedAction();
		}
		else
		{
			_dee->ignore();
		}
	}
	else
	{
		_dee->ignore();
	}
}




void PadsGDXView::dropEvent(QDropEvent* _de)
{
	QString type =StringPairDrag::decodeKey(_de);
	QString value=StringPairDrag::decodeValue(_de);

        //qInfo("PadsGDXView::dropEvent %s %s",qPrintable(type),qPrintable(value));

	if(type=="samplefile")
	{
		_de->accept();
		castModel<PadsGDX>()->setAudioFile(value);
		m_waveView->setSample(castModel<PadsGDX>()->currentSample());
		return;
	}
	else
        if(type==QString("tco_%1").arg(Track::SampleTrack))
	{
		_de->accept();
		DataFile dataFile(value.toUtf8());
                QString fileName=dataFile.content().firstChild().toElement().attribute("src");
		castModel<PadsGDX>()->setAudioFile(fileName);
		m_waveView->setSample(castModel<PadsGDX>()->currentSample());
		return;
	}

	_de->ignore();
}




void PadsGDXView::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.drawPixmap(0,0,*s_artwork );

	PadsGDX* a=castModel<PadsGDX>();
        SampleBuffer* sample=a->currentSample();

        if(sample)
        {
                QString fileName=sample->audioFile();
                QString length  =QString("%1 ms").arg(sample->sampleLength());
                fileName=fileName.replace(QRegExp(QString("^.*%1").arg(QDir::separator())),"");

                p.setFont(pointSize<8>(font()));
                p.setPen(Qt::white);
                p.drawText(46,86+8,fileName);
                p.drawText(46,86+18,length);
        }
}




void PadsGDXView::openAudioFile()
{
        SampleBuffer* sample=castModel<PadsGDX>()->currentSample();
        QString file;
        if(sample) file=sample->audioFile();
        file=SampleBuffer::selectAudioFile(file);

	if(file!="")
	{
                int key=castModel<PadsGDX>()->currentKey();
                castModel<PadsGDX>()->createKey(key,file);
		//castModel<PadsGDX>()->setAudioFile(f);
		Engine::getSong()->setModified();
		updateWaveView(true);
                //m_waveView->setSample(castModel<PadsGDX>()->currentSample());
	}
}
