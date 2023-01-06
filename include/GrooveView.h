

#ifndef GROOVEVIEW_H
#define GROOVEVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QComboBox>

#include "Groove.h"
#include "SerializingObject.h"

namespace lmms::gui
{

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

} // namespace lmms

#endif // GROOVEVIEW_H
