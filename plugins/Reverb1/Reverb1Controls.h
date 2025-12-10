#ifndef LMMS_REVERB1_CONTROLS_H

#define LMMS_REVERB1_CONTROLS_H



#include "EffectControls.h"

#include "Reverb1ControlDialog.h"



namespace lmms {



class Reverb1Effect;



class Reverb1Controls : public EffectControls {

    Q_OBJECT



public:

    Reverb1Controls(Reverb1Effect* effect);

    ~Reverb1Controls() override = default;



    void saveSettings(QDomDocument& doc, QDomElement& parent) override;

    void loadSettings(const QDomElement& parent) override;



    inline QString nodeName() const override { return "Reverb1Controls"; }



    gui::EffectControlDialog* createView() override { return new gui::Reverb1ControlDialog(this); }

    int controlCount() override { return 6; } // Updated count



    // PARAMETERS

    FloatModel m_roomSizeModel; 

    FloatModel m_dampingModel; 

    FloatModel m_widthModel; 

    FloatModel m_preDelayModel; // New: 0 to 200ms

    

    // MIX

    FloatModel m_dryModel;

    FloatModel m_wetModel;



    // PRESETS

    IntModel m_presetModel; // New: Selects the room type



public slots:

    void updatePreset(); // Slot to handle preset changes



private:

    Reverb1Effect* m_effect;

};



} // namespace lmms



#endif // LMMS_REVERB1_CONTROLS_H
