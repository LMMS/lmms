

#ifndef GROOVEVIEW_H
#define GROOVEVIEW_H

#include <QtCore/QObject>

#include <QtGui/QWidget>
#include <QtGui/QComboBox>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>

#include "Groove.h"
#include "SerializingObject.h"

class GrooveView : public QWidget
{
    Q_OBJECT
public:
	explicit GrooveView(QWidget * parent = 0);
    virtual ~GrooveView();

	void clear();

signals:

public slots:
	void update();
	void grooveChanged(int index);

private:
	void setView(Groove * groove);

	QComboBox * m_dropDown;
	QVBoxLayout * m_layout;
    
};

#endif // GROOVEVIEW_H
