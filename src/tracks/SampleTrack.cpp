/*
 * SampleTrack.cpp - implementation of class SampleTrack, a track which
 *                   provides arrangement of samples
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "SampleTrack.h"

#include <QDropEvent>
#include <QFileInfo>
#include <QMenu>
#include <QLayout>
#include <QLineEdit>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPainter>
#include <QPushButton>

#include "gui_templates.h"
#include "GuiApplication.h"
#include "Song.h"
#include "embed.h"
#include "ToolTip.h"
#include "BBTrack.h"
#include "SamplePlayHandle.h"
#include "SampleRecordHandle.h"
#include "SongEditor.h"
#include "StringPairDrag.h"
#include "TimeLineWidget.h"
#include "Knob.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "EffectRackView.h"
#include "FxMixerView.h"
#include "TabWidget.h"
#include "TrackLabelButton.h"

SampleTCO::SampleTCO( Track * _track ) :
	TrackContentObject( _track ),
	m_sampleBuffer( new SampleBuffer ),
	m_isPlaying( false )
{
	saveJournallingState( false );
	setSampleFile( "" );
	restoreJournallingState();

	// we need to receive bpm-change-events, because then we have to
	// change length of this TCO
	connect( Engine::getSong(), SIGNAL( tempoChanged( bpm_t ) ),
					this, SLOT( updateLength() ), Qt::DirectConnection );
	connect( Engine::getSong(), SIGNAL( timeSignatureChanged( int,int ) ),
					this, SLOT( updateLength() ) );

	//care about positionmarker
	TimeLineWidget * timeLine = Engine::getSong()->getPlayPos( Engine::getSong()->Mode_PlaySong ).m_timeLine;
	if( timeLine )
	{
		connect( timeLine, SIGNAL( positionMarkerMoved() ), this, SLOT( playbackPositionChanged() ) );
	}
	//playbutton clicked or space key / on Export Song set isPlaying to false
	connect( Engine::getSong(), SIGNAL( playbackStateChanged() ),
			this, SLOT( playbackPositionChanged() ), Qt::DirectConnection );
	//care about loops
	connect( Engine::getSong(), SIGNAL( updateSampleTracks() ),
			this, SLOT( playbackPositionChanged() ), Qt::DirectConnection );
	//care about mute TCOs
	connect( this, SIGNAL( dataChanged() ), this, SLOT( playbackPositionChanged() ) );
	//care about mute track
	connect( getTrack()->getMutedModel(), SIGNAL( dataChanged() ),
			this, SLOT( playbackPositionChanged() ), Qt::DirectConnection );
	//care about TCO position
	connect( this, SIGNAL( positionChanged() ), this, SLOT( updateTrackTcos() ) );

	switch( getTrack()->trackContainer()->type() )
	{
		case TrackContainer::BBContainer:
			setAutoResize( true );
			break;

		case TrackContainer::SongContainer:
			// move down
		default:
			setAutoResize( false );
			break;
	}
	updateTrackTcos();
}




SampleTCO::~SampleTCO()
{
	SampleTrack * sampletrack = dynamic_cast<SampleTrack*>( getTrack() );
	if ( sampletrack )
	{
		sampletrack->updateTcos();
	}
	Engine::mixer()->requestChangeInModel();
	sharedObject::unref( m_sampleBuffer );
	Engine::mixer()->doneChangeInModel();
}




void SampleTCO::changeLength( const MidiTime & _length )
{
	TrackContentObject::changeLength( qMax( static_cast<int>( _length ), 1 ) );
}




const QString & SampleTCO::sampleFile() const
{
	return m_sampleBuffer->audioFile();
}



void SampleTCO::setSampleBuffer( SampleBuffer* sb )
{
	Engine::mixer()->requestChangeInModel();
	sharedObject::unref( m_sampleBuffer );
	Engine::mixer()->doneChangeInModel();
	m_sampleBuffer = sb;
	updateLength();

	emit sampleChanged();
}



void SampleTCO::setSampleFile( const QString & _sf )
{
	int length;
	if ( _sf.isEmpty() )
	{	//When creating an empty sample pattern make it a bar long
		float nom = Engine::getSong()->getTimeSigModel().getNumerator();
		float den = Engine::getSong()->getTimeSigModel().getDenominator();
		length = DefaultTicksPerBar * ( nom / den );
	}
	else
	{	//Otherwise set it to the sample's length
		m_sampleBuffer->setAudioFile( _sf );
		length = sampleLength();
	}
	changeLength(length);

	setStartTimeOffset( 0 );

	emit sampleChanged();
	emit playbackPositionChanged();
}




void SampleTCO::toggleRecord()
{
	m_recordModel.setValue( !m_recordModel.value() );
	emit dataChanged();
}




void SampleTCO::playbackPositionChanged()
{
	Engine::mixer()->removePlayHandlesOfTypes( getTrack(), PlayHandle::TypeSamplePlayHandle );
	SampleTrack * st = dynamic_cast<SampleTrack*>( getTrack() );
	st->setPlayingTcos( false );
}




void SampleTCO::updateTrackTcos()
{
	SampleTrack * sampletrack = dynamic_cast<SampleTrack*>( getTrack() );
	if( sampletrack)
	{
		sampletrack->updateTcos();
	}
}




bool SampleTCO::isPlaying() const
{
	return m_isPlaying;
}




void SampleTCO::setIsPlaying(bool isPlaying)
{
	m_isPlaying = isPlaying;
}




void SampleTCO::updateLength()
{
	emit sampleChanged();
}




MidiTime SampleTCO::sampleLength() const
{
	return (int)( m_sampleBuffer->frames() / Engine::framesPerTick() );
}




void SampleTCO::setSampleStartFrame(f_cnt_t startFrame)
{
	m_sampleBuffer->setStartFrame( startFrame );
}




void SampleTCO::setSamplePlayLength(f_cnt_t length)
{
	m_sampleBuffer->setEndFrame( length );
}




void SampleTCO::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	if( _this.parentNode().nodeName() == "clipboard" )
	{
		_this.setAttribute( "pos", -1 );
	}
	else
	{
		_this.setAttribute( "pos", startPosition() );
	}
	_this.setAttribute( "len", length() );
	_this.setAttribute( "muted", isMuted() );
	_this.setAttribute( "src", sampleFile() );
	_this.setAttribute( "off", startTimeOffset() );
	if( sampleFile() == "" )
	{
		QString s;
		_this.setAttribute( "data", m_sampleBuffer->toBase64( s ) );
	}

	_this.setAttribute ("sample_rate", m_sampleBuffer->sampleRate());
	// TODO: start- and end-frame
}




void SampleTCO::loadSettings( const QDomElement & _this )
{
	if( _this.attribute( "pos" ).toInt() >= 0 )
	{
		movePosition( _this.attribute( "pos" ).toInt() );
	}
	setSampleFile( _this.attribute( "src" ) );
	if( sampleFile().isEmpty() && _this.hasAttribute( "data" ) )
	{
		m_sampleBuffer->loadFromBase64( _this.attribute( "data" ) );
	}
	changeLength( _this.attribute( "len" ).toInt() );
	setMuted( _this.attribute( "muted" ).toInt() );
	setStartTimeOffset( _this.attribute( "off" ).toInt() );

	if (_this.hasAttribute("sample_rate")) {
		m_sampleBuffer->setSampleRate(_this.attribute("sample_rate").toInt());
	}
}




TrackContentObjectView * SampleTCO::createView( TrackView * _tv )
{
	return new SampleTCOView( this, _tv );
}




SampleTCOView::SampleTCOView( SampleTCO * _tco, TrackView * _tv ) :
	TrackContentObjectView( _tco, _tv ),
	m_tco( _tco ),
	m_paintPixmap()
{
	// update UI and tooltip
	updateSample();

	// track future changes of SampleTCO
	connect( m_tco, SIGNAL( sampleChanged() ),
			this, SLOT( updateSample() ) );

	setStyle( QApplication::style() );
}

void SampleTCOView::updateSample()
{
	update();
	// set tooltip to filename so that user can see what sample this
	// sample-tco contains
	ToolTip::add( this, ( m_tco->m_sampleBuffer->audioFile() != "" ) ?
					m_tco->m_sampleBuffer->audioFile() :
					tr( "Double-click to open sample" ) );
}




void SampleTCOView::contextMenuEvent( QContextMenuEvent * _cme )
{
	if( _cme->modifiers() )
	{
		return;
	}

	QMenu contextMenu( this );
	if( fixedTCOs() == false )
	{
		contextMenu.addAction( embed::getIconPixmap( "cancel" ),
					tr( "Delete (middle mousebutton)" ),
						this, SLOT( remove() ) );
		contextMenu.addSeparator();
		contextMenu.addAction( embed::getIconPixmap( "edit_cut" ),
					tr( "Cut" ), this, SLOT( cut() ) );
	}
	contextMenu.addAction( embed::getIconPixmap( "edit_copy" ),
					tr( "Copy" ), m_tco, SLOT( copy() ) );
	contextMenu.addAction( embed::getIconPixmap( "edit_paste" ),
					tr( "Paste" ), m_tco, SLOT( paste() ) );
	contextMenu.addSeparator();
	contextMenu.addAction( embed::getIconPixmap( "muted" ),
				tr( "Mute/unmute (<%1> + middle click)" ).arg(UI_CTRL_KEY),
						m_tco, SLOT( toggleMute() ) );
	/*contextMenu.addAction( embed::getIconPixmap( "record" ),
				tr( "Set/clear record" ),
						m_tco, SLOT( toggleRecord() ) );*/
	constructContextMenu( &contextMenu );

	contextMenu.exec( QCursor::pos() );
}




