#ifndef PLUGIN_LOAD_FAILED_DIALOG
#define PLUGIN_LOAD_FAILED_DIALOG

#include <QDialog>

class QLabel;
class QTableWidget;

namespace lmms
{

void pluginLoadFailed(QString pluginName, QString error);

namespace gui
{

class PluginLoadFailedDialog : public QDialog
{
	Q_OBJECT

public:
	static inline PluginLoadFailedDialog* inst()
	{
		if (s_inst == nullptr) { s_inst = new PluginLoadFailedDialog; }
		return s_inst;
	}
	PluginLoadFailedDialog();
	~PluginLoadFailedDialog() override;
	void addEntry(QString pluginName, QString error);

private:
	static PluginLoadFailedDialog* s_inst;
	QTableWidget* m_table;
	QLabel* m_label;
};

} // namespace gui

} // namespace lmms

#endif
