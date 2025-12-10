#include "Reverb1Controls.h"

#include "Reverb1.h" 

#include <QDomElement>



namespace lmms {



Reverb1Controls::Reverb1Controls(Reverb1Effect* effect)

    : EffectControls(effect),

      m_roomSizeModel(0.5f, 0.0f, 1.0f, 0.01f, this, tr("Room Size")),

      m_dampingModel(0.5f, 0.0f, 1.0f, 0.01f, this, tr("Damping")),

      m_widthModel(1.0f, 0.0f, 1.0f, 0.01f, this, tr("Width")),

      m_preDelayModel(0.0f, 0.0f, 250.0f, 1.0f, this, tr("Pre-Delay")),

      m_dryModel(1.0f, 0.0f, 1.0f, 0.01f, this, tr("Dry")),

      m_wetModel(0.25f, 0.0f, 1.0f, 0.01f, this, tr("Wet")),

      // FIX: Removed the '1' step argument. IntModel is (val, min, max, parent, name)

      m_presetModel(0, 0, 14, this, tr("Preset")), 

      m_effect(effect) 

{

    // Connect the preset model change to our function

    connect(&m_presetModel, SIGNAL(dataChanged()), this, SLOT(updatePreset()));

}



void Reverb1Controls::updatePreset() {

    int p = m_presetModel.value();



    // Struct to hold preset data

    struct Preset { float size; float damp; float width; float pre; };



    // 15 Presets (Size, Damp, Width, PreDelay)

    Preset presets[] = {

        { 0.10f, 0.10f, 1.00f,  5.0f }, // 0: Small Bathroom (Bright)

        { 0.15f, 0.60f, 0.40f,  0.0f }, // 1: Drum Booth (Dead)

        { 0.30f, 0.30f, 0.80f, 10.0f }, // 2: Living Room

        { 0.40f, 0.20f, 0.90f, 15.0f }, // 3: Small Club

        { 0.60f, 0.40f, 1.00f, 25.0f }, // 4: Large Club

        { 0.70f, 0.30f, 1.00f, 30.0f }, // 5: Small Hall

        { 0.85f, 0.25f, 1.00f, 60.0f }, // 6: Large Hall

        { 0.90f, 0.20f, 1.00f, 80.0f }, // 7: Church

        { 0.96f, 0.10f, 1.00f, 120.0f}, // 8: Cathedral

        { 0.95f, 0.05f, 0.50f, 40.0f }, // 9: Tunnel

        { 0.75f, 0.10f, 1.00f, 20.0f }, // 10: Gymnasium (Slapback)

        { 0.50f, 0.00f, 1.00f,  0.0f }, // 11: Plate (Metallic)

        { 0.20f, 0.00f, 0.20f,  0.0f }, // 12: Spring (Boingy)

        { 0.99f, 0.50f, 1.00f, 50.0f }, // 13: Ambient Wash

        { 1.00f, 0.00f, 1.00f, 200.0f}  // 14: Outer Space

    };



    if (p >= 0 && p < 15) {

        m_roomSizeModel.setValue(presets[p].size);

        m_dampingModel.setValue(presets[p].damp);

        m_widthModel.setValue(presets[p].width);

        m_preDelayModel.setValue(presets[p].pre);

    }

}



void Reverb1Controls::saveSettings(QDomDocument& doc, QDomElement& parent) {

    m_roomSizeModel.saveSettings(doc, parent, "roomsize");

    m_dampingModel.saveSettings(doc, parent, "damping");

    m_widthModel.saveSettings(doc, parent, "width");

    m_preDelayModel.saveSettings(doc, parent, "predelay");

    m_dryModel.saveSettings(doc, parent, "dry");

    m_wetModel.saveSettings(doc, parent, "wet");

    m_presetModel.saveSettings(doc, parent, "preset");

}



void Reverb1Controls::loadSettings(const QDomElement& parent) {

    m_roomSizeModel.loadSettings(parent, "roomsize");

    m_dampingModel.loadSettings(parent, "damping");

    m_widthModel.loadSettings(parent, "width");

    m_preDelayModel.loadSettings(parent, "predelay");

    m_dryModel.loadSettings(parent, "dry");

    m_wetModel.loadSettings(parent, "wet");

    m_presetModel.loadSettings(parent, "preset");

}



} // namespace lmms
