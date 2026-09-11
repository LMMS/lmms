#include "AudioRecorderView.h"
#include "AudioRecorder.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

using namespace lmms;
using namespace lmms::gui;

AudioRecorderView::AudioRecorderView(AudioRecorder* plugin, QWidget* parent)
    : ToolPluginView(plugin),  // ToolPluginView takes ToolPlugin* (no QWidget* in ctor)
      m_plugin(plugin)
{
    auto* v = new QVBoxLayout(this);

    auto* title = new QLabel(tr("Audio Recorder"), this);
    title->setStyleSheet("font-weight:600;");
    v->addWidget(title);

    auto* row = new QHBoxLayout();
    m_startBtn = new QPushButton(tr("Record"), this);
    m_stopBtn  = new QPushButton(tr("Stop"), this);
    m_stopBtn->setEnabled(false);
    row->addWidget(m_startBtn);
    row->addWidget(m_stopBtn);
    v->addLayout(row);

    auto* prow   = new QHBoxLayout();
    auto* plabel = new QLabel(tr("Saved to:"), this);
    m_pathVal    = new QLabel(tr("(not yet)"), this);
    m_pathVal->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto* openBtn = new QPushButton(tr("Open folder"), this);
    prow->addWidget(plabel);
    prow->addWidget(m_pathVal, 1);
    prow->addWidget(openBtn);
    v->addLayout(prow);

    // Wire up buttons
    connect(m_startBtn, &QPushButton::clicked, this, [this]{
        if (!m_plugin) return;
        m_plugin->start();
        m_pathVal->setText(m_plugin->lastPath());
        m_startBtn->setEnabled(false);
        m_stopBtn->setEnabled(true);
    });

    connect(m_stopBtn, &QPushButton::clicked, this, [this]{
    	if (!m_plugin) return;
    	m_plugin->stop();
    	m_pathVal->setText(m_plugin->lastPath());   // <- add this (optional)
   		m_startBtn->setEnabled(true);
    	m_stopBtn->setEnabled(false);
	});


    connect(openBtn, &QPushButton::clicked, this, [this]{
        if (!m_plugin) return;
        const auto dir = QFileInfo(m_plugin->lastPath()).absolutePath();
        if (!dir.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });
}
