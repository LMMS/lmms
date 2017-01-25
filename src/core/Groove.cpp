
#include <QtCore/QObject> // needed for midi_tim.h to compile?!?!
#include <QtGui/QLabel>

#include "Groove.h"
#include "lmms_basics.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"
#include "Song.h"

Groove::Groove()
{

}

/**
 * Default groove is no groove. Not even a wiggle.
 * @return 0 or -1
 */
int Groove::isInTick(MidiTime * _cur_start, fpp_t _frames, f_cnt_t _offset,
						 Note * _n, Pattern * _p ) {

	return _n->pos().getTicks() == _cur_start->getTicks() ? 0 : -1;

}


void Groove::saveSettings( QDomDocument & _doc, QDomElement & _element )
{

}

void Groove::loadSettings( const QDomElement & _this )
{

}

QWidget * Groove::instantiateView( QWidget * _parent )
{
	return new QLabel("No groove");
}
