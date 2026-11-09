#include "AudioRecorder.h"
#include "AudioRecorderView.h"
#include "embed.h"
#include "plugin_export.h"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

#include <sndfile.h>

#ifdef __linux__
#include <alsa/asoundlib.h>
#endif

namespace lmms {

extern "C" {
Plugin::Descriptor PLUGIN_EXPORT audiorecorder_plugin_descriptor = {
    LMMS_STRINGIFY(PLUGIN_NAME),
    "Audio Recorder",
    QT_TRANSLATE_NOOP("PluginBrowser", "Record audio from a microphone to WAV"),
    "Your Name <you@example.com>",
    0x0100,
    Plugin::Type::Tool,
    new PluginPixmapLoader("logo"),
    nullptr,
    nullptr
};

PLUGIN_EXPORT Plugin* lmms_plugin_main(Model*, void*) { return new AudioRecorder; }
}

static QString recordingsDir() {
    const QString music = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    QDir dir(music.isEmpty() ? QDir::homePath() : music);
    dir.mkpath("LMMS Recordings");
    return dir.filePath("LMMS Recordings");
}

QString AudioRecorder::makeDefaultOutPath() {
    return recordingsDir() + "/" +
           QString("LMMS-Record_%1.wav").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
}

AudioRecorder::AudioRecorder()
    : ToolPlugin(&audiorecorder_plugin_descriptor, nullptr) {}

AudioRecorder::~AudioRecorder() { stop(); }

void AudioRecorder::start()
{
    // already recording?
    if (m_recording.exchange(true))
        return;

    // join any old thread
    if (m_worker.joinable())
        m_worker.join();

#ifdef __linux__
    // Build save location: ~/Music/LMMS Recordings/LMMS-Record_YYYYmmdd_HHmmss.wav
    const QString music = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    QDir dir(music.isEmpty() ? QDir::homePath() : music);
    dir.mkpath("LMMS Recordings");
    m_lastPath = dir.filePath(QString("LMMS Recordings/LMMS-Record_%1.wav")
                 .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")));

    m_worker = std::thread([this]() {
        const unsigned sampleRate = 44100;
        const int channels = 1;
        const snd_pcm_format_t fmt = SND_PCM_FORMAT_S16_LE;

        snd_pcm_t* pcm = nullptr;
        if (snd_pcm_open(&pcm, "default", SND_PCM_STREAM_CAPTURE, 0) < 0) {
            m_recording.store(false);
            return;
        }
        if (snd_pcm_set_params(pcm, fmt, SND_PCM_ACCESS_RW_INTERLEAVED,
                               channels, sampleRate, 1, 500000) < 0) {
            snd_pcm_close(pcm);
            m_recording.store(false);
            return;
        }

        SF_INFO sfinfo{};
        sfinfo.channels   = channels;
        sfinfo.samplerate = sampleRate;
        sfinfo.format     = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

        SNDFILE* sf = sf_open(m_lastPath.toUtf8().constData(), SFM_WRITE, &sfinfo);
        if (!sf) {
            snd_pcm_close(pcm);
            m_recording.store(false);
            return;
        }

        const size_t frames = 1024;
        std::vector<int16_t> buf(frames * channels);

        while (m_recording.load()) {
            snd_pcm_sframes_t got = snd_pcm_readi(pcm, buf.data(), frames);
            if (got < 0) { snd_pcm_prepare(pcm); continue; }
            if (got > 0)  sf_write_short(sf, buf.data(), got * channels);
        }

        sf_write_sync(sf);
        sf_close(sf);
        snd_pcm_close(pcm);
    });
#else
    // Not implemented on this platform
    m_recording.store(false);
#endif
}

void AudioRecorder::stop()
{
    // flip flag and join thread; writer closes the WAV
    if (!m_recording.exchange(false))
        return;

    if (m_worker.joinable())
        m_worker.join();

    // optional: log where it went
    qInfo() << "AudioRecorder: saved to" << m_lastPath;
}

gui::PluginView* AudioRecorder::instantiateView(QWidget* parent) {
    return new gui::AudioRecorderView(this, parent);
}

QString AudioRecorder::nodeName() const {
    return QString::fromLatin1(audiorecorder_plugin_descriptor.name);
}
} // namespace lmms