void SampleTCOView::dragEnterEvent( QDragEnterEvent * _dee )
{
	if( StringPairDrag::processDragEnterEvent( _dee,
					"samplefile,sampledata" ) == false )
	{
		TrackContentObjectView::dragEnterEvent( _dee );
	}
}






void SampleTCOView::dropEvent( QDropEvent * _de )
{
	if( StringPairDrag::decodeKey( _de ) == "samplefile" )
	{
		m_tco->setSampleFile( StringPairDrag::decodeValue( _de ) );
		_de->accept();
	}
	else if( StringPairDrag::decodeKey( _de ) == "sampledata" )
	{
		m_tco->m_sampleBuffer->loadFromBase64(
					StringPairDrag::decodeValue( _de ) );
		m_tco->updateLength();
		update();
		_de->accept();
		Engine::getSong()->setModified();
	}
	else
	{
		TrackContentObjectView::dropEvent( _de );
	}
}




void SampleTCOView::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton &&
		_me->modifiers() & Qt::ControlModifier &&
		_me->modifiers() & Qt::ShiftModifier )
	{
		m_tco->toggleRecord();
	}
	else
	{
		if( _me->button() == Qt::MiddleButton && _me->modifiers() == Qt::ControlModifier )
		{
			SampleTCO * sTco = dynamic_cast<SampleTCO*>( getTrackContentObject() );
			if( sTco )
			{
				sTco->updateTrackTcos();
			}
		}
		TrackContentObjectView::mousePressEvent( _me );
	}
}




