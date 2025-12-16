/* * TransferFunctionControlDialog.cpp */
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

// ... (Keep PoleZeroGraph & LogScaleWidget Implementations) ...

PoleZeroGraph::PoleZeroGraph(QWidget* parent, TransferFunctionControls* controls) 
    : QWidget(parent), m_controls(controls) { setMinimumHeight(200); setCursor(Qt::CrossCursor); setAutoFillBackground(false); }
void PoleZeroGraph::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.fillRect(rect(), QColor(30, 30, 30));
    int w = width(); int h = height(); int cx = w / 2; int cy = h / 2; float scale = std::min(w, h) * 0.4f;
    p.setPen(QPen(QColor(100, 100, 100), 1, Qt::DashLine)); p.drawEllipse(QPoint(cx, cy), (int)scale, (int)scale);
    p.setPen(QColor(60, 60, 60)); p.drawLine(cx, 0, cx, h); p.drawLine(0, cy, w, cy);
    p.setPen(QPen(QColor(255, 50, 50), 2));
    for(const auto& pole : m_controls->getPoles()) { int x = cx + (int)(pole.real() * scale); int y = cy - (int)(pole.imag() * scale); p.drawLine(x-4, y-4, x+4, y+4); p.drawLine(x-4, y+4, x+4, y-4); }
    p.setPen(QPen(QColor(50, 255, 255), 2)); p.setBrush(Qt::NoBrush);
    for(const auto& zero : m_controls->getZeros()) { int x = cx + (int)(zero.real() * scale); int y = cy - (int)(zero.imag() * scale); p.drawEllipse(QPoint(x, y), 4, 4); }
    p.setPen(QColor(150, 150, 150)); p.setFont(QFont("Monospace", 8)); p.drawText(5, 15, "L-Click: Pole (x) | R-Click: Zero (o) | Drag to Move");
}

void PoleZeroGraph::mousePressEvent(QMouseEvent* ev) {
    float scale = std::min(width(), height()) * 0.4f; float re = (ev->x() - width() / 2.0f) / scale; float im = -(ev->y() - height() / 2.0f) / scale;
    std::complex<float> clicked(re, im); float minDist = 0.2f; m_dragIndex = -1;
    auto poles = m_controls->getPoles(); for(size_t i=0; i<poles.size(); ++i) { if(std::abs(poles[i] - clicked) < minDist) { minDist = std::abs(poles[i] - clicked); m_dragIndex = i; m_dragIsPole = true; } }
    auto zeros = m_controls->getZeros(); for(size_t i=0; i<zeros.size(); ++i) { if(std::abs(zeros[i] - clicked) < minDist) { minDist = std::abs(zeros[i] - clicked); m_dragIndex = i; m_dragIsPole = false; } }
    if (m_dragIndex == -1) { if(ev->button() == Qt::LeftButton) m_controls->addPole(clicked); else if(ev->button() == Qt::RightButton) m_controls->addZero(clicked); update(); } 
    else { if (ev->button() == Qt::MiddleButton) { m_controls->removeClosestPoleZero(clicked); m_dragIndex = -1; update(); } }
}
void PoleZeroGraph::mouseMoveEvent(QMouseEvent* ev) {
    if (m_dragIndex != -1) { float scale = std::min(width(), height()) * 0.4f; float re = (ev->x() - width() / 2.0f) / scale; float im = -(ev->y() - height() / 2.0f) / scale; m_controls->updatePoleZero(m_dragIndex, m_dragIsPole, std::complex<float>(re, im)); update(); }
}

class LogScaleWidget : public QWidget{
public: LogScaleWidget(QWidget* parent = nullptr) : QWidget(parent) { setFixedHeight(15); setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); }
protected: void paintEvent(QPaintEvent*) override { /* ... Same painting code ... */ }
};

