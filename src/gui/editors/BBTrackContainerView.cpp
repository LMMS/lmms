//
// Created by Nicholas Wertzberger on 7/4/16.
//

#include <include/Pattern.h>
#include <include/MainWindow.h>
#include <include/StringPairDrag.h>
#include "BBTrackContainerView.h"


BBTrackContainerView::BBTrackContainerView(BBTrackContainer* tc) :
		TrackContainerView(tc),
		m_bbtc(tc)
{
	setModel( tc );
}




void BBTrackContainerView::addSteps()
{
	printf("BBTrackContainerView::addSteps()\n");
	makeSteps( false );
}

void BBTrackContainerView::cloneSteps()
{
	printf("BBTrackContainerView::cloneSteps()\n");
	makeSteps( true );
}




void BBTrackContainerView::removeSteps()
{
	printf("BBTrackContainerView::removeSteps()\n");
	TrackContainer::TrackList tl = model()->tracks();

	for( TrackContainer::TrackList::iterator it = tl.begin(); it != tl.end(); ++it )
	{
		if( ( *it )->type() == Track::InstrumentTrack )
		{
			Pattern* p = static_cast<Pattern *>( ( *it )->getTCO( m_bbtc->currentBB() ) );
			p->removeSteps();
		}
	}
}




void BBTrackContainerView::addAutomationTrack()
{
	printf("BBTrackContainerView::addAutomationTrack()\n");
	(void) Track::create( Track::AutomationTrack, model() );
}




void BBTrackContainerView::removeBBView(int bb)
{
	for( TrackView* view : trackViews() )
	{
		view->getTrackContentWidget()->removeTCOView( bb );
	}
}



void BBTrackContainerView::saveSettings(QDomDocument& doc, QDomElement& element)
{
	printf("BBTrackContainerView::saveSettings()\n");
	MainWindow::saveWidgetState(parentWidget(), element, QSize( 640, 400 ) );
}

void BBTrackContainerView::loadSettings(const QDomElement& element)
{
	printf("BBTrackContainerView::loadSettings()\n");
	MainWindow::restoreWidgetState(parentWidget(), element);
}




void BBTrackContainerView::dropEvent(QDropEvent* de)
{
	printf("BBTrackContainerView::dropEvent()\n");
	QString type = StringPairDrag::decodeKey( de );
	QString value = StringPairDrag::decodeValue( de );

	if( type.left( 6 ) == "track_" )
	{
		DataFile dataFile( value.toUtf8() );
		Track * t = Track::create( dataFile.content().firstChild().toElement(), model() );

		t->deleteTCOs();
		m_bbtc->updateAfterTrackAdd();

		de->accept();
	}
	else
	{
		TrackContainerView::dropEvent( de );
	}
}




void BBTrackContainerView::updatePosition()
{
	printf("BBTrackContainerView::updatePosition()\n");
	//realignTracks();
	emit positionChanged( m_currentPosition );
}




void BBTrackContainerView::makeSteps( bool clone )
{
	printf("BBTrackContainerView::makeSteps(%s)\n", clone ? "true" : "false");
	TrackContainer::TrackList tl = model()->tracks();

	for( TrackContainer::TrackList::iterator it = tl.begin(); it != tl.end(); ++it )
	{
		if( ( *it )->type() == Track::InstrumentTrack )
		{
			Pattern* p = static_cast<Pattern *>( ( *it )->getTCO( m_bbtc->currentBB() ) );
			if( clone )
			{
				p->cloneSteps();
			} else
			{
				p->addSteps();
			}
		}
	}
}
