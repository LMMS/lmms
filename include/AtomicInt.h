/// \file  AtomicInt.h
/// \brief Compatibility subclass of QAtomicInt for supporting both Qt4 and Qt5

#ifndef LMMS_ATOMIC_H
#define LMMS_ATOMIC_H

#include <QtCore/QAtomicInt>

#if QT_VERSION < 0x050300

class AtomicInt : public QAtomicInt
{
public:
	AtomicInt( int value = 0 ) :
		QAtomicInt( value )
	{
	}

	int fetchAndAndOrdered( int valueToAnd )
	{
		int value;

		do
		{
			value = ( int ) * this;
		}
		while( !testAndSetOrdered( value, value & valueToAnd ) );

		return value;
	}

#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050300
	operator int() const
	{
		return loadAcquire();
	}
#endif

};

#else

typedef QAtomicInt AtomicInt;

#endif // QT_VERSION < 0x050300

#endif
