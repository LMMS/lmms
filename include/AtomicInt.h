/// \file  AtomicInt.h
/// \brief Compatibility subclass of QAtomicInt for supporting both Qt4 and Qt5

#ifndef LMMS_ATOMIC_H
#define LMMS_ATOMIC_H

#include <QtCore/QAtomicInt>

#if QT_VERSION >= 0x050000 && QT_VERSION <= 0x050300

class AtomicInt : public QAtomicInt
{
public:
	AtomicInt(int value=0) : QAtomicInt(value) {};

	operator int() const {return loadAcquire();}
};

#else

typedef QAtomicInt AtomicInt;

#endif // QT_VERSION >= 0x050000 && QT_VERSION <= 0x050300

#endif
