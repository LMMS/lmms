#ifndef LMMS_GUI_TRANSFERFUNCTION_CONTROL_DIALOG_H
#define LMMS_GUI_TRANSFERFUNCTION_CONTROL_DIALOG_H

#include "EffectControlDialog.h"
#include <QEvent> 
#include <QPushButton> 
#include <QLabel> 
#include <QWidget>

namespace lmms
{
class TransferFunctionControls;

namespace gui
{

// Custom widget for Pole-Zero interaction

class PoleZeroGraph : public QWidget
{
    Q_OBJECT

public:
    PoleZeroGraph(QWidget* parent, TransferFunctionControls* controls);
protected:
    void paintEvent(QPaintEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;

private:
    TransferFunctionControls* m_controls;
    int m_dragIndex = -1;
    bool m_dragIsPole = false;
};

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
    void updateFormulaLabel(); 

private:
    TransferFunctionControls* m_pluginControls;
    QPushButton* m_clearButton;
    QLabel* m_activeFormulaLabel; 
  
    // Containers to toggle visibility
    QWidget* m_bodeContainer;
    PoleZeroGraph* m_poleZeroGraph;
};

} // namespace gui
} // namespace lmms

#endif // LMMS_GUI_TRANSFERFUNCTION_CONTROL_DIALOG_H
