/*

 * TransferFunctionControlDialog.cpp - control dialog for TransferFunction effect

 */



#include "TransferFunctionControlDialog.h"

#include "TransferFunctionControls.h"

#include "embed.h"

#include "Knob.h"

#include "Graph.h"

#include <QGridLayout>

#include <QHBoxLayout> // Added for side labels

#include <QLineEdit>

#include <QLabel>

#include <QPalette>



namespace lmms::gui

{



TransferFunctionControlDialog::TransferFunctionControlDialog(TransferFunctionControls* controls) :

    EffectControlDialog(controls)

{

    setAutoFillBackground(true);

    QPalette pal;

    pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));

    setPalette(pal);



    // WINDOW SIZE

    setMinimumWidth(700);

    setMinimumHeight(500);



    QGridLayout* gridLayout = new QGridLayout(this);



    // ============================================================

    // LEFT COLUMN: PRESET KNOB + FORMULA BOX

    // ============================================================

    auto makeKnob = [this](const QString& label, const QString& hintText,

                            const QString& unit, FloatModel* model, bool isVolume)

    {

        Knob* newKnob = new Knob(KnobType::Bright26, label, this);

        newKnob->setModel(model);

        newKnob->setHintText(hintText, unit);

        return newKnob;

    };



    // PRESET knob

    gridLayout->addWidget(

        makeKnob(tr("PRESET"), tr("Preset ID"), "#", &controls->m_volumeModel, false),

        0, 0, Qt::AlignHCenter

    );



    // FORMULA TEXTBOX

    QLineEdit* formulaBox = new QLineEdit(this);

    formulaBox->setMinimumWidth(600);

    formulaBox->setPlaceholderText("e.g. 1 / sqrt(1 + (freq/800)^2)");

    formulaBox->setStyleSheet(

        "background-color: #222;"

        "color: #0F0;"

        "border: 1px solid #555;"

        "font-family: Monospace;"

        "font-size: 10px;"

        "padding: 2px;"

        "margin-top: 15px;"

    );

    formulaBox->setText(controls->getFormula());

    connect(formulaBox, &QLineEdit::textChanged, controls, &TransferFunctionControls::setFormula);

    gridLayout->addWidget(formulaBox, 1, 0, Qt::AlignHCenter);





    // ============================================================

    // RIGHT COLUMN: TEXT INFO PANEL

    // ============================================================

    QLabel* infoLabel = new QLabel(this);

    infoLabel->setStyleSheet(

        "color: #ccc;"

        "font-size: 10px;"

        "font-family: Monospace;"

        "background-color: rgba(0,0,0,100);"

        "padding: 10px;"

        "border-radius: 5px;"

    );

    infoLabel->setText(

        "<b>PRESETS:</b><br>"

        "1: CUSTOM (Use Box Below) &nbsp;&nbsp; 10: Comb Notch<br>"

        "2: Lowpass &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 11: Allpass<br>"        "3: Highpass &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 12: Bitcrusher<br>"

        "4: Low Shelf &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 13: Spectral Decay<br>"

        "5: High Shelf &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 14: Gate Sweep<br>"

        "6: Telephone &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 15: Formants<br>"

        "7: Notch &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 16: Odd Booster<br>"

        "8: Resonator &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 17: Phase Tilt<br>"

        "9: Comb<br>"

        "<br>"

        "<b>CUSTOM SYNTAX (Preset 1):</b><br>"

        "Vars: <i>freq</i> (Hz), <i>j</i> (Imag), <i>pi</i><br>"

        "Math: +, -, *, /, ^ , ( )<br>"

        "Funcs: sqrt, exp, sin, cos, abs, log<br>"

        "Ex: <code>1 / sqrt(1 + (freq/800)^2)</code>"

    );

    gridLayout->addWidget(infoLabel, 0, 1, 2, 1, Qt::AlignLeft);



    gridLayout->setColumnStretch(0, 4);

    gridLayout->setColumnStretch(1, 5);





    // ============================================================

    // BODE PLOTS (With Side Labels)

    // ============================================================



    // HELPER to create [Label] -> [Graph] row

    auto addLabeledGraph = [this, gridLayout](int row, QString text, graphModel* model, QColor color) {

        QWidget* container = new QWidget(this);

        QHBoxLayout* layout = new QHBoxLayout(container);

        layout->setContentsMargins(0,0,0,0);

        

        // Label

        QLabel* lbl = new QLabel(text, this);

        lbl->setFixedWidth(60); 

        lbl->setStyleSheet("color: #BBB; font-size: 9px; font-weight: bold;");

        lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        

        // Graph

        auto graph = new Graph(this, Graph::Style::LinearNonCyclic, 300, 100);

        graph->setModel(model);

        graph->setGraphColor(color);



        layout->addWidget(lbl);

        layout->addWidget(graph);



        // Add to main grid (span 2 columns)

        gridLayout->addWidget(container, row, 0, 1, 2);

    };



    // Add Magnitude Row

    addLabeledGraph(2, "MAG\n(dB)", &controls->m_bodeMagModel, QColor(0, 255, 0));



    // Add Phase Row

    addLabeledGraph(3, "PHASE\n(rad)", &controls->m_bodePhaseModel, QColor(255, 200, 0));



}



} // namespace lmms::gui
