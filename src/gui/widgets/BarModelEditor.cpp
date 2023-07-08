#include <BarModelEditor.h>

#include <QPainter>


namespace lmms::gui
{

BarModelEditor::BarModelEditor(QString text, FloatModel * floatModel, QWidget * parent) :
	FloatModelEditorBase(parent),
	m_text(text)
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

} // namespace lmms::gui