void SampleTCOView::mouseReleaseEvent(QMouseEvent *_me)
{
	if( _me->button() == Qt::MiddleButton && !_me->modifiers() )
	{
		SampleTCO * sTco = dynamic_cast<SampleTCO*>( getTrackContentObject() );
		if( sTco )
		{
			sTco->playbackPositionChanged();
		}
	}
	TrackContentObjectView::mouseReleaseEvent( _me );
}




void SampleTCOView::mouseDoubleClickEvent( QMouseEvent * )
{
	QString af = m_tco->m_sampleBuffer->openAudioFile();

	if ( af.isEmpty() ) {} //Don't do anything if no file is loaded
	else if ( af == m_tco->m_sampleBuffer->audioFile() )
	{	//Instead of reloading the existing file, just reset the size
		int length = (int) ( m_tco->m_sampleBuffer->frames() / Engine::framesPerTick() );
		m_tco->changeLength(length);
	}
	else
	{	//Otherwise load the new file as ususal
		m_tco->setSampleFile( af );
		Engine::getSong()->setModified();
	}
}




void SampleTCOView::paintEvent( QPaintEvent * pe )
{
	QPainter painter( this );

	if( !needsUpdate() )
	{
		painter.drawPixmap( 0, 0, m_paintPixmap );
		return;
	}

	setNeedsUpdate( false );

	if (m_paintPixmap.isNull() || m_paintPixmap.size() != size())
	{
		m_paintPixmap = QPixmap(size());
	}

	QPainter p( &m_paintPixmap );

	QLinearGradient lingrad( 0, 0, 0, height() );
	QColor c;
	bool muted = m_tco->getTrack()->isMuted() || m_tco->isMuted();

	// state: selected, muted, normal
	c = isSelected() ? selectedColor() : ( muted ? mutedBackgroundColor()
		: painter.background().color() );

	lingrad.setColorAt( 1, c.darker( 300 ) );
	lingrad.setColorAt( 0, c );

	// paint a black rectangle under the pattern to prevent glitches with transparent backgrounds
	p.fillRect( rect(), QColor( 0, 0, 0 ) );

	if( gradient() )
	{
		p.fillRect( rect(), lingrad );
	}
	else
	{
		p.fillRect( rect(), c );
	}

	p.setPen( !muted ? painter.pen().brush().color() : mutedColor() );

	const int spacing = TCO_BORDER_WIDTH + 1;
	const float ppb = fixedTCOs() ?
			( parentWidget()->width() - 2 * TCO_BORDER_WIDTH )
					/ (float) m_tco->length().getBar() :
								pixelsPerBar();

	float nom = Engine::getSong()->getTimeSigModel().getNumerator();
	float den = Engine::getSong()->getTimeSigModel().getDenominator();
	float ticksPerBar = DefaultTicksPerBar * nom / den;

	float offset =  m_tco->startTimeOffset() / ticksPerBar * pixelsPerBar();
	QRect r = QRect( TCO_BORDER_WIDTH + offset, spacing,
			qMax( static_cast<int>( m_tco->sampleLength() * ppb / ticksPerBar ), 1 ), rect().bottom() - 2 * spacing );
	m_tco->m_sampleBuffer->visualize( p, r, pe->rect() );

	QFileInfo fileInfo(m_tco->m_sampleBuffer->audioFile());
	QString filename = fileInfo.fileName();
	paintTextLabel(filename, p);

	// disable antialiasing for borders, since its not needed
	p.setRenderHint( QPainter::Antialiasing, false );

	// inner border
	p.setPen( c.lighter( 160 ) );
	p.drawRect( 1, 1, rect().right() - TCO_BORDER_WIDTH,
		rect().bottom() - TCO_BORDER_WIDTH );

	// outer border
	p.setPen( c.darker( 300 ) );
	p.drawRect( 0, 0, rect().right(), rect().bottom() );

	// draw the 'muted' pixmap only if the pattern was manualy muted
	if( m_tco->isMuted() )
	{
		const int spacing = TCO_BORDER_WIDTH;
		const int size = 14;
		p.drawPixmap( spacing, height() - ( size + spacing ),
			embed::getIconPixmap( "muted", size, size ) );
	}

	// recording sample tracks is not possible at the moment

	/* if( m_tco->isRecord() )
	{
		p.setFont( pointSize<7>( p.font() ) );

		p.setPen( textShadowColor() );
		p.drawText( 10, p.fontMetrics().height()+1, "Rec" );
		p.setPen( textColor() );
		p.drawText( 9, p.fontMetrics().height(), "Rec" );

		p.setBrush( QBrush( textColor() ) );
		p.drawEllipse( 4, 5, 4, 4 );
	}*/

	p.end();

	painter.drawPixmap( 0, 0, m_paintPixmap );
}






