#ifndef LMMS_GUI_TRANSFERFUNCTION_CONTROL_DIALOG_H
#define LMMS_GUI_TRANSFERFUNCTION_CONTROL_DIALOG_H
#include "EffectControlDialog.h"
#include <QEvent> 
#include <QPushButton> 
#include <QLabel> // Added this
namespace lmms
{
class TransferFunctionControls;
namespace gui
{
class TransferFunctionControlDialog : public EffectControlDialog
{
    Q_OBJECT
public:
    TransferFunctionControlDialog(TransferFunctionControls* controls);
    ~TransferFunctionControlDialog() override = default;
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
private slots:
    void updateUIState();
    void updateFormulaLabel(); // Fixed: Added declaration
private:
    TransferFunctionControls* m_pluginControls;
    
    QPushButton* m_clearButton;
    QLabel* m_activeFormulaLabel; // Fixed: Added declaration
};
} // namespace gui
} // namespace lmms
#endif // LMMS_GUI_TRANSFERFUNCTION_CONTROL_DIALOG_H
