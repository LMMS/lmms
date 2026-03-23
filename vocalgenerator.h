#ifndef VOCALGENERATOR_H
#define VOCALGENERATOR_H

#include <QWidget>
#include <QTextEdit>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

class VocalGeneratorTab : public QWidget {
    Q_OBJECT

public:
    explicit VocalGeneratorTab(QWidget *parent = nullptr);

private slots:
    void generateExpressions();
    void updateLabels();

private:
    QComboBox *targetVowelCombo;

    QSlider *morphTimeSlider;
    QLabel *morphTimeLabel;

    QSlider *decaySlider;
    QLabel *decayLabel;

    QSlider *vibratoRateSlider;
    QLabel *vibratoRateLabel;

    QSlider *vibratoDepthSlider;
    QLabel *vibratoDepthLabel;

    QSlider *phaseSlider;
    QLabel *phaseLabel;

    QCheckBox *nightlyCheckBox;
    QPushButton *generateButton;

    QTextEdit *w1Output;
    QTextEdit *w2Output;
    QTextEdit *o1Output;

    void setupUI();
    QString buildHarmonicSeries(double amps[], int count, bool nightly);
};

#endif // VOCALGENERATOR_H