SampleTrack::SampleTrack( TrackContainer* tc ) :
	Track( Track::SampleTrack, tc ),
	m_volumeModel( DefaultVolume, MinVolume, MaxVolume, 0.1f, this,
							tr( "Volume" ) ),
	m_panningModel( DefaultPanning, PanningLeft, PanningRight, 0.1f,
					this, tr( "Panning" ) ),
	m_effectChannelModel( 0, 0, 0, this, tr( "FX channel" ) ),
	m_audioPort( tr( "Sample track" ), true, &m_volumeModel, &m_panningModel, &m_mutedModel )
{
	setName( tr( "Sample track" ) );
	m_panningModel.setCenterValue( DefaultPanning );
	m_effectChannelModel.setRange( 0, Engine::fxMixer()->numChannels()-1, 1);

	connect( &m_effectChannelModel, SIGNAL( dataChanged() ), this, SLOT( updateEffectChannel() ) );
}




SampleTrack::~SampleTrack()
{
	Engine::mixer()->removePlayHandlesOfTypes( this, PlayHandle::TypeSamplePlayHandle );
}




bool SampleTrack::play( const MidiTime & _start, const fpp_t _frames,
					const f_cnt_t _offset, int _tco_num )
{
	m_audioPort.effects()->startRunning();
	bool played_a_note = false;	// will be return variable

	tcoVector tcos;
	::BBTrack * bb_track = NULL;
	if( _tco_num >= 0 )
	{
		if( _start != 0 )
		{
			return false;
		}
		tcos.push_back( getTCO( _tco_num ) );
		if (trackContainer() == (TrackContainer*)Engine::getBBTrackContainer())
		{
			bb_track = BBTrack::findBBTrack( _tco_num );
		}
	}
	else
	{
		for( int i = 0; i < numOfTCOs(); ++i )
		{
			TrackContentObject * tco = getTCO( i );
			SampleTCO * sTco = dynamic_cast<SampleTCO*>( tco );

			if( _start >= sTco->startPosition() && _start < sTco->endPosition() )
			{
				if( sTco->isPlaying() == false && _start > sTco->startPosition() + sTco->startTimeOffset() )
				{
					auto bufferFramesPerTick = Engine::framesPerTick (sTco->sampleBuffer ()->sampleRate ());
					f_cnt_t sampleStart = bufferFramesPerTick * ( _start - sTco->startPosition() - sTco->startTimeOffset() );
					f_cnt_t tcoFrameLength = bufferFramesPerTick * ( sTco->endPosition() - sTco->startPosition() - sTco->startTimeOffset() );
					f_cnt_t sampleBufferLength = sTco->sampleBuffer()->frames();
					//if the Tco smaller than the sample length we play only until Tco end
					//else we play the sample to the end but nothing more
					f_cnt_t samplePlayLength = tcoFrameLength > sampleBufferLength ? sampleBufferLength : tcoFrameLength;
					//we only play within the sampleBuffer limits
					if( sampleStart < sampleBufferLength )
					{
						sTco->setSampleStartFrame( sampleStart );
						sTco->setSamplePlayLength( samplePlayLength );
						tcos.push_back( sTco );
						sTco->setIsPlaying( true );
					}
				}
			}
			else
			{
				sTco->setIsPlaying( false );
			}
		}
	}

	for( tcoVector::Iterator it = tcos.begin(); it != tcos.end(); ++it )
	{
		SampleTCO * st = dynamic_cast<SampleTCO *>( *it );
		if( !st->isMuted() )
		{
			PlayHandle* handle;
			if( st->isRecord() )
			{
				if( !Engine::getSong()->isRecording() )
				{
					return played_a_note;
				}
				SampleRecordHandle* smpHandle = new SampleRecordHandle( st );
				handle = smpHandle;
			}
			else
			{
				SamplePlayHandle* smpHandle = new SamplePlayHandle( st );
				smpHandle->setVolumeModel( &m_volumeModel );
				smpHandle->setBBTrack( bb_track );
				handle = smpHandle;
			}
			handle->setOffset( _offset );
			// send it to the mixer
			Engine::mixer()->addPlayHandle( handle );
			played_a_note = true;
		}
	}

	return played_a_note;
}




