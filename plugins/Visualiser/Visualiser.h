#pragma once

#include <lmms/Effect.h>
#include <lmms/EffectControls.h>
#include <lmms/EffectControlDialog.h>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QObject>
#include <QImage>
#include <QDir>
#include <complex>
#include <vector>
#include <deque>

// Forward declaration
class VisualiserScreen;

// --- DATA STRUCTURES ---
struct Particle {
    float x, y;
    float vx, vy;
    int life;
    QColor color;
};

struct Point3D {
    float x, y, z;
};

struct Star {
    float x, y, z;
};

// ================================================================
// 1. CONTROLS CLASS
// ================================================================
class VisualiserEffectControls : public lmms::EffectControls
{
    Q_OBJECT 

public:
    explicit VisualiserEffectControls(lmms::Effect *eff)
        : lmms::EffectControls(eff)
        , m_mode(0)
        , m_showText(true) 
        , m_wiggleText(false)
        , m_recResolution(0) // 0=Window, 1=720p, 2=1080p, 3=4K
        , m_text(QString::fromUtf8("YOUR TEXT")) 
    {
    }

    int controlCount() override { return 1; }

    void saveSettings(QDomDocument &doc, QDomElement &parent) override;
    void loadSettings(const QDomElement &parent) override;

    QString nodeName() const override { return "VisualiserControls"; }

    lmms::gui::EffectControlDialog *createView() override;

    int mode() const { return m_mode; }
    void setMode(int m) { m_mode = m; }

    bool showText() const { return m_showText; }
    void setShowText(bool show) { m_showText = show; }

    bool wiggleText() const { return m_wiggleText; }
    void setWiggleText(bool wiggle) { m_wiggleText = wiggle; }

    int recResolution() const { return m_recResolution; }
    void setRecResolution(int r) { m_recResolution = r; }

    QString text() const { return m_text; }
    void setText(const QString &t) { m_text = t; }

private:
    int m_mode;
    bool m_showText;
    bool m_wiggleText;
    int m_recResolution;
    QString m_text;
};

// ================================================================
// 2. DIALOG CLASS
// ================================================================
class VisualiserControlDialog : public lmms::gui::EffectControlDialog
{
    Q_OBJECT

public:
    explicit VisualiserControlDialog(lmms::EffectControls *controls);

private slots:
    void onModeChanged(int index);
    void onShowTextChanged(bool checked);
    void onWiggleTextChanged(bool checked);
    void onRecToggled(bool checked);
    void onResChanged(int index); 
    void onTextChanged(const QString &text);

private:
    VisualiserEffectControls *m_controls;
    VisualiserScreen         *m_screen;
    QComboBox                *m_modeCombo;
    QCheckBox                *m_textCheck;
    QCheckBox                *m_wiggleCheck;
    QCheckBox                *m_recCheck; // Record Button
    QComboBox                *m_resCombo; // Resolution Dropdown
    QLineEdit                *m_textBox;
};
