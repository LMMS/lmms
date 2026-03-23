#include "melodyrenderertab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QDomDocument>
#include <QFile>
#include <QPainter>
#include <QMouseEvent>
#include <cmath>
#include <QRegularExpression>
#include <QApplication>
#include <QClipboard>


PatternVisualizer::PatternVisualizer(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(150);
    setStyleSheet("background-color: #1e1e1e; border: 1px solid #444;");
}

void PatternVisualizer::setNotes(const QVector<MonoNote>& notes, int steps) {
    m_notes = notes;
    m_totalSteps = steps > 0 ? steps : 16;
    update();
}

void PatternVisualizer::setBpm(double bpm) { m_bpm = bpm; update(); }
void PatternVisualizer::setSampleLengthSec(double sec) { m_sampleLengthSec = sec; update(); }

void PatternVisualizer::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) return;

    int w = width(); int h = height();
    double stepWidth = (double)w / m_totalSteps;

    int minKey = 127, maxKey = 0;
    for (const auto& note : m_notes) {
        if (note.key < minKey) minKey = note.key;
        if (note.key > maxKey) maxKey = note.key;
    }
    int keyRange = qMax(12, maxKey - minKey + 4);


    for (int i = m_notes.size() - 1; i >= 0; --i) {
        const auto& note = m_notes[i];
        double startX = (note.pos / 48.0) * stepWidth;
        double noteW = (note.len / 48.0) * stepWidth;
        double normalizedY = 1.0 - ((double)(note.key - minKey + 2) / keyRange);
        double startY = normalizedY * h;
        double noteH = h / keyRange;


        QRectF noteRect(startX - 3, startY - 3, noteW + 6, noteH + 6);

        if (noteRect.contains(event->pos())) {
            m_isDraggingNote = true;
            m_dragNoteIndex = i;
            m_dragStartX = event->pos().x();
            m_dragStartY = event->pos().y();
            m_dragNoteStartPos = note.pos;
            m_dragNoteStartKey = note.key;
            m_dragPixelsPerKey = (double)h / keyRange; // Lock the zoom scale here!
            return;
        }
    }

    m_isDraggingSample = true;
    m_dragStartX = event->pos().x();
    m_dragStartOffset = m_offsetTicks;
}

void PatternVisualizer::mouseMoveEvent(QMouseEvent *event) {
    if (!m_isDraggingNote && !m_isDraggingSample) return;

    double stepWidth = (double)width() / m_totalSteps;
    double ticksPerPixel = 48.0 / stepWidth;

    if (m_isDraggingNote && m_dragNoteIndex >= 0 && m_dragNoteIndex < m_notes.size()) {
        int dx = event->pos().x() - m_dragStartX;
        int dy = event->pos().y() - m_dragStartY;


        m_notes[m_dragNoteIndex].pos = qMax(0.0, m_dragNoteStartPos + (dx * ticksPerPixel));


        int keyDelta = -qRound(dy / m_dragPixelsPerKey);
        m_notes[m_dragNoteIndex].key = qBound(0, m_dragNoteStartKey + keyDelta, 127);

        update();
        emit notesChanged(m_notes);
    }
    else if (m_isDraggingSample) {
        int dx = event->pos().x() - m_dragStartX;
        m_offsetTicks = m_dragStartOffset + (dx * ticksPerPixel);
        update();
        emit offsetChanged();
    }
}

void PatternVisualizer::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDraggingSample = false;
        m_isDraggingNote = false;
        m_dragNoteIndex = -1;
    }
}

void PatternVisualizer::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(); int h = height();
    double stepWidth = (double)w / m_totalSteps;


    p.setPen(QColor(60, 60, 60));
    for (int i = 0; i <= m_totalSteps; ++i) {
        int x = i * stepWidth;
        p.drawLine(x, 0, x, h);
    }


    double secondsPerTick = 60.0 / (m_bpm * 48.0);
    double sampleTicks = m_sampleLengthSec / secondsPerTick;
    double sampleX = (m_offsetTicks / 48.0) * stepWidth;
    double sampleW = (sampleTicks / 48.0) * stepWidth;

    p.setPen(QPen(QColor(100, 255, 100), 2));
    p.setBrush(QColor(50, 200, 50, 60));
    p.drawRect(sampleX, 5, sampleW, h - 10);
    p.setPen(Qt::white);
    p.drawText(sampleX + 5, 20, "Sample Area (Drag Me)");


    if (m_notes.isEmpty()) return;
    int minKey = 127, maxKey = 0;
    for (const auto& note : m_notes) {
        if (note.key < minKey) minKey = note.key;
        if (note.key > maxKey) maxKey = note.key;
    }
    int keyRange = qMax(12, maxKey - minKey + 4);

    p.setPen(Qt::NoPen);
    for (const auto& note : m_notes) {
        double startX = (note.pos / 48.0) * stepWidth;
        double noteW = (note.len / 48.0) * stepWidth;
        double normalizedY = 1.0 - ((double)(note.key - minKey + 2) / keyRange);
        double startY = normalizedY * h;
        double noteH = h / keyRange;

        p.setBrush(QColor(0, 180, 255, 180));
        p.drawRoundedRect(startX, startY, noteW, noteH, 2, 2);
    }
}