TrackView * SampleTrack::createView( TrackContainerView* tcv )
{
	return new SampleTrackView( this, tcv );
}




TrackContentObject * SampleTrack::createTCO(const MidiTime & pos)
{
	SampleTCO * sTco = new SampleTCO(this);
	sTco->movePosition(pos);
	return sTco;
}




void SampleTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	m_audioPort.effects()->saveState( _doc, _this );
#if 0
	_this.setAttribute( "icon", tlb->pixmapFile() );
#endif
	m_volumeModel.saveSettings( _doc, _this, "vol" );
	m_panningModel.saveSettings( _doc, _this, "pan" );
	m_effectChannelModel.saveSettings( _doc, _this, "fxch" );
}




void SampleTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
	QDomNode node = _this.firstChild();
	m_audioPort.effects()->clear();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( m_audioPort.effects()->nodeName() == node.nodeName() )
			{
				m_audioPort.effects()->restoreState( node.toElement() );
			}
		}
		node = node.nextSibling();
	}
	m_volumeModel.loadSettings( _this, "vol" );
	m_panningModel.loadSettings( _this, "pan" );
	m_effectChannelModel.setRange( 0, Engine::fxMixer()->numChannels() - 1 );
	m_effectChannelModel.loadSettings( _this, "fxch" );
}




