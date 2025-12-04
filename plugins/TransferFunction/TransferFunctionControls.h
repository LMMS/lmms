#ifndef LMMS_TRANSFERFUNCTION_CONTROLS_H

#define LMMS_TRANSFERFUNCTION_CONTROLS_H



#include "EffectControls.h"

#include "Graph.h"

#include "TransferFunctionControlDialog.h"

#include <QString>



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

    

    // MOVED here so we can connect the Knob to it

    void updateBodePlot();



private:

    TransferFunctionEffect* m_effect;



    // Parameters

    FloatModel m_volumeModel; // This is the PRESET knob

    QString m_formulaString;



    // Bode plot graph models

    graphModel m_bodeMagModel;

    graphModel m_bodePhaseModel;



    friend class gui::TransferFunctionControlDialog;

    friend class TransferFunctionEffect;

};



} // namespace lmms



#endif // LMMS_TRANSFERFUNCTION_CONTROLS_H