MelodyRendererTab::MelodyRendererTab(QWidget *parent) : QWidget(parent), m_patternSteps(16) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);


    QHBoxLayout* topLayout = new QHBoxLayout();
    m_btnLoadPattern = new QPushButton("Load Pattern (.xpt)");

    m_bpmSpin = new QSpinBox(); m_bpmSpin->setRange(20, 300); m_bpmSpin->setValue(140);
    m_octaveSpin = new QSpinBox(); m_octaveSpin->setRange(-4, 4); m_octaveSpin->setValue(0);

    m_sampleLenSpin = new QDoubleSpinBox();
    m_sampleLenSpin->setRange(0.1, 10.0);
    m_sampleLenSpin->setValue(2.0);
    m_sampleLenSpin->setSuffix("s");

    m_glideCheck = new QCheckBox("Glides");
    m_versionCombo = new QComboBox(); m_versionCombo->addItems({"Legacy", "Nightly"});

    topLayout->addWidget(m_btnLoadPattern);
    topLayout->addWidget(new QLabel("BPM:")); topLayout->addWidget(m_bpmSpin);
    topLayout->addWidget(new QLabel("Sample Len:")); topLayout->addWidget(m_sampleLenSpin);
    topLayout->addWidget(new QLabel("Octave:")); topLayout->addWidget(m_octaveSpin);
    topLayout->addWidget(m_glideCheck);
    topLayout->addWidget(m_versionCombo);
    topLayout->addStretch();


    QLabel* inputLabel = new QLabel("<b>1. Input Base PCM Formula:</b> (Enter Nightly String)");
    m_inputPcmEdit = new QPlainTextEdit();
    m_inputPcmEdit->setPlaceholderText("");
    m_inputPcmEdit->setMaximumHeight(80);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_btnFlattenPitch = new QPushButton("Flatten Pitch (Set to 440)");
    m_btnGenerate = new QPushButton("Generate Optimized Melody");
    m_btnGenerate->setStyleSheet("background-color: #008800; color: white; font-weight: bold;");
    btnLayout->addWidget(m_btnFlattenPitch);
    btnLayout->addWidget(m_btnGenerate);
    btnLayout->addStretch();

    m_visualizer = new PatternVisualizer();

    QHBoxLayout* outputHeaderLayout = new QHBoxLayout();
    QLabel* outputLabel = new QLabel("<b>2. Output Generated Expression:</b> (Use this in LMMS!)");
    m_btnCopyOutput = new QPushButton("📋 Copy to Clipboard");
    m_btnCopyOutput->setStyleSheet("background-color: #555555; color: white;");
    outputHeaderLayout->addWidget(outputLabel);
    outputHeaderLayout->addStretch();
    outputHeaderLayout->addWidget(m_btnCopyOutput);

    m_outputExprEdit = new QPlainTextEdit();
    m_outputExprEdit->setReadOnly(true);

    m_btnPlay = new QPushButton("▶ Play via SynthEngine");
    m_btnPlay->setStyleSheet("background-color: #0066cc; color: white; padding: 5px;");


    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(inputLabel);
    mainLayout->addWidget(m_inputPcmEdit);
    mainLayout->addLayout(btnLayout);
    mainLayout->addWidget(m_visualizer);
    mainLayout->addLayout(outputHeaderLayout);
    mainLayout->addWidget(m_outputExprEdit);
    mainLayout->addWidget(m_btnPlay);


    connect(m_btnLoadPattern, &QPushButton::clicked, this, &MelodyRendererTab::onLoadPattern);
    connect(m_btnFlattenPitch, &QPushButton::clicked, this, &MelodyRendererTab::onFlattenPitch);
    connect(m_btnGenerate, &QPushButton::clicked, this, &MelodyRendererTab::onGenerateExpression);
    connect(m_btnPlay, &QPushButton::clicked, this, &MelodyRendererTab::onPlay);
    connect(m_bpmSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val){ m_visualizer->setBpm(val); });
    connect(m_sampleLenSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double val){ m_visualizer->setSampleLengthSec(val); });
    connect(m_btnCopyOutput, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_outputExprEdit->toPlainText());
        QMessageBox::information(this, "Copied", "Expression copied to clipboard!");
    });
    connect(m_inputPcmEdit, &QPlainTextEdit::textChanged, this, &MelodyRendererTab::onPcmTextChanged);

    connect(m_visualizer, &PatternVisualizer::notesChanged, this, [this](const QVector<MonoNote>& notes){ m_loadedNotes = notes; onGenerateExpression(); });
    connect(m_visualizer, &PatternVisualizer::offsetChanged, this, &MelodyRendererTab::onGenerateExpression);
}