TransferFunctionControlDialog::TransferFunctionControlDialog(TransferFunctionControls* controls) :
    EffectControlDialog(controls), m_pluginControls(controls) 
{
    setAutoFillBackground(true); QPalette pal; pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork")); setPalette(pal);
    setMinimumWidth(740); setMinimumHeight(580); QGridLayout* gridLayout = new QGridLayout(this); gridLayout->setSpacing(10);
    auto makeKnob = [this, controls](const QString& label, const QString& hintText, const QString& unit, FloatModel* model) { Knob* newKnob = new Knob(KnobType::Bright26, label, this); newKnob->setModel(model); newKnob->setHintText(hintText, unit); return newKnob; };
    gridLayout->addWidget(makeKnob(tr("PRESET"), tr("Preset ID"), "#", &controls->m_volumeModel), 0, 0, Qt::AlignHCenter);

    QWidget* formulaContainer = new QWidget(this); QVBoxLayout* formulaLayout = new QVBoxLayout(formulaContainer); formulaLayout->setAlignment(Qt::AlignRight); 
    QLabel* entryLabel = new QLabel("Mode 1 Entry", this); entryLabel->setStyleSheet("color: #888; font-size: 9px; margin-right: 20px;"); formulaLayout->addWidget(entryLabel);
    QLineEdit* formulaBox = new QLineEdit(this); formulaBox->setMinimumWidth(350); formulaBox->setStyleSheet("background-color: #222; color: #0F0; border: 1px solid #555;"); formulaBox->setText(controls->getFormula());
    connect(formulaBox, &QLineEdit::textChanged, controls, &TransferFunctionControls::setFormula); formulaLayout->addWidget(formulaBox);
    gridLayout->addWidget(formulaContainer, 1, 0, Qt::AlignRight | Qt::AlignTop);

    m_activeFormulaLabel = new QLabel(this); m_activeFormulaLabel->setStyleSheet("color: #888; font-size: 9px; font-style: italic; margin-right: 20px;");
    gridLayout->addWidget(m_activeFormulaLabel, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
    connect(&controls->m_volumeModel, SIGNAL(dataChanged()), this, SLOT(updateFormulaLabel())); updateFormulaLabel(); 

    // --- RESTORED NOTES HERE ---
    QLabel* infoLabel = new QLabel(this);
    infoLabel->setStyleSheet("color: #ccc; font-size: 10px; font-family: Monospace; background-color: rgba(0,0,0,100); padding: 8px;");
    infoLabel->setText(

        "<b>PRESETS:</b><table width='100%'>"
        "<tr><td>1: CUSTOM (Box)</td><td>11: Allpass</td></tr>"
        "<tr><td>2: Lowpass</td><td>12: Bitcrusher</td></tr>"
        "<tr><td>3: Highpass</td><td>13: Spectral Decay</td></tr>"
        "<tr><td>4: Low Shelf</td><td>14: Gate Sweep</td></tr>"
        "<tr><td>5: High Shelf</td><td>15: Formants</td></tr>"
        "<tr><td>6: Telephone</td><td>16: Odd Booster</td></tr>"
        "<tr><td>7: Notch</td><td>17: Phase Tilt</td></tr>"
        "<tr><td>8: Resonator</td><td><b>18: CUSTOM DRAW</b></td></tr>"
        "<tr><td>9: Comb</td><td>19: Spectral Reverb</td></tr>"
        "<tr><td>10: Comb Notch</td><td><b>20: Pole/Zero Editor</b></td></tr>"
        "</table><br>"

        "<b>CUSTOM SYNTAX (Preset 1):</b><br>"
        "Vars: <i>freq</i>, <i>j</i>, <i>pi</i> &nbsp;|&nbsp; Math: +, -, *, /, ^<br>"
        "Funcs: sqrt, exp, sin, cos, abs, log"

    );

    gridLayout->addWidget(infoLabel, 0, 1, 3, 1, Qt::AlignTop);
    gridLayout->setColumnStretch(0, 5); gridLayout->setColumnStretch(1, 4);

    m_bodeContainer = new QWidget(this); QVBoxLayout* bodeLayout = new QVBoxLayout(m_bodeContainer); bodeLayout->setContentsMargins(0,0,0,0);
    auto addGraph = [this, controls, bodeLayout](QString title, graphModel* model, QColor col, bool interact) {
        QWidget* wrap = new QWidget(this); QHBoxLayout* hl = new QHBoxLayout(wrap); hl->setContentsMargins(0,0,0,0);
        QLabel* lb = new QLabel(title, this); lb->setFixedWidth(50); lb->setStyleSheet("color:#BBB; font-weight:bold;"); hl->addWidget(lb);
        if (interact) { m_clearButton = new QPushButton("Clear", this); m_clearButton->setFixedWidth(50); connect(m_clearButton, &QPushButton::clicked, controls, &TransferFunctionControls::clearDrawing); hl->addWidget(m_clearButton); } 
        else { QWidget* sp = new QWidget(this); sp->setFixedWidth(50); hl->addWidget(sp); }
        auto g = new Graph(this, Graph::Style::LinearNonCyclic, 300, 100); g->setModel(model); g->setGraphColor(col); if(interact) { g->installEventFilter(this); g->setObjectName("MagGraph"); } hl->addWidget(g); bodeLayout->addWidget(wrap);
    };
    addGraph("MAG", &controls->m_bodeMagModel, QColor(0, 255, 0), true); addGraph("PHASE", &controls->m_bodePhaseModel, QColor(255, 200, 0), false);
    LogScaleWidget* sc = new LogScaleWidget(this); bodeLayout->addWidget(sc); gridLayout->addWidget(m_bodeContainer, 3, 0, 2, 2);
    m_poleZeroGraph = new PoleZeroGraph(this, controls); m_poleZeroGraph->setVisible(false); gridLayout->addWidget(m_poleZeroGraph, 3, 0, 2, 2);
    connect(&controls->m_volumeModel, SIGNAL(dataChanged()), this, SLOT(updateUIState())); updateUIState();
}

void TransferFunctionControlDialog::updateFormulaLabel() {
    int preset = static_cast<int>(m_pluginControls->m_volumeModel.value());
    m_activeFormulaLabel->setText(QString("Formula: %1").arg(QString::fromStdString(lmms::getPresetFormulaString(preset))));
}

void TransferFunctionControlDialog::updateUIState() {
    if (!m_pluginControls) return;
    int preset = static_cast<int>(m_pluginControls->m_volumeModel.value());
    if (preset == 20) { m_bodeContainer->setVisible(false); m_poleZeroGraph->setVisible(true); } else { m_bodeContainer->setVisible(true); m_poleZeroGraph->setVisible(false); }
    if (m_clearButton) { if (preset == 18) { m_clearButton->setEnabled(true); m_clearButton->setText("Clear"); } else { m_clearButton->setEnabled(false); m_clearButton->setText(""); } }
}

bool TransferFunctionControlDialog::eventFilter(QObject* obj, QEvent* event) {
    if (obj->objectName() == "MagGraph") {
        if (!m_pluginControls) return false; int preset = static_cast<int>(m_pluginControls->m_volumeModel.value());
        if (preset == 18 && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove)) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event); if (me->buttons() & Qt::LeftButton) {
                QWidget* w = static_cast<QWidget*>(obj); float xPct = (float)me->x() / w->width(); if(xPct<0)xPct=0; if(xPct>1)xPct=1;
                float minF=20.0f, maxF=22050.0f; float freq = minF * std::pow(maxF/minF, xPct); float yPct = (float)me->y() / w->height(); if(yPct<0)yPct=0; if(yPct>1)yPct=1;
                m_pluginControls->setDrawPoint(freq, 1.0f - yPct); return true; 
            }
        }
    }

    return EffectControlDialog::eventFilter(obj, event);
}

} // namespace lmms::gui
