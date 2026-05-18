#include "PluginLoadFailedDialog.h"

#include "GuiApplication.h"

#include <QBoxLayout>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
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
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers); // Disable cell editing
	m_table->setFocusPolicy(Qt::NoFocus);
	m_table->setSelectionMode(QAbstractItemView::NoSelection); // Disable cell focus/selection
	m_table->setColumnCount(2);
	m_table->setHorizontalHeaderLabels({tr("Plugin name"), tr("Reason")});
	m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_table->verticalHeader()->hide(); // Hide row numbers
	layout()->addWidget(m_table);

	auto* dialogButtons = new QDialogButtonBox{QDialogButtonBox::Ok};
	connect(dialogButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	layout()->addWidget(dialogButtons);
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
