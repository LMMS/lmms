#ifndef LMMS_TRANSFERFUNCTION_CONTROLS_H
#define LMMS_TRANSFERFUNCTION_CONTROLS_H
#include "EffectControls.h"
#include "Graph.h"
#include "TransferFunctionControlDialog.h"
#include "TransferFunctionLogic.h" 
#include <QString>
#include <vector>
namespace lmms
{
class TransferFunctionEffect;
class TransferFunctionControls : public EffectControls
{
    Q_OBJECT
public:
    TransferFunctionControls(TransferFunctionEffect* effect);
    ~TransferFunctionControls() override = default;
    void saveSettings(QDomDocument& doc, QDomElement& parent) override;
    void loadSettings(const QDomElement& parent) override;
    QString getFormula() const { return m_formulaString; }
    
    const std::vector<DrawPoint>& getDrawPoints() const { return m_drawPoints; }
    
    void setDrawPoint(float freq, float mag);
    // NEW: Function to reset the drawing
    void clearDrawing();
    inline QString nodeName() const override
    {
        return "TransferFunctionControls";
    }
    gui::EffectControlDialog* createView() override
    {
        return new gui::TransferFunctionControlDialog(this);
    }
    int controlCount() override { return 4; }
public slots:
    void setFormula(const QString& text);
    void updateBodePlot();
private:
    TransferFunctionEffect* m_effect;
    FloatModel m_volumeModel; 
    QString m_formulaString;
    
    std::vector<DrawPoint> m_drawPoints;
    graphModel m_bodeMagModel;
    graphModel m_bodePhaseModel;
    friend class gui::TransferFunctionControlDialog;
    friend class TransferFunctionEffect;
};
} // namespace lmms
#endif // LMMS_TRANSFERFUNCTION_CONTROLS_H
