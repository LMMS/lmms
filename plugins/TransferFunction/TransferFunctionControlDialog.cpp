/*
 * TransferFunctionControlDialog.cpp
 */
#include "TransferFunctionControlDialog.h"
#include "TransferFunctionControls.h"
#include "embed.h"
#include "Knob.h"
#include "Graph.h"
#include "TransferFunctionLogic.h" 
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPalette>
#include <QMouseEvent> 
#include <QPainter>
#include <QPushButton>
#include <cmath>
namespace lmms::gui
{
// ============================================================
// CUSTOM WIDGET: LOG SCALE AXIS
// ============================================================
class LogScaleWidget : public QWidget
{
public:
    LogScaleWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedHeight(15); 
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setPen(QColor(100, 100, 100)); 
        p.setFont(QFont("Monospace", 7)); 
        float width = this->width();
        float minFreq = 20.0f;
        float maxFreq = 22050.0f;
        float logMin = std::log10(minFreq);
        float logMax = std::log10(maxFreq);
        float logRange = logMax - logMin;
        std::vector<float> ticks = { 100, 1000, 10000 };
        std::vector<QString> labels = { "100", "1k", "10k" };
        for(size_t i=0; i<ticks.size(); ++i) {
            float f = ticks[i];
            float logF = std::log10(f);
            float pct = (logF - logMin) / logRange;
            
            if (pct >= 0 && pct <= 1) {
                int x = (int)(pct * width);
                p.drawLine(x, 0, x, 3);
                p.drawText(x - 10, 12, labels[i]);
            }
        }
    }
};
// ============================================================
// DIALOG IMPLEMENTATION
// ============================================================
TransferFunctionControlDialog::TransferFunctionControlDialog(TransferFunctionControls* controls) :
    EffectControlDialog(controls),
    m_pluginControls(controls) 
{
    setAutoFillBackground(true);
    QPalette pal;
    pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
    setPalette(pal);
    setMinimumWidth(740); 
    setMinimumHeight(580); 
    QGridLayout* gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(10);
    // ============================================================
    // PRESET KNOB
    // ============================================================
    auto makeKnob = [this, controls](const QString& label, const QString& hintText,
                            const QString& unit, FloatModel* model)
    {
        Knob* newKnob = new Knob(KnobType::Bright26, label, this);
        newKnob->setModel(model);
        newKnob->setHintText(hintText, unit);
        return newKnob;
    };
    gridLayout->addWidget(
        makeKnob(tr("PRESET"), tr("Preset ID"), "#", &controls->m_volumeModel),
        0, 0, Qt::AlignHCenter
    );
    // ============================================================
    // FORMULA BOX CONTAINER (Label + Box)
    // ============================================================
    // We wrap them in a widget to stack them vertically on the right
    QWidget* formulaContainer = new QWidget(this);
    QVBoxLayout* formulaLayout = new QVBoxLayout(formulaContainer);
    formulaLayout->setContentsMargins(0,0,0,0);
    formulaLayout->setSpacing(2); // Tight spacing between "Mode 1" label and Box
    formulaLayout->setAlignment(Qt::AlignRight); 
    // 1. The Label "Mode 1 Entry"
    QLabel* entryLabel = new QLabel("Mode 1 Entry", this);
    entryLabel->setStyleSheet("color: #888; font-size: 9px; font-weight: bold; margin-right: 20px;");
    entryLabel->setAlignment(Qt::AlignRight);
    formulaLayout->addWidget(entryLabel);
    // 2. The Text Box
    QLineEdit* formulaBox = new QLineEdit(this);
    formulaBox->setMinimumWidth(350); 
    formulaBox->setPlaceholderText("e.g. 1 / sqrt(1 + (freq/800)^2)");
    formulaBox->setStyleSheet(
        "background-color: #222; color: #0F0; border: 1px solid #555; "
        "font-family: Monospace; font-size: 10px; padding: 2px; margin-right: 20px;"
    );
    formulaBox->setText(controls->getFormula());
    connect(formulaBox, &QLineEdit::textChanged, controls, &TransferFunctionControls::setFormula);
    formulaLayout->addWidget(formulaBox);
    
    // Add the whole container to Row 1. 
    // AlignTop pulls it "Higher" as requested.
    gridLayout->addWidget(formulaContainer, 1, 0, Qt::AlignRight | Qt::AlignTop);
    // ============================================================
    // ACTIVE FORMULA LABEL
    // ============================================================
    m_activeFormulaLabel = new QLabel(this);
    m_activeFormulaLabel->setStyleSheet("color: #888; font-size: 9px; font-style: italic; margin-right: 20px;");
    m_activeFormulaLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    gridLayout->addWidget(m_activeFormulaLabel, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
    
    connect(&controls->m_volumeModel, SIGNAL(dataChanged()), this, SLOT(updateFormulaLabel()));
    updateFormulaLabel(); 
    // ============================================================
    // INFO PANEL
    // ============================================================
    QLabel* infoLabel = new QLabel(this);
    infoLabel->setStyleSheet(
        "color: #ccc; font-size: 10px; font-family: Monospace; "
        "background-color: rgba(0,0,0,100); padding: 8px; border-radius: 4px;"
    );
    
    infoLabel->setText(
        "<b>PRESETS:</b>"
        "<table width='100%'>"
        "<tr><td>1: CUSTOM (Box)</td><td>10: Comb Notch</td></tr>"
        "<tr><td>2: Lowpass</td><td>11: Allpass</td></tr>"
        "<tr><td>3: Highpass</td><td>12: Bitcrusher</td></tr>"
        "<tr><td>4: Low Shelf</td><td>13: Spectral Decay</td></tr>"
        "<tr><td>5: High Shelf</td><td>14: Gate Sweep</td></tr>"
        "<tr><td>6: Telephone</td><td>15: Formants</td></tr>"
        "<tr><td>7: Notch</td><td>16: Odd Booster</td></tr>"
        "<tr><td>8: Resonator</td><td>17: Phase Tilt</td></tr>"
        "<tr><td>9: Comb</td><td><b>18: CUSTOM DRAW</b></td></tr>"
        "</table>"
        "<br>"
        "<b>CUSTOM SYNTAX (Preset 1):</b><br>"
        "Vars: <i>freq</i>, <i>j</i>, <i>pi</i> &nbsp;|&nbsp; Math: +, -, *, /, ^<br>"
        "Funcs: sqrt, exp, sin, cos, abs, log"
    );
    gridLayout->addWidget(infoLabel, 0, 1, 3, 1, Qt::AlignTop);
    gridLayout->setColumnStretch(0, 5);
    gridLayout->setColumnStretch(1, 4);
    // ============================================================
    // GRAPHS
    // ============================================================
    
    auto addGraphRow = [this, gridLayout, controls](int row, QString title, graphModel* model, QColor color, bool interactive) 
    {
        QWidget* container = new QWidget(this);
        QVBoxLayout* vLayout = new QVBoxLayout(container);
        vLayout->setContentsMargins(0,0,0,0);
        vLayout->setSpacing(2);
        // -- GRAPH AREA --
        QWidget* graphArea = new QWidget(container);
        QHBoxLayout* hLayout = new QHBoxLayout(graphArea);
        hLayout->setContentsMargins(0,0,0,0);
        hLayout->setSpacing(5);
        // 1. Label
        QLabel* lbl = new QLabel(title, this);
        lbl->setFixedWidth(50); 
        lbl->setStyleSheet("color: #BBB; font-size: 9px; font-weight: bold;");
        lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        hLayout->addWidget(lbl);
        // 2. Button Area
        if (interactive) {
            m_clearButton = new QPushButton("Clear", this);
            m_clearButton->setFixedWidth(50);
            m_clearButton->setStyleSheet("font-size: 9px; padding: 2px;");
            m_clearButton->setCursor(Qt::PointingHandCursor);
            connect(m_clearButton, &QPushButton::clicked, controls, &TransferFunctionControls::clearDrawing);
            hLayout->addWidget(m_clearButton);
        } else {
            // Invisible spacer
            QWidget* spacer = new QWidget(this);
            spacer->setFixedWidth(50); 
            hLayout->addWidget(spacer);
        }
        // 3. The Graph
        auto graph = new Graph(this, Graph::Style::LinearNonCyclic, 300, 100);
        graph->setModel(model);
        graph->setGraphColor(color);
        if (interactive) {
            graph->installEventFilter(this);
            graph->setObjectName("MagGraph");
        }
        hLayout->addWidget(graph);
        
        vLayout->addWidget(graphArea);
        // -- SCALE AREA --
        QWidget* scaleArea = new QWidget(container);
        QHBoxLayout* sLayout = new QHBoxLayout(scaleArea);
        sLayout->setContentsMargins(0,0,0,0);
        sLayout->setSpacing(5);
        // Spacer for Label (50) + Spacer for Button (50) + Spacing (5)
        // Total offset = 105px
        QLabel* offsetSpacer = new QLabel("", this);
        offsetSpacer->setFixedWidth(105); 
        sLayout->addWidget(offsetSpacer);
        LogScaleWidget* scale = new LogScaleWidget(this);
        sLayout->addWidget(scale);
        
        vLayout->addWidget(scaleArea);
        gridLayout->addWidget(container, row, 0, 1, 2);
    };
    // Add Mag Row (Interactive = true)
    addGraphRow(3, "MAG", &controls->m_bodeMagModel, QColor(0, 255, 0), true);
    // Add Phase Row (Interactive = false)
    addGraphRow(4, "PHASE", &controls->m_bodePhaseModel, QColor(255, 200, 0), false);
    
    connect(&controls->m_volumeModel, SIGNAL(dataChanged()), this, SLOT(updateUIState()));
    updateUIState();
}
void TransferFunctionControlDialog::updateFormulaLabel()
{
    if (!m_pluginControls) return;
    int preset = static_cast<int>(m_pluginControls->m_volumeModel.value());
    // Safe Namespaced call
    std::string formula = lmms::getPresetFormulaString(preset);
    m_activeFormulaLabel->setText(QString("Active Formula: %1").arg(QString::fromStdString(formula)));
}
void TransferFunctionControlDialog::updateUIState()
{
    if (m_pluginControls && m_clearButton) {
        int preset = static_cast<int>(m_pluginControls->m_volumeModel.value());
        
        if (preset == 18) {
            m_clearButton->setEnabled(true);
            m_clearButton->setText("Clear");
            m_clearButton->setStyleSheet("font-size: 9px; padding: 2px; color: #EEE; background-color: #444; border: 1px solid #666; border-radius: 3px;");
        } else {
            m_clearButton->setEnabled(false);
            m_clearButton->setText(""); 
            m_clearButton->setStyleSheet("background: transparent; border: none;");
        }
    }
}
// EVENT FILTER for Drawing
bool TransferFunctionControlDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (obj->objectName() == "MagGraph")
    {
        if (!m_pluginControls) return false;
        int preset = static_cast<int>(m_pluginControls->m_volumeModel.value());
        
        if (preset == 18 && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove))
        {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (me->buttons() & Qt::LeftButton)
            {
                QWidget* w = static_cast<QWidget*>(obj);
                float width = w->width();
                float height = w->height();
                
                float xPct = (float)me->x() / width;
                if(xPct < 0) xPct = 0; if(xPct > 1) xPct = 1;
                
                float minFreq = 20.0f;
                float maxFreq = 22050.0f; 
                float freq = minFreq * std::pow(maxFreq/minFreq, xPct);
                
                float yPct = (float)me->y() / height;
                if(yPct < 0) yPct = 0; if(yPct > 1) yPct = 1;
                float mag = 1.0f - yPct; 
                
                m_pluginControls->setDrawPoint(freq, mag);
                return true; 
            }
        }
    }
    return EffectControlDialog::eventFilter(obj, event);
}
} // namespace lmms::gui
