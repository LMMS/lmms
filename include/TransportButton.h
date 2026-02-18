#ifndef TRANSPORTBUTTON_H
#define TRANSPORTBUTTON_H

#include "AutomatableButton.h"

namespace lmms::gui
{

class LMMS_EXPORT TransportButton : public AutomatableButton
{
	Q_OBJECT
public:
	TransportButton( QWidget * _parent,
					const QString & _name = QString() );
	~TransportButton() override = default;

protected:
	void mousePressEvent( QMouseEvent * _me ) override;
	void mouseReleaseEvent( QMouseEvent * _me ) override;
	void contextMenuEvent( QContextMenuEvent * _me ) override;
} ;

} // namespace lmms::gui

#endif // TRANSPORTBUTTON_H
