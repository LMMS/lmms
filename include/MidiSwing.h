#ifndef MIDISWING_H
#define MIDISWING_H

#include <QtCore/QObject>

#include "Groove.h"
#include "lmms_basics.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"

/**
 * A swing groove that adjusts by whole ticks.
 * Someone might like it, also might be able to save the output to a midi file later.
 */
class MidiSwing : public QObject, public Groove
{
    Q_OBJECT
public:
	MidiSwing(QObject * _parent=0 );

    ~MidiSwing();
    
    // TODO why declaring this should it not come from super class?
    int isInTick(MidiTime * cur_start, const fpp_t _frames, const f_cnt_t _offset, Note * n, Pattern * p );
	int isInTick(MidiTime * _cur_start, Note * _n, Pattern * _p );

	void loadSettings( const QDomElement & _this );
	void saveSettings( QDomDocument & _doc, QDomElement & _element );
	inline virtual QString nodeName() const
	{
		return "midi";
	}

	QWidget * instantiateView( QWidget * _parent );

signals:
    
public slots:
    
} ;

#endif // MIDISWING_H
