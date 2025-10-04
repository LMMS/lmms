#pragma once

#include "ToolPluginView.h"
#include <QWidget>

class QPushButton;
class QLabel;

namespace lmms {
class AudioRecorder;

namespace gui {

class AudioRecorderView final : public ToolPluginView {
    Q_OBJECT
public:
    explicit AudioRecorderView(AudioRecorder* plugin, QWidget* parent = nullptr);
    ~AudioRecorderView() override = default;

private:
    AudioRecorder* m_plugin{};
    QPushButton*   m_startBtn{};
    QPushButton*   m_stopBtn{};
    QLabel*        m_pathVal{};
};

} // namespace gui
} // namespace lmms
