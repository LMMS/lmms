

#ifndef GROOVEVIEW_H
#define GROOVEVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QComboBox>

#include "Groove.h"
#include "SerializingObject.h"

class GrooveView : public QWidget
{
	Q_OBJECT
public:
	GrooveView(QWidget * parent);
	virtual ~GrooveView();

	void clear();

signals:

public slots:
	void update();
	void grooveChanged();

private:
	void setView(Groove * groove);

	QVBoxLayout * m_layout;
	QComboBox * m_comboBox;
};

#endif // GROOVEVIEW_H
