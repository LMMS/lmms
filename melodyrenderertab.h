#ifndef MELODYRENDERERTAB_H
#define MELODYRENDERERTAB_H

#include <QWidget>
#include <QVector>
#include <QString>

class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QLabel;

struct MonoNote {
    int key;
    double pos;
    double len;
};

class PatternVisualizer : public QWidget {
    Q_OBJECT
public:
    explicit PatternVisualizer(QWidget *parent = nullptr);
    void setNotes(const QVector<MonoNote>& notes, int steps);
    void setBpm(double bpm);
    void setSampleLengthSec(double sec);

    double getOffsetTicks() const { return m_offsetTicks; }

signals:
    void offsetChanged();
    void notesChanged(const QVector<MonoNote>& newNotes);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QVector<MonoNote> m_notes;
    int m_totalSteps = 16;
    double m_bpm = 140.0;
    double m_sampleLengthSec = 2.0;
    double m_offsetTicks = 0.0;

    bool m_isDraggingSample = false;
    bool m_isDraggingNote = false;
    int m_dragNoteIndex = -1;

    int m_dragStartX = 0;
    int m_dragStartY = 0;
    double m_dragStartOffset = 0.0;
    double m_dragNoteStartPos = 0.0;
    int m_dragNoteStartKey = 0;
    double m_dragPixelsPerKey = 10.0;
};

class MelodyRendererTab : public QWidget {
    Q_OBJECT

public:
    explicit MelodyRendererTab(QWidget *parent = nullptr);

signals:
    void playRequested(const QString& pcmString, double bpm);

private slots:
    void onLoadPattern();
    void onFlattenPitch();
    void onGenerateExpression();
    void onPlay();
    void onPcmTextChanged();

private:
    bool parseXptFile(const QString& filePath);
    double keyToFreq(int key, int octaveOffset);

    QPlainTextEdit* m_inputPcmEdit;
    QPlainTextEdit* m_outputExprEdit;

    QPushButton* m_btnLoadPattern;
    QPushButton* m_btnFlattenPitch;
    QPushButton* m_btnGenerate;
    QPushButton* m_btnPlay;

    QSpinBox* m_bpmSpin;
    QSpinBox* m_octaveSpin;
    QDoubleSpinBox* m_sampleLenSpin;
    QComboBox* m_versionCombo;
    QCheckBox* m_glideCheck;

    PatternVisualizer* m_visualizer;
    QVector<MonoNote> m_loadedNotes;
    QPushButton* m_btnCopyOutput;
    double detectSampleLength(const QString& pcm);
    int m_patternSteps;
};

#endif // MELODYRENDERERTAB_H