void MelodyRendererTab::onLoadPattern() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Pattern", "", "Pattern Files (*.xpt *.mmp)");
    if (!fileName.isEmpty() && parseXptFile(fileName)) {
        m_visualizer->setNotes(m_loadedNotes, m_patternSteps);
    }
}

bool MelodyRendererTab::parseXptFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QDomDocument doc; doc.setContent(&file); file.close();

    m_loadedNotes.clear();
    QDomNodeList patternNodes = doc.elementsByTagName("pattern");
    if (patternNodes.isEmpty()) patternNodes = doc.elementsByTagName("midiclip");
    if (patternNodes.isEmpty()) return false;

    QDomElement patternElem = patternNodes.at(0).toElement();
    if (patternElem.hasAttribute("steps")) m_patternSteps = patternElem.attribute("steps").toInt();

    QDomNodeList noteNodes = patternElem.elementsByTagName("note");
    for (int i = 0; i < noteNodes.count(); ++i) {
        QDomElement n = noteNodes.at(i).toElement();
        MonoNote note;
        note.key = n.attribute("key").toInt();
        note.pos = n.attribute("pos").toDouble();
        note.len = n.attribute("len").toDouble();
        m_loadedNotes.push_back(note);
    }
    return !m_loadedNotes.isEmpty();
}

void MelodyRendererTab::onFlattenPitch() {
    QString pcm = m_inputPcmEdit->toPlainText();
    pcm.replace(QRegularExpression("\\b(f|p)\\b"), "440.0");
    m_inputPcmEdit->setPlainText(pcm);
}

double MelodyRendererTab::keyToFreq(int key, int octaveOffset) {
    return 440.0 * std::pow(2.0, (key + (octaveOffset * 12) - 69) / 12.0);
}


double MelodyRendererTab::detectSampleLength(const QString& pcm) {

    QRegularExpression reT("t\\s*<\\s*=?\\s*([0-9]*\\.?[0-9]+)");
    QRegularExpressionMatchIterator iT = reT.globalMatch(pcm);
    double maxLen = -1.0;
    while (iT.hasNext()) {
        double val = iT.next().captured(1).toDouble();
        if (val > maxLen) maxLen = val;
    }


    if (maxLen <= 0.0) {

        QRegularExpression reSr("floor\\(\\s*t\\s*\\*\\s*([0-9]+(?:\\.[0-9]+)?)\\s*\\)");
        QRegularExpressionMatch srMatch = reSr.match(pcm);
        double sr = srMatch.hasMatch() ? srMatch.captured(1).toDouble() : 8000.0;


        QRegularExpression reS("s\\s*<\\s*=?\\s*([0-9]+)");
        QRegularExpressionMatchIterator iS = reS.globalMatch(pcm);
        double maxS = -1.0;
        while (iS.hasNext()) {
            double val = iS.next().captured(1).toDouble();
            if (val > maxS) maxS = val;
        }


        if (maxS > 0.0) {
            maxLen = maxS / sr;
        }
    }

    return maxLen;
}

void MelodyRendererTab::onPcmTextChanged() {
    QString pcm = m_inputPcmEdit->toPlainText();
    double detectedLen = detectSampleLength(pcm);
    if (detectedLen > 0.0) {
        m_sampleLenSpin->setValue(detectedLen);
    }
}

