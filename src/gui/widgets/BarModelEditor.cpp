#include <BarModelEditor.h>

#include <QPainter>


namespace lmms::gui
{

BarModelEditor::BarModelEditor(QString text, FloatModel * floatModel, QWidget * parent) :
	FloatModelEditorBase(DirectionOfManipulation::Horizontal, parent),
	m_text(text),
	m_backgroundBrush(palette().base()),
	m_barBrush(palette().button()),
	m_textColor(palette().text().color())
{
	setModel(floatModel);
}

QSizePolicy BarModelEditor::sizePolicy() const
{
	return QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

QSize BarModelEditor::minimumSizeHint() const
{
	auto const fm = fontMetrics();
	return QSize(50, fm.height() + 6);
}

QSize BarModelEditor::sizeHint() const
{
	return minimumSizeHint();
}

QBrush const & BarModelEditor::getBackgroundBrush() const
{
	return m_backgroundBrush;
}

void BarModelEditor::setBackgroundBrush(QBrush const & backgroundBrush)
{
	m_backgroundBrush = backgroundBrush;
}

QBrush const & BarModelEditor::getBarBrush() const
{
	return m_barBrush;
}

void BarModelEditor::setBarBrush(QBrush const & barBrush)
{
	m_barBrush = barBrush;
}

QColor const & BarModelEditor::getTextColor() const
{
	return m_textColor;
}

void BarModelEditor::setTextColor(QColor const & textColor)
{
	m_textColor = textColor;
}

void BarModelEditor::paintEvent(QPaintEvent *event)
{
	QWidget::paintEvent(event);

	auto const * mod = model();
	auto const minValue = mod->minValue();
	auto const maxValue = mod->maxValue();
	auto const range = maxValue - minValue;

	QRect const r = rect();

	QPainter painter(this);

	// Paint the base rectangle into which the bar and the text go
	QBrush const & backgroundBrush = getBackgroundBrush();
	painter.setPen(backgroundBrush.color());
	painter.setBrush(backgroundBrush);
	painter.drawRect(r);


	// Paint the bar
	// Compute the percentage as:
	// min + x * (max - min) = v <=> x = (v - min) / (max - min)
	auto const percentage = range == 0 ? 1. : (model()->inverseScaledValue(model()->value()) - minValue) / range;

	int const margin = 3;
	QMargins const margins(margin, margin, margin, margin);
	QRect const valueRect = r.marginsRemoved(margins);

	QBrush const & barBrush = getBarBrush();
	painter.setPen(barBrush.color());
	painter.setBrush(barBrush);
	QPoint const startPoint = valueRect.topLeft();
	QPoint endPoint = valueRect.bottomRight();
	endPoint.setX(startPoint.x() + percentage * (endPoint.x() - startPoint.x()));

	painter.drawRect(QRect(startPoint, endPoint));


	// Draw the text into the value rectangle but move it slightly to the right
	QRect const textRect = valueRect.marginsRemoved(QMargins(3, 0, 0, 0));

	// Elide the text if needed
	auto const fm = fontMetrics();
	QString const elidedText = fm.elidedText(m_text, Qt::ElideRight, textRect.width());

	// Now draw the text
	painter.setPen(getTextColor());
	painter.drawText(textRect, elidedText);
}

} // namespace lmms::gui