void SampleTrack::updateTcos()
{
	Engine::mixer()->removePlayHandlesOfTypes( this, PlayHandle::TypeSamplePlayHandle );
	setPlayingTcos( false );
}




void SampleTrack::setPlayingTcos( bool isPlaying )
{
	for( int i = 0; i < numOfTCOs(); ++i )
	{
		TrackContentObject * tco = getTCO( i );
		SampleTCO * sTco = dynamic_cast<SampleTCO*>( tco );
		sTco->setIsPlaying( isPlaying );
	}
}




void SampleTrack::updateEffectChannel()
{
	m_audioPort.setNextFxChannel( m_effectChannelModel.value() );
}






SampleTrackView::SampleTrackView( SampleTrack * _t, TrackContainerView* tcv ) :
	TrackView( _t, tcv )
{
	setFixedHeight( 32 );

	m_tlb = new TrackLabelButton(this, getTrackSettingsWidget());
	m_tlb->setCheckable(true);
	connect(m_tlb, SIGNAL(clicked( bool )),
			this, SLOT(showEffects()));
	m_tlb->setIcon(embed::getIconPixmap("sample_track"));
	m_tlb->move(3, 1);
	m_tlb->show();

	m_volumeKnob = new Knob( knobSmall_17, getTrackSettingsWidget(),
						    tr( "Track volume" ) );
	m_volumeKnob->setVolumeKnob( true );
	m_volumeKnob->setModel( &_t->m_volumeModel );
	m_volumeKnob->setHintText( tr( "Channel volume:" ), "%" );

	int settingsWidgetWidth = ConfigManager::inst()->
					value( "ui", "compacttrackbuttons" ).toInt()
				? DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT
				: DEFAULT_SETTINGS_WIDGET_WIDTH;
	m_volumeKnob->move( settingsWidgetWidth - 2 * 24, 2 );
	m_volumeKnob->setLabel( tr( "VOL" ) );
	m_volumeKnob->show();

	m_panningKnob = new Knob( knobSmall_17, getTrackSettingsWidget(),
							tr( "Panning" ) );
	m_panningKnob->setModel( &_t->m_panningModel );
	m_panningKnob->setHintText( tr( "Panning:" ), "%" );
	m_panningKnob->move( settingsWidgetWidth - 24, 2 );
	m_panningKnob->setLabel( tr( "PAN" ) );
	m_panningKnob->show();

	setModel( _t );

	m_window = new SampleTrackWindow(this);
	m_window->toggleVisibility(false);
}




SampleTrackView::~SampleTrackView()
{
	if(m_window != NULL)
	{
		m_window->setSampleTrackView(NULL);
		m_window->parentWidget()->hide();
	}
	m_window = NULL;
}



//FIXME: This is identical to InstrumentTrackView::createFxMenu
QMenu * SampleTrackView::createFxMenu(QString title, QString newFxLabel)
{
	int channelIndex = model()->effectChannelModel()->value();

	FxChannel *fxChannel = Engine::fxMixer()->effectChannel(channelIndex);

	// If title allows interpolation, pass channel index and name
	if (title.contains("%2"))
	{
		title = title.arg(channelIndex).arg(fxChannel->m_name);
	}

	QMenu *fxMenu = new QMenu(title);

	fxMenu->addAction(newFxLabel, this, SLOT(createFxLine()));
	fxMenu->addSeparator();

	for (int i = 0; i < Engine::fxMixer()->numChannels(); ++i)
	{
		FxChannel * currentChannel = Engine::fxMixer()->effectChannel(i);

		if (currentChannel != fxChannel)
		{
			const auto index = currentChannel->m_channelIndex;
			QString label = tr("FX %1: %2").arg(currentChannel->m_channelIndex).arg(currentChannel->m_name);
			fxMenu->addAction(label, [this, index](){
				assignFxLine(index);
			});
		}
	}

	return fxMenu;
}




void SampleTrackView::showEffects()
{
	m_window->toggleVisibility(m_window->parentWidget()->isHidden());
}



void SampleTrackView::modelChanged()
{
	SampleTrack * st = castModel<SampleTrack>();
	m_volumeKnob->setModel(&st->m_volumeModel);

	TrackView::modelChanged();
}




void SampleTrackView::dragEnterEvent(QDragEnterEvent *dee)
{
	StringPairDrag::processDragEnterEvent(dee, QString("samplefile"));
}




