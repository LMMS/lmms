#ifndef MODULAR_ONE_H
#define MODULAR_ONE_H
#include <vector>
#include <atomic>
#include <memory>
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
#include "ModelView.h"
namespace lmms
{
struct PatchCable {
    int sourceID;
    int destID;
};
struct ModularPatch {
    std::vector<PatchCable> cables;
};
class ModularOnePlugin : public Instrument
{
public:
    ModularOnePlugin(InstrumentTrack* t);
    virtual ~ModularOnePlugin() = default;
    void playNote(NotePlayHandle* _n, SampleFrame* _working_buffer) override;
    void deleteNotePluginData(NotePlayHandle* _n) override;
    void saveSettings(QDomDocument& doc, QDomElement& parent) override;
    void loadSettings(const QDomElement& thisElement) override;
    gui::PluginView* instantiateView(QWidget* p) override;
    virtual QString nodeName() const override { return "ModularOne"; } 
    std::shared_ptr<ModularPatch> getPatch();
    void addCable(int src, int dst);
 
    // --- MATCHES .CPP EXACTLY ---
    void removeCable(int src, int dst);
    void removeCablesAt(int jackID); 
public:
    // --- KNOBS ---
    FloatModel m_knobOscFreq;
    FloatModel m_knobPWM;      
    FloatModel m_knobVCO2Detune; 
    FloatModel m_knobLfoRate;
    FloatModel m_knobCutoff;
    FloatModel m_knobResonance;
    FloatModel m_knobEnvA;
    FloatModel m_knobEnvD;
    FloatModel m_knobEnvS;
    FloatModel m_knobEnvR;
    FloatModel m_knobDrive;
private:
    std::shared_ptr<ModularPatch> m_currentPatch;
};
namespace gui
{
    struct JackDef {
        int id;
        QRect rect;
        QString label;
        bool isOutput;
        QColor color;
    };
    class ModularOneView : public InstrumentViewFixedSize
    {
    public:
        ModularOneView(Instrument* i, QWidget* p);
    
        void paintEvent(QPaintEvent* e) override;
        void mousePressEvent(QMouseEvent* e) override;
        void mouseMoveEvent(QMouseEvent* e) override;
        void mouseReleaseEvent(QMouseEvent* e) override;
    private:
        void initJacks();
        int getJackAt(QPoint pos);
       
        void drawPanel(QPainter& p, int x, int y, int w, int h, QString title);
        void drawScrew(QPainter& p, int x, int y);
        void drawFancyJack(QPainter& p, const JackDef& j);
        std::vector<JackDef> m_jacks;
        bool m_isDragging;
        int m_dragStartJack;
        QPoint m_currentMousePos;
    };
} // namespace gui
} // namespace lmms
#endif // MODULAR_ONE_H