void MelodyRendererTab::onGenerateExpression() {
    if (m_loadedNotes.isEmpty()) {
        m_outputExprEdit->setPlainText("⚠️ Error: No pattern loaded!");
        return;
    }

    QString basePcm = m_inputPcmEdit->toPlainText().trimmed();
    if (basePcm.isEmpty()) basePcm = "sinew(t*440.0)";

    double bpm = m_bpmSpin->value();
    double secondsPerTick = 60.0 / (bpm * 48.0);
    double offsetSec = m_visualizer->getOffsetTicks() * secondsPerTick;


    bool isPcm = basePcm.contains("floor(t", Qt::CaseInsensitive) || basePcm.contains("s <=", Qt::CaseInsensitive);

    QStringList gateTerms;
    QStringList freqTerms;
    QStringList localTTerms;

    double baseFreq = keyToFreq(60, 0);

    QVector<MonoNote> sortedNotes = m_loadedNotes;
    std::sort(sortedNotes.begin(), sortedNotes.end(), [](const MonoNote& a, const MonoNote& b) {
        return a.pos < b.pos;
    });

    for (int i = 0; i < sortedNotes.size(); ++i) {
        const MonoNote& current = sortedNotes[i];

        double startTime = (current.pos * secondsPerTick) - offsetSec;
        double endTime = (i < sortedNotes.size() - 1) ?
                             (sortedNotes[i+1].pos * secondsPerTick) - offsetSec :
                             ((current.pos + current.len) * secondsPerTick) - offsetSec;

        if (endTime <= 0) continue;
        if (startTime < 0) startTime = 0;

        double freq = keyToFreq(current.key, m_octaveSpin->value());
        double pitchRatio = freq / baseFreq; // Speed multiplier (1.0 = normal speed)

        QString timeCond = QString("(t>=%1)*(t<%2)").arg(startTime, 0, 'f', 3).arg(endTime, 0, 'f', 3);
        gateTerms.append(timeCond);

        QString localT = QString("(%1 * (t - %2) * %3)").arg(timeCond).arg(startTime, 0, 'f', 4).arg(pitchRatio, 0, 'f', 4);
        localTTerms.append(localT);

        freqTerms.append(QString("(%1 * %2)").arg(timeCond).arg(freq, 0, 'f', 2));
    }

    if (gateTerms.isEmpty()) {
        m_outputExprEdit->setPlainText("0");
        return;
    }

    QString gateEnvelope = "(" + gateTerms.join(" + ") + ")";
    QString freqEnvelope = "(" + freqTerms.join(" + ") + ")";
    QString localTEnvelope = "(" + localTTerms.join(" + ") + ")";

    bool isNightly = (m_versionCombo->currentText() == "Nightly");
    QString finalExpr;

    if (isNightly) {
        QString injectedPcm = basePcm;
        if (isPcm) {

            injectedPcm.replace(QRegularExpression("\\bt\\b"), "local_t");
        } else {

            if (injectedPcm.contains("440.0")) injectedPcm.replace("440.0", "m_pitch");
            else injectedPcm.replace(QRegularExpression("\\b(f|p)\\b"), "m_pitch");
        }

        finalExpr = QString("var m_pitch := %1;\nvar m_gate := %2;\nvar local_t := %3;\n\nm_gate * {\n%4\n}")
                        .arg(freqEnvelope)
                        .arg(gateEnvelope)
                        .arg(localTEnvelope)
                        .arg(injectedPcm);
    } else {
        QString injectedPcm = basePcm;

        QRegularExpression varRe("var\\s+([a-zA-Z0-9_]+)\\s*:=\\s*([^;]+);");
        QRegularExpressionMatchIterator it = varRe.globalMatch(injectedPcm);
        QMap<QString, QString> vars;
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            vars[match.captured(1)] = match.captured(2).trimmed();
        }
        injectedPcm.remove(QRegularExpression("var\\s+[a-zA-Z0-9_]+\\s*:=\\s*[^;]+;\\s*"));
        for (auto varIt = vars.begin(); varIt != vars.end(); ++varIt) {
            injectedPcm.replace(QRegularExpression("\\b" + varIt.key() + "\\b"), "(" + varIt.value() + ")");
        }

        if (isPcm) {

            injectedPcm.replace(QRegularExpression("\\bt\\b"), localTEnvelope);
        } else {
            if (injectedPcm.contains("440.0")) injectedPcm.replace("440.0", freqEnvelope);
            else injectedPcm.replace(QRegularExpression("\\b(f|p)\\b"), freqEnvelope);
        }

        finalExpr = QString("(%1) * (%2)").arg(gateEnvelope).arg(injectedPcm);
    }

    m_outputExprEdit->setPlainText(finalExpr);
}

void MelodyRendererTab::onPlay() {
    emit playRequested(m_outputExprEdit->toPlainText(), m_bpmSpin->value());
}
