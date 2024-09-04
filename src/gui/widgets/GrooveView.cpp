#include "GrooveView.h"

#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>

#include "embed.h"
#include "Engine.h"
#include "MainWindow.h"
#include "Song.h"
#include "GuiApplication.h"

#include "Groove.h"
#include "HydrogenSwing.h"
#include "HalfSwing.h"
#include "GrooveExperiments.h"
#include "MidiSwing.h"


namespace lmms::gui
{

GrooveView::GrooveView(QWidget * parent) :
	QWidget(parent)
{
	m_layout = new QVBoxLayout(this);

	m_comboBox = new QComboBox(this);
	// Insert reverse order.
	m_comboBox->insertItem(0, tr("Experiment swing") , QVariant::fromValue(5));
	m_comboBox->insertItem(0, tr("Half swing") , QVariant::fromValue(4));
	m_comboBox->insertItem(0, tr("Hydrogen swing") , QVariant::fromValue(3));
	m_comboBox->insertItem(0, tr("MIDI swing") , QVariant::fromValue(2));
	m_comboBox->insertItem(0, tr("No swing") , QVariant::fromValue(1));
	m_comboBox->setCurrentIndex(0);

	m_layout->addWidget(m_comboBox);
	m_layout->addWidget(new QLabel(""));

	connect(m_comboBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(grooveChanged()));

	connect(Engine::getSong(), SIGNAL(dataChanged()),
			this, SLOT(update()));

	connect(Engine::getSong(), SIGNAL(projectLoaded()),
			this, SLOT(update()));

	update();
}

GrooveView::~GrooveView()
{
}

void GrooveView::update()
{
	Groove * groove = Engine::getSong()->globalGroove();
	if (groove->nodeName() == "none")
	{
		m_comboBox->setCurrentIndex(0);
	}
	if (groove->nodeName() == "midi")
	{
		m_comboBox->setCurrentIndex(1);
	}
	if (groove->nodeName() == "hydrogen")
	{
		m_comboBox->setCurrentIndex(2);
	}
	if (groove->nodeName() == "half")
	{
		m_comboBox->setCurrentIndex(3);
	}
	if (groove->nodeName() == "experiment")
	{
		m_comboBox->setCurrentIndex(4);
	}
	setView(groove);
}

void GrooveView::clear()
{
	QLayoutItem * li = m_layout->takeAt(1);
	delete li->widget();
	delete li;

	m_comboBox->setCurrentIndex(0);
	m_layout->addWidget(new QLabel(""));
}

void GrooveView::grooveChanged()
{
	Groove * groove = NULL;

	int currentIndex = m_comboBox->currentIndex();
	switch (currentIndex) {
		case 0 :
		{
			groove = new Groove();
			break;
		}
		case 1 :
		{
			groove = new MidiSwing();
			break;
		}
		case 2 :
		{
			groove = new HydrogenSwing();
			break;
		}
		case 3 :
		{
			groove = new HalfSwing();
			break;
		}
		case 4 :
		{
			groove = new GrooveExperiments();
			break;
		}
	}
	Song * song = Engine::getSong();
	song->setGlobalGroove(groove); // TODO: This can fail.
	setView(groove);
}

void GrooveView::setView(Groove * groove)
{
	QWidget * view = groove->instantiateView(this);
	QLayoutItem * li = m_layout->takeAt(1);
	delete li->widget();
	delete li;

	m_layout->addWidget(view);
}

} // namespace lmms::gui
