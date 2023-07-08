#include <BarModelEditor.h>

#include "CaptionMenu.h"

#include <QPainter>
#include <QInputDialog>


namespace lmms::gui
{

BarModelEditor::BarModelEditor(QString text, FloatModel * floatModel, QWidget * parent) :
	QWidget(parent),
	FloatModelView( floatModel, this ),
	m_text(text)
{
	connectToModelSignals();
}

QSizePolicy BarModelEditor::sizePolicy() const
{
	return QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

QSize BarModelEditor::minimumSizeHint() const
{
	auto const fm = fontMetrics();
	return QSize(200, fm.height() * 1.3);
}

QSize BarModelEditor::sizeHint() const
{
	return minimumSizeHint();
}

void BarModelEditor::paintEvent(QPaintEvent *event)
{
	QWidget::paintEvent(event);

	QColor const background(30, 40, 51);
	QColor const foreground(3, 94, 97);
	QColor const textColor(14, 192, 198);

	auto const * mod = model();
	auto const minValue = mod->minValue();
	auto const maxValue = mod->maxValue();
	auto const range = maxValue - minValue;

	QRect const r = rect();

	QPainter painter(this);
	painter.setPen(background);
	painter.setBrush(background);
	painter.drawRect(r);

	// Compute the percentage
	// min + x * (max - min) = v <=> x = (v - min) / (max - min)
	auto const percentage = range == 0 ? 1. : (mod->value() - minValue) / range;

	int const margin = 2;
	QMargins const margins(margin, margin, margin, margin);
	QRect const valueRect = r.marginsRemoved(margins);

	painter.setPen(foreground);
	painter.setBrush(foreground);
	painter.drawRect(QRect(valueRect.topLeft(), QPoint(valueRect.width() * percentage, valueRect.height())));

	// Draw text
	QRect const textRect = valueRect.marginsRemoved(margins);
	painter.setPen(textColor);
	painter.drawText(textRect, m_text);
}

void BarModelEditor::contextMenuEvent(QContextMenuEvent * me)
{
	CaptionMenu contextMenu(model()->displayName(), this);

	addDefaultActions(&contextMenu);

	contextMenu.addSeparator();
	contextMenu.exec(QCursor::pos());
}

void BarModelEditor::mouseDoubleClickEvent(QMouseEvent * me)
{
	bool ok;

	float new_val = QInputDialog::getDouble(
		this, tr("Set value"),
		tr("Please enter a new value between "
		   "%1 and %2:").
		arg(model()->minValue()).
		arg(model()->maxValue()),
		model()->getRoundedValue(),
		model()->minValue(),
		model()->maxValue(), model()->getDigitCount(), &ok);

	if (ok)
	{
		model()->setValue(new_val);
	}
}

void BarModelEditor::connectToModelSignals()
{
	auto * m = model();
	if(m)
	{
		// TODO The first connection does a "friendly" update in Knob. Do we also have to do this?
		QObject::connect(m, SIGNAL(dataChanged()), this, SLOT(update()));
		QObject::connect(m ,SIGNAL(propertiesChanged()), this, SLOT(update()));
	}
}

}
