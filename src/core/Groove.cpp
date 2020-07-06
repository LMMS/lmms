
#include <QObject>
#include <QLabel>

#include "Groove.h"
#include "lmms_basics.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"
#include "Song.h"

Groove::Groove(QObject *parent) :
	QObject(parent)
{
}

/*
 * Default groove is no groove. Not even a wiggle.
 * @return 0 or -1
 */
int Groove::isInTick(MidiTime * curStart, fpp_t frames, f_cnt_t offset,
						Note * n, Pattern * p) {

	return n->pos().getTicks() == curStart->getTicks() ? 0 : -1;
}

void Groove::setAmount(int amount)
{

	if (amount < 0) {
		amount = 0;
	}
	if (amount > 127) {
		amount = 127;
	}

	m_amount = amount;
	m_swingFactor = ((float)m_amount) / 127.0f;
	emit amountChanged(m_amount);

}

void Groove::saveSettings(QDomDocument & doc, QDomElement & element)
{
	Q_UNUSED(doc);
	element.setAttribute("amount", m_amount);
}

void Groove::loadSettings(const QDomElement & element)
{
	bool ok;
	int amount = element.attribute("amount").toInt(&ok);
	if (ok)
	{
		setAmount(amount);
	}
	else
	{
		setAmount(0);
	}
}

QWidget * Groove::instantiateView(QWidget * parent)
{
	return new QLabel("");
}
