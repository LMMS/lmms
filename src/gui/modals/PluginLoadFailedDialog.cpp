#include "PluginLoadFailedDialog.h"

#include "GuiApplication.h"

#include <QBoxLayout>
#include <QDebug>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QTableWidget>
#include <QString>

namespace lmms
{

void pluginLoadFailed(QString pluginName, QString error)
{
	if (gui::getGUI())
	{
		gui::PluginLoadFailedDialog::inst()->addEntry(pluginName, error);
	}
	else
	{
		qWarning() << "Plugin" << pluginName << "failed to load:" <<  error;
	}
}

namespace gui {

PluginLoadFailedDialog::PluginLoadFailedDialog()
	: QDialog{}
{
	setModal(false);
	setAttribute(Qt::WA_DeleteOnClose);
	setLayout(new QVBoxLayout);
	m_label = new QLabel{tr("<strong>The following plugin(s) failed to load:</strong>")};
	layout()->addWidget(m_label);
	m_table = new QTableWidget;
	m_table->setColumnCount(2);
	m_table->setHorizontalHeaderLabels({tr("Plugin name"), tr("Reason")});
	m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_table->verticalHeader()->hide(); // Hides row numbers
	layout()->addWidget(m_table);
}

PluginLoadFailedDialog::~PluginLoadFailedDialog()
{
	if (s_inst == this) { s_inst = nullptr; }
}

void PluginLoadFailedDialog::addEntry(QString pluginName, QString error)
{
	m_table->insertRow(m_table->rowCount());
	m_table->setItem(m_table->rowCount()-1, 0, new QTableWidgetItem{pluginName});
	m_table->setItem(m_table->rowCount()-1, 1, new QTableWidgetItem{error});
	show();
}

PluginLoadFailedDialog* PluginLoadFailedDialog::s_inst = nullptr;

} // namespace gui

} // namespace lmms
