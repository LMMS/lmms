

#ifndef GROOVEVIEW_H
#define GROOVEVIEW_H

#include <QWidget>
#include <QCloseEvent>

#include <QComboBox>
#include <QVBoxLayout>

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
	void grooveChanged(int index);

private:
	void setView(Groove * groove);

	QComboBox * m_comboBox;
	QVBoxLayout * m_layout;
};

#endif // GROOVEVIEW_H
