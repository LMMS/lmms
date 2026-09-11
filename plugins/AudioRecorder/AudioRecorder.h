#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include <QString>
#include "ToolPlugin.h"


namespace lmms {

class AudioRecorder final : public ToolPlugin {
public:
    AudioRecorder();
    ~AudioRecorder() override;
	const QString& lastPath() const { return m_lastPath; }
    void start();
    void stop();

    gui::PluginView* instantiateView(QWidget*) override;
    QString nodeName() const override;

    void saveSettings(QDomDocument&, QDomElement&) override {}
    void loadSettings(const QDomElement&) override {}

    const QString& lastPath() const { return m_lastPath; }  // <— expose last path

private:
    static QString makeDefaultOutPath();                     // <— one place to decide path
    private:
    QString              m_lastPath;   // final file path used by the running recording
    std::vector<float>   m_buffer;     // (optional) in-memory float buffer
    std::atomic<bool>    m_recording{false};
    std::thread          m_worker;
};

} // namespace lmms