void SampleTrackView::dropEvent(QDropEvent *de)
{
	QString type  = StringPairDrag::decodeKey(de);
	QString value = StringPairDrag::decodeValue(de);

	if (type == "samplefile")
	{
		int trackHeadWidth = ConfigManager::inst()->value("ui", "compacttrackbuttons").toInt()==1
				? DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT + TRACK_OP_WIDTH_COMPACT
				: DEFAULT_SETTINGS_WIDGET_WIDTH + TRACK_OP_WIDTH;

		int xPos = de->pos().x() < trackHeadWidth
				? trackHeadWidth
				: de->pos().x();

		MidiTime tcoPos = trackContainerView()->fixedTCOs()
				? MidiTime(0)
				: MidiTime(((xPos - trackHeadWidth) / trackContainerView()->pixelsPerBar()
							* MidiTime::ticksPerBar()) + trackContainerView()->currentPosition()
						).quantize(1.0);

		SampleTCO * sTco = static_cast<SampleTCO*>(getTrack()->createTCO(tcoPos));
		if (sTco) { sTco->setSampleFile(value); }
	}

}




SampleTrackWindow::SampleTrackWindow(SampleTrackView * tv) :
	QWidget(),
	ModelView(NULL, this),
	m_track(tv->model()),
	m_stv(tv)
{
	// init own layout + widgets
	setFocusPolicy(Qt::StrongFocus);
	QVBoxLayout * vlayout = new QVBoxLayout(this);
	vlayout->setMargin(0);
	vlayout->setSpacing(0);

	TabWidget* generalSettingsWidget = new TabWidget(tr("GENERAL SETTINGS"), this);

	QVBoxLayout* generalSettingsLayout = new QVBoxLayout(generalSettingsWidget);

	generalSettingsLayout->setContentsMargins(8, 18, 8, 8);
	generalSettingsLayout->setSpacing(6);

	QWidget* nameWidget = new QWidget(generalSettingsWidget);
	QHBoxLayout* nameLayout = new QHBoxLayout(nameWidget);
	nameLayout->setContentsMargins(0, 0, 0, 0);
	nameLayout->setSpacing(2);

	// setup line edit for changing sample track name
	m_nameLineEdit = new QLineEdit;
	m_nameLineEdit->setFont(pointSize<9>(m_nameLineEdit->font()));
	connect(m_nameLineEdit, SIGNAL(textChanged(const QString &)),
				this, SLOT(textChanged(const QString &)));

	m_nameLineEdit->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
	nameLayout->addWidget(m_nameLineEdit);


	generalSettingsLayout->addWidget(nameWidget);


	QGridLayout* basicControlsLayout = new QGridLayout;
	basicControlsLayout->setHorizontalSpacing(3);
	basicControlsLayout->setVerticalSpacing(0);
	basicControlsLayout->setContentsMargins(0, 0, 0, 0);

	QString labelStyleSheet = "font-size: 6pt;";
	Qt::Alignment labelAlignment = Qt::AlignHCenter | Qt::AlignTop;
	Qt::Alignment widgetAlignment = Qt::AlignHCenter | Qt::AlignCenter;

	// set up volume knob
	m_volumeKnob = new Knob(knobBright_26, NULL, tr("Sample volume"));
	m_volumeKnob->setVolumeKnob(true);
	m_volumeKnob->setHintText(tr("Volume:"), "%");

	basicControlsLayout->addWidget(m_volumeKnob, 0, 0);
	basicControlsLayout->setAlignment(m_volumeKnob, widgetAlignment);

	QLabel *label = new QLabel(tr("VOL"), this);
	label->setStyleSheet(labelStyleSheet);
	basicControlsLayout->addWidget(label, 1, 0);
	basicControlsLayout->setAlignment(label, labelAlignment);


	// set up panning knob
	m_panningKnob = new Knob(knobBright_26, NULL, tr("Panning"));
	m_panningKnob->setHintText(tr("Panning:"), "");

	basicControlsLayout->addWidget(m_panningKnob, 0, 1);
	basicControlsLayout->setAlignment(m_panningKnob, widgetAlignment);

	label = new QLabel(tr("PAN"),this);
	label->setStyleSheet(labelStyleSheet);
	basicControlsLayout->addWidget(label, 1, 1);
	basicControlsLayout->setAlignment(label, labelAlignment);


	basicControlsLayout->setColumnStretch(2, 1);


	// setup spinbox for selecting FX-channel
	m_effectChannelNumber = new FxLineLcdSpinBox(2, NULL, tr("FX channel"), m_stv);

	basicControlsLayout->addWidget(m_effectChannelNumber, 0, 3);
	basicControlsLayout->setAlignment(m_effectChannelNumber, widgetAlignment);

	label = new QLabel(tr("FX"), this);
	label->setStyleSheet(labelStyleSheet);
	basicControlsLayout->addWidget(label, 1, 3);
	basicControlsLayout->setAlignment(label, labelAlignment);

	generalSettingsLayout->addLayout(basicControlsLayout);

	m_effectRack = new EffectRackView(tv->model()->audioPort()->effects());
	m_effectRack->setFixedSize(240, 242);

	vlayout->addWidget(generalSettingsWidget);
	vlayout->addWidget(m_effectRack);


	setModel(tv->model());

	QMdiSubWindow * subWin = gui->mainWindow()->addWindowedWidget(this);
	Qt::WindowFlags flags = subWin->windowFlags();
	flags |= Qt::MSWindowsFixedSizeDialogHint;
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags(flags);

	// Hide the Size and Maximize options from the system menu
	// since the dialog size is fixed.
	QMenu * systemMenu = subWin->systemMenu();
	systemMenu->actions().at(2)->setVisible(false); // Size
	systemMenu->actions().at(4)->setVisible(false); // Maximize

	subWin->setWindowIcon(embed::getIconPixmap("sample_track"));
	subWin->setFixedSize(subWin->size());
	subWin->hide();
}



SampleTrackWindow::~SampleTrackWindow()
{
}



void SampleTrackWindow::setSampleTrackView(SampleTrackView* tv)
{
	if(m_stv && tv)
	{
		m_stv->m_tlb->setChecked(false);
	}

	m_stv = tv;
}



void SampleTrackWindow::modelChanged()
{
	m_track = castModel<SampleTrack>();

	m_nameLineEdit->setText(m_track->name());

	m_track->disconnect(SIGNAL(nameChanged()), this);

	connect(m_track, SIGNAL(nameChanged()),
			this, SLOT(updateName()));

	m_volumeKnob->setModel(&m_track->m_volumeModel);
	m_panningKnob->setModel(&m_track->m_panningModel);
	m_effectChannelNumber->setModel(&m_track->m_effectChannelModel);

	updateName();
}



/*! \brief Create and assign a new FX Channel for this track */
void SampleTrackView::createFxLine()
{
	int channelIndex = gui->fxMixerView()->addNewChannel();

	Engine::fxMixer()->effectChannel(channelIndex)->m_name = getTrack()->name();

	assignFxLine(channelIndex);
}




/*! \brief Assign a specific FX Channel for this track */
void SampleTrackView::assignFxLine(int channelIndex)
{
	model()->effectChannelModel()->setValue(channelIndex);

	gui->fxMixerView()->setCurrentFxLine(channelIndex);
}



void SampleTrackWindow::updateName()
{
	setWindowTitle(m_track->name().length() > 25 ? (m_track->name().left(24) + "...") : m_track->name());

	if(m_nameLineEdit->text() != m_track->name())
	{
		m_nameLineEdit->setText(m_track->name());
	}
}



void SampleTrackWindow::textChanged(const QString& new_name)
{
	m_track->setName(new_name);
	Engine::getSong()->setModified();
}



void SampleTrackWindow::toggleVisibility(bool on)
{
	if(on)
	{
		show();
		parentWidget()->show();
		parentWidget()->raise();
	}
	else
	{
		parentWidget()->hide();
	}
}




void SampleTrackWindow::closeEvent(QCloseEvent* ce)
{
	ce->ignore();

	if(gui->mainWindow()->workspace())
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}

	m_stv->m_tlb->setFocus();
	m_stv->m_tlb->setChecked(false);
}



void SampleTrackWindow::saveSettings(QDomDocument& doc, QDomElement & element)
{
	MainWindow::saveWidgetState(this, element);
	Q_UNUSED(element)
}



void SampleTrackWindow::loadSettings(const QDomElement& element)
{
	MainWindow::restoreWidgetState(this, element);
	if(isVisible())
	{
		m_stv->m_tlb->setChecked(true);
	}
}
