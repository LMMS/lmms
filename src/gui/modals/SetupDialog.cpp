/*
 * SetupDialog.cpp - dialog for setting up LMMS
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QImageReader>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QScrollArea>

#include "AudioEngine.h"
#include "embed.h"
#include "Engine.h"
#include "FileDialog.h"
#include "MainWindow.h"
#include "MidiSetupWidget.h"
#include "ProjectJournal.h"
#include "SetupDialog.h"
#include "TabBar.h"
#include "TabButton.h"


// Platform-specific audio-interface classes.
#include "AudioAlsa.h"
#include "AudioAlsaSetupWidget.h"
#include "AudioDummy.h"
#include "AudioJack.h"
#include "AudioOss.h"
#include "AudioPortAudio.h"
#include "AudioPulseAudio.h"
#include "AudioSdl.h"
#include "AudioSndio.h"
#include "AudioSoundIo.h"

// Platform-specific midi-interface classes.
#include "MidiAlsaRaw.h"
#include "MidiAlsaSeq.h"
#include "MidiApple.h"
#include "MidiDummy.h"
#include "MidiJack.h"
#include "MidiOss.h"
#include "MidiSndio.h"
#include "MidiWinMM.h"


namespace lmms::gui
{


constexpr int BUFFERSIZE_RESOLUTION = 32;

inline void labelWidget(QWidget * w, const QString & txt)
{
	auto title = new QLabel(txt, w);
	QFont f = title->font();
	f.setBold(true);
	title->setFont(f);

	QBoxLayout * boxLayout = dynamic_cast<QBoxLayout *>(w->layout());
	assert(boxLayout);

	boxLayout->addWidget(title);
}




SetupDialog::SetupDialog(ConfigTab tab_to_open) :
	m_tooltips(!ConfigManager::inst()->value(
			"tooltips", "disabled").toInt()),
	m_displayWaveform(ConfigManager::inst()->value(
			"ui", "displaywaveform").toInt()),
	m_printNoteLabels(ConfigManager::inst()->value(
			"ui", "printnotelabels").toInt()),
	m_showFaderTicks(ConfigManager::inst()->value(
			"ui", "showfaderticks").toInt()),
	m_compactTrackButtons(ConfigManager::inst()->value(
			"ui", "compacttrackbuttons").toInt()),
	m_oneInstrumentTrackWindow(ConfigManager::inst()->value(
			"ui", "oneinstrumenttrackwindow").toInt()),
	m_sideBarOnRight(ConfigManager::inst()->value(
			"ui", "sidebaronright").toInt()),
	m_letPreviewsFinish(ConfigManager::inst()->value(
			"ui", "letpreviewsfinish").toInt()),
	m_soloLegacyBehavior(ConfigManager::inst()->value(
			"app", "sololegacybehavior", "0").toInt()),
	m_trackDeletionWarning(ConfigManager::inst()->value(
			"ui", "trackdeletionwarning", "1").toInt()),
	m_mixerChannelDeletionWarning(ConfigManager::inst()->value(
			"ui", "mixerchanneldeletionwarning", "1").toInt()),
	m_MMPZ(!ConfigManager::inst()->value(
			"app", "nommpz").toInt()),
	m_disableBackup(!ConfigManager::inst()->value(
			"app", "disablebackup").toInt()),
	m_openLastProject(ConfigManager::inst()->value(
			"app", "openlastproject").toInt()),
	m_loopMarkerMode{ConfigManager::inst()->value("app", "loopmarkermode", "dual")},
	m_lang(ConfigManager::inst()->value(
			"app", "language")),
	m_saveInterval(	ConfigManager::inst()->value(
			"ui", "saveinterval").toInt() < 1 ?
			MainWindow::DEFAULT_SAVE_INTERVAL_MINUTES :
			ConfigManager::inst()->value(
			"ui", "saveinterval").toInt()),
	m_enableAutoSave(ConfigManager::inst()->value(
			"ui", "enableautosave", "1").toInt()),
	m_enableRunningAutoSave(ConfigManager::inst()->value(
			"ui", "enablerunningautosave", "0").toInt()),
	m_smoothScroll(ConfigManager::inst()->value(
			"ui", "smoothscroll").toInt()),
	m_animateAFP(ConfigManager::inst()->value(
			"ui", "animateafp", "1").toInt()),
	m_vstEmbedMethod(ConfigManager::inst()->vstEmbedMethod()),
	m_vstAlwaysOnTop(ConfigManager::inst()->value(
			"ui", "vstalwaysontop").toInt()),
	m_disableAutoQuit(ConfigManager::inst()->value(
			"ui", "disableautoquit", "1").toInt()),
	m_NaNHandler(ConfigManager::inst()->value(
			"app", "nanhandler", "1").toInt()),
	m_bufferSize(ConfigManager::inst()->value(
			"audioengine", "framesperaudiobuffer").toInt()),
	m_sampleRate(ConfigManager::inst()->value(
			"audioengine", "samplerate").toInt()),
	m_midiAutoQuantize(ConfigManager::inst()->value(
			"midi", "autoquantize", "0").toInt() != 0),
	m_workingDir(QDir::toNativeSeparators(ConfigManager::inst()->workingDir())),
	m_vstDir(QDir::toNativeSeparators(ConfigManager::inst()->vstDir())),
	m_ladspaDir(QDir::toNativeSeparators(ConfigManager::inst()->ladspaDir())),
	m_gigDir(QDir::toNativeSeparators(ConfigManager::inst()->gigDir())),
	m_sf2Dir(QDir::toNativeSeparators(ConfigManager::inst()->sf2Dir())),
#ifdef LMMS_HAVE_FLUIDSYNTH
	m_sf2File(QDir::toNativeSeparators(ConfigManager::inst()->sf2File())),
#endif
	m_themeDir(QDir::toNativeSeparators(ConfigManager::inst()->themeDir())),
	m_backgroundPicFile(QDir::toNativeSeparators(ConfigManager::inst()->backgroundPicFile()))
{
	setWindowIcon(embed::getIconPixmap("setup_general"));
	setWindowTitle(tr("Settings"));
	setWindowFlag(Qt::WindowContextHelpButtonHint, false);
	setModal(true);

	Engine::projectJournal()->setJournalling(false);


	// Main widget.
	auto main_w = new QWidget(this);

	// Vertical layout.
	auto vlayout = new QVBoxLayout(this);
	vlayout->setSpacing(0);
	vlayout->setContentsMargins(0, 0, 0, 0);

	// Horizontal layout.
	auto hlayout = new QHBoxLayout(main_w);
	hlayout->setSpacing(0);
	hlayout->setContentsMargins(0, 0, 0, 0);

	// Tab bar for the main tabs.
	m_tabBar = new TabBar(main_w, QBoxLayout::TopToBottom);
	m_tabBar->setExclusive(true);
	m_tabBar->setFixedWidth(72);

	// Settings widget.
	auto settings_w = new QWidget(main_w);

	QVBoxLayout * settingsLayout = new QVBoxLayout(settings_w);

	// General widget.
	auto general_w = new QWidget(settings_w);
	auto general_layout = new QVBoxLayout(general_w);
	general_layout->setSpacing(10);
	general_layout->setContentsMargins(0, 0, 0, 0);
	labelWidget(general_w, tr("General"));

	// General scroll area.
	auto generalScroll = new QScrollArea(general_w);
	generalScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	generalScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// General controls widget.
	auto generalControls = new QWidget(general_w);

	// Path selectors layout.
	auto generalControlsLayout = new QVBoxLayout;
	generalControlsLayout->setSpacing(10);
	generalControlsLayout->setContentsMargins(0, 0, 0, 0);

	auto addCheckBox = [&](const QString& ledText, QWidget* parent, QBoxLayout * layout,
									  bool initialState, const char* toggledSlot, bool showRestartWarning) -> QCheckBox * {
		auto checkBox = new QCheckBox(ledText, parent);
		checkBox->setChecked(initialState);
		connect(checkBox, SIGNAL(toggled(bool)), this, toggledSlot);

		if (showRestartWarning)
		{
			connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(showRestartWarning()));
		}

		if (layout)
		{
			layout->addWidget(checkBox);
		}

		return checkBox;
	};

	// GUI tab.
	QGroupBox * guiGroupBox = new QGroupBox(tr("Graphical user interface (GUI)"), generalControls);
	QVBoxLayout * guiGroupLayout = new QVBoxLayout(guiGroupBox);

	addCheckBox(tr("Enable tooltips"), guiGroupBox, guiGroupLayout,
		m_tooltips, SLOT(toggleTooltips(bool)), true);
	addCheckBox(tr("Enable master oscilloscope by default"), guiGroupBox, guiGroupLayout,
		m_displayWaveform, SLOT(toggleDisplayWaveform(bool)), true);
	addCheckBox(tr("Enable all note labels in piano roll"), guiGroupBox, guiGroupLayout,
		m_printNoteLabels, SLOT(toggleNoteLabels(bool)), false);
	addCheckBox(tr("Show fader ticks"), guiGroupBox, guiGroupLayout,
		m_showFaderTicks, SLOT(toggleShowFaderTicks(bool)), false);
	addCheckBox(tr("Enable compact track buttons"), guiGroupBox, guiGroupLayout,
		m_compactTrackButtons, SLOT(toggleCompactTrackButtons(bool)), true);
	addCheckBox(tr("Enable one instrument-track-window mode"), guiGroupBox, guiGroupLayout,
		m_oneInstrumentTrackWindow, SLOT(toggleOneInstrumentTrackWindow(bool)), true);
	addCheckBox(tr("Show sidebar on the right-hand side"), guiGroupBox, guiGroupLayout,
		m_sideBarOnRight, SLOT(toggleSideBarOnRight(bool)), true);
	addCheckBox(tr("Let sample previews continue when mouse is released"), guiGroupBox, guiGroupLayout,
		m_letPreviewsFinish, SLOT(toggleLetPreviewsFinish(bool)), false);
	addCheckBox(tr("Mute automation tracks during solo"), guiGroupBox, guiGroupLayout,
		m_soloLegacyBehavior, SLOT(toggleSoloLegacyBehavior(bool)), false);
	addCheckBox(tr("Show warning when deleting tracks"), guiGroupBox, guiGroupLayout,
		m_trackDeletionWarning, SLOT(toggleTrackDeletionWarning(bool)), false);
	addCheckBox(tr("Show warning when deleting a mixer channel that is in use"), guiGroupBox, guiGroupLayout,
		m_mixerChannelDeletionWarning,	SLOT(toggleMixerChannelDeletionWarning(bool)), false);

	m_loopMarkerComboBox = new QComboBox{guiGroupBox};

	m_loopMarkerComboBox->addItem(tr("Dual-button"), "dual");
	m_loopMarkerComboBox->addItem(tr("Grab closest"), "closest");
	m_loopMarkerComboBox->addItem(tr("Handles"), "handles");

	m_loopMarkerComboBox->setCurrentIndex(m_loopMarkerComboBox->findData(m_loopMarkerMode));
	connect(m_loopMarkerComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
		this, &SetupDialog::loopMarkerModeChanged);

	guiGroupLayout->addWidget(new QLabel{tr("Loop edit mode"), guiGroupBox});
	guiGroupLayout->addWidget(m_loopMarkerComboBox);

	generalControlsLayout->addWidget(guiGroupBox);

	generalControlsLayout->addSpacing(10);

	// Projects tab.
	QGroupBox * projectsGroupBox = new QGroupBox(tr("Projects"), generalControls);
	QVBoxLayout * projectsGroupLayout = new QVBoxLayout(projectsGroupBox);

	addCheckBox(tr("Compress project files by default"), projectsGroupBox, projectsGroupLayout,
		m_MMPZ, SLOT(toggleMMPZ(bool)), true);
	addCheckBox(tr("Create a backup file when saving a project"), projectsGroupBox, projectsGroupLayout,
		m_disableBackup, SLOT(toggleDisableBackup(bool)), false);
	addCheckBox(tr("Reopen last project on startup"), projectsGroupBox, projectsGroupLayout,
		m_openLastProject, SLOT(toggleOpenLastProject(bool)), false);

	generalControlsLayout->addWidget(projectsGroupBox);

	generalControlsLayout->addSpacing(10);

	// Language tab.
	QGroupBox * languageGroupBox = new QGroupBox(tr("Language"), generalControls);
	QVBoxLayout * languageGroupLayout = new QVBoxLayout(languageGroupBox);

	auto changeLang = new QComboBox(languageGroupBox);
	languageGroupLayout->addWidget(changeLang);

	QDir dir(ConfigManager::inst()->localeDir());
	QStringList fileNames = dir.entryList(QStringList("*.qm"));
	for(int i = 0; i < fileNames.size(); ++i)
	{
		// Extract ISO-639 language code from filename
		fileNames[i].truncate(fileNames[i].lastIndexOf('.'));
		// Skip invalid language codes
		QLocale locale(fileNames[i]);
		if (locale.language() == QLocale::C)
		{
			continue;
		}
		// Display the native language name or fallback to the English name
		QString langName = locale.nativeLanguageName();
		if (langName.isEmpty())
		{
			langName = QLocale::languageToString(locale.language());
		}
		m_languages.append(fileNames[i]);
		changeLang->addItem(langName);
	}

	// If language unset, fallback to system language when available.
	if(m_lang == "")
	{
		QString tmp = QLocale::system().name().left(2);
		if(m_languages.contains(tmp))
		{
			m_lang = tmp;
		}
		else
		{
			m_lang = "en";
		}
	}

	for(int i = 0; i < changeLang->count(); ++i)
	{
		if(m_lang == m_languages.at(i))
		{
			changeLang->setCurrentIndex(i);
			break;
		}
	}

	connect(changeLang, SIGNAL(currentIndexChanged(int)),
			this, SLOT(setLanguage(int)));
	connect(changeLang, SIGNAL(currentIndexChanged(int)),
			this, SLOT(showRestartWarning()));

	generalControlsLayout->addWidget(languageGroupBox);
	generalControlsLayout->addSpacing(10);

	// General layout ordering.
	generalControlsLayout->addStretch();
	generalControls->setLayout(generalControlsLayout);
	generalScroll->setWidget(generalControls);
	generalScroll->setWidgetResizable(true);
	general_layout->addWidget(generalScroll, 1);



	// Performance widget.
	auto performance_w = new QWidget(settings_w);
	auto performance_layout = new QVBoxLayout(performance_w);
	performance_layout->setSpacing(10);
	performance_layout->setContentsMargins(0, 0, 0, 0);
	labelWidget(performance_w,
			tr("Performance"));


	// Autosave tab.
	QGroupBox * autoSaveBox = new QGroupBox(tr("Autosave"), performance_w);
	QVBoxLayout * autoSaveLayout = new QVBoxLayout(autoSaveBox);
	QHBoxLayout * autoSaveSubLayout = new QHBoxLayout();

	m_saveIntervalSlider = new QSlider(Qt::Horizontal, autoSaveBox);
	m_saveIntervalSlider->setValue(m_saveInterval);
	m_saveIntervalSlider->setRange(1, 20);
	m_saveIntervalSlider->setTickInterval(1);
	m_saveIntervalSlider->setPageStep(1);
	m_saveIntervalSlider->setTickPosition(QSlider::TicksBelow);

	connect(m_saveIntervalSlider, SIGNAL(valueChanged(int)),
			this, SLOT(setAutoSaveInterval(int)));

	auto autoSaveResetBtn = new QPushButton(embed::getIconPixmap("reload"), "", autoSaveBox);
	autoSaveResetBtn->setFixedSize(32, 32);
	connect(autoSaveResetBtn, SIGNAL(clicked()),
		this, SLOT(resetAutoSave()));

	autoSaveSubLayout->addWidget(m_saveIntervalSlider);
	autoSaveSubLayout->addWidget(autoSaveResetBtn);

	autoSaveLayout->addLayout(autoSaveSubLayout);

	m_saveIntervalLbl = new QLabel(autoSaveBox);
	setAutoSaveInterval(m_saveIntervalSlider->value());
	autoSaveLayout->addWidget(m_saveIntervalLbl);

	m_autoSave = addCheckBox(tr("Enable autosave"), autoSaveBox, autoSaveLayout,
		m_enableAutoSave, SLOT(toggleAutoSave(bool)), false);

	m_runningAutoSave = addCheckBox(tr("Allow autosave while playing"), autoSaveBox, autoSaveLayout,
		m_enableRunningAutoSave, SLOT(toggleRunningAutoSave(bool)), false);

	m_saveIntervalSlider->setEnabled(m_enableAutoSave);
	m_runningAutoSave->setVisible(m_enableAutoSave);


	// UI effect vs. performance tab.
	QGroupBox * uiFxBox = new QGroupBox(tr("User interface (UI) effects vs. performance"), performance_w);
	QVBoxLayout * uiFxLayout = new QVBoxLayout(uiFxBox);

	addCheckBox(tr("Smooth scroll in song editor"), uiFxBox, uiFxLayout,
		m_smoothScroll, SLOT(toggleSmoothScroll(bool)), false);
	addCheckBox(tr("Display playback cursor in AudioFileProcessor"), uiFxBox, uiFxLayout,
		m_animateAFP, SLOT(toggleAnimateAFP(bool)), false);


	// Plugins group
	QGroupBox * pluginsBox = new QGroupBox(tr("Plugins"), performance_w);
	QVBoxLayout * pluginsLayout = new QVBoxLayout(pluginsBox);

	m_vstEmbedLbl = new QLabel(pluginsBox);
	m_vstEmbedLbl->setText(tr("VST plugins embedding:"));
	pluginsLayout->addWidget(m_vstEmbedLbl);

	m_vstEmbedComboBox = new QComboBox(pluginsBox);

	QStringList embedMethods = ConfigManager::availableVstEmbedMethods();
	m_vstEmbedComboBox->addItem(tr("No embedding"), "none");
	if(embedMethods.contains("qt"))
	{
		m_vstEmbedComboBox->addItem(tr("Embed using Qt API"), "qt");
	}
	if(embedMethods.contains("win32"))
	{
		m_vstEmbedComboBox->addItem(tr("Embed using native Win32 API"), "win32");
	}
	if(embedMethods.contains("xembed"))
	{
		m_vstEmbedComboBox->addItem(tr("Embed using XEmbed protocol"), "xembed");
	}
	m_vstEmbedComboBox->setCurrentIndex(m_vstEmbedComboBox->findData(m_vstEmbedMethod));
	connect(m_vstEmbedComboBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(vstEmbedMethodChanged()));
	pluginsLayout->addWidget(m_vstEmbedComboBox);

	m_vstAlwaysOnTopCheckBox = addCheckBox(tr("Keep plugin windows on top when not embedded"), pluginsBox, pluginsLayout,
		m_vstAlwaysOnTop, SLOT(toggleVSTAlwaysOnTop(bool)), false);

	addCheckBox(tr("Keep effects running even without input"), pluginsBox, pluginsLayout,
		m_disableAutoQuit, SLOT(toggleDisableAutoQuit(bool)), false);


	// Performance layout ordering.
	performance_layout->addWidget(autoSaveBox);
	performance_layout->addWidget(uiFxBox);
	performance_layout->addWidget(pluginsBox);
	performance_layout->addStretch();



	// Audio widget.
	auto audio_w = new QWidget(settings_w);
	auto audio_layout = new QVBoxLayout(audio_w);
	audio_layout->setSpacing(10);
	audio_layout->setContentsMargins(0, 0, 0, 0);
	labelWidget(audio_w,
			tr("Audio"));

	// Audio interface group
	QGroupBox * audioInterfaceBox = new QGroupBox(tr("Audio interface"), audio_w);
	QVBoxLayout * audioInterfaceLayout = new QVBoxLayout(audioInterfaceBox);

	m_audioInterfaces = new QComboBox(audioInterfaceBox);
	audioInterfaceLayout->addWidget(m_audioInterfaces);

	// Ifaces-settings-widget.
	auto as_w = new QWidget(audio_w);

	auto as_w_layout = new QHBoxLayout(as_w);
	as_w_layout->setSpacing(0);
	as_w_layout->setContentsMargins(0, 0, 0, 0);

#ifdef LMMS_HAVE_JACK
	m_audioIfaceSetupWidgets[AudioJack::name()] =
			new AudioJack::setupWidget(as_w);
#endif

#ifdef LMMS_HAVE_ALSA
	m_audioIfaceSetupWidgets[AudioAlsa::name()] =
			new AudioAlsaSetupWidget(as_w);
#endif

#ifdef LMMS_HAVE_PULSEAUDIO
	m_audioIfaceSetupWidgets[AudioPulseAudio::name()] =
			new AudioPulseAudio::setupWidget(as_w);
#endif

#ifdef LMMS_HAVE_PORTAUDIO
	m_audioIfaceSetupWidgets[AudioPortAudio::name()] =
			new AudioPortAudio::setupWidget(as_w);
#endif

#ifdef LMMS_HAVE_SOUNDIO
	m_audioIfaceSetupWidgets[AudioSoundIo::name()] =
			new AudioSoundIo::setupWidget(as_w);
#endif

#ifdef LMMS_HAVE_SDL
	m_audioIfaceSetupWidgets[AudioSdl::name()] =
			new AudioSdl::setupWidget(as_w);
#endif

#ifdef LMMS_HAVE_OSS
	m_audioIfaceSetupWidgets[AudioOss::name()] =
			new AudioOss::setupWidget(as_w);
#endif

#ifdef LMMS_HAVE_SNDIO
	m_audioIfaceSetupWidgets[AudioSndio::name()] =
			new AudioSndio::setupWidget(as_w);
#endif

	m_audioIfaceSetupWidgets[AudioDummy::name()] =
			new AudioDummy::setupWidget(as_w);


	for(AswMap::iterator it = m_audioIfaceSetupWidgets.begin();
		it != m_audioIfaceSetupWidgets.end(); ++it)
	{
		m_audioIfaceNames[
			AudioDeviceSetupWidget::tr(it.key().toUtf8())] = it.key();
	}
	for(trMap::iterator it = m_audioIfaceNames.begin();
		it != m_audioIfaceNames.end(); ++it)
	{
		QWidget * audioWidget = m_audioIfaceSetupWidgets[it.value()];
		audioWidget->hide();
		as_w_layout->addWidget(audioWidget);
		m_audioInterfaces->addItem(it.key());
	}

	// If no preferred audio device is saved, save the current one.
	QString audioDevName = ConfigManager::inst()->value("audioengine", "audiodev");
	if (m_audioInterfaces->findText(audioDevName) < 0)
	{
		audioDevName = Engine::audioEngine()->audioDevName();
		ConfigManager::inst()->setValue("audioengine", "audiodev", audioDevName);
	}
	m_audioInterfaces->
		setCurrentIndex(m_audioInterfaces->findText(audioDevName));
	m_audioIfaceSetupWidgets[audioDevName]->show();

	connect(m_audioInterfaces, SIGNAL(activated(const QString&)),
			this, SLOT(audioInterfaceChanged(const QString&)));

	// Advanced setting, hidden for now
	// // TODO Handle or remove.
	// auto useNaNHandler = new LedCheckBox(tr("Use built-in NaN handler"), audio_w);
	// audio_layout->addWidget(useNaNHandler);
	// useNaNHandler->setChecked(m_NaNHandler);

	auto sampleRateBox = new QGroupBox{tr("Sample rate"), audio_w};

	m_sampleRateSlider = new QSlider{Qt::Horizontal};
	m_sampleRateSlider->setRange(0, SUPPORTED_SAMPLERATES.size() - 1);
	m_sampleRateSlider->setTickPosition(QSlider::TicksBelow);

	auto sampleRateResetButton = new QPushButton{embed::getIconPixmap("reload"), ""};
	sampleRateResetButton->setFixedSize(32, 32);

	auto sampleRateSubLayout = new QHBoxLayout{};
	sampleRateSubLayout->addWidget(m_sampleRateSlider);
	sampleRateSubLayout->addWidget(sampleRateResetButton);

	auto sampleRateLabel = new QLabel{sampleRateBox};
	auto sampleRateLayout = new QVBoxLayout{sampleRateBox};
	sampleRateLayout->addLayout(sampleRateSubLayout);
	sampleRateLayout->addWidget(sampleRateLabel);

	auto setSampleRate = [this, sampleRateLabel](int sampleRate)
	{	
		const auto it = std::find(SUPPORTED_SAMPLERATES.begin(), SUPPORTED_SAMPLERATES.end(), sampleRate);
		const auto index = it == SUPPORTED_SAMPLERATES.end() ? 0 : std::distance(SUPPORTED_SAMPLERATES.begin(), it);

		m_sampleRate = SUPPORTED_SAMPLERATES[index];
		m_sampleRateSlider->setValue(index);
		sampleRateLabel->setText(tr("Sample rate: %1").arg(m_sampleRate));
	};

	setSampleRate(m_sampleRate);

	connect(m_sampleRateSlider, &QSlider::valueChanged, this, &SetupDialog::showRestartWarning);

	connect(m_sampleRateSlider, &QSlider::valueChanged, this,
		[setSampleRate](int value) { setSampleRate(SUPPORTED_SAMPLERATES[value]); });

	connect(sampleRateResetButton, &QPushButton::clicked, this,
		[setSampleRate] { setSampleRate(SUPPORTED_SAMPLERATES.front()); });

	// Buffer size group
	QGroupBox * bufferSizeBox = new QGroupBox(tr("Buffer size"), audio_w);
	QVBoxLayout * bufferSizeLayout = new QVBoxLayout(bufferSizeBox);
	QHBoxLayout * bufferSizeSubLayout = new QHBoxLayout();

	m_bufferSizeSlider = new QSlider(Qt::Horizontal, bufferSizeBox);
	m_bufferSizeSlider->setRange(1, MAXIMUM_BUFFER_SIZE / BUFFERSIZE_RESOLUTION);
	m_bufferSizeSlider->setTickInterval(8);
	m_bufferSizeSlider->setPageStep(8);
	m_bufferSizeSlider->setValue(m_bufferSize / BUFFERSIZE_RESOLUTION);
	m_bufferSizeSlider->setTickPosition(QSlider::TicksBelow);

	connect(m_bufferSizeSlider, SIGNAL(valueChanged(int)),
			this, SLOT(setBufferSize(int)));
	connect(m_bufferSizeSlider, SIGNAL(valueChanged(int)),
			this, SLOT(showRestartWarning()));
	bufferSizeSubLayout->addWidget(m_bufferSizeSlider, 1);

	auto bufferSize_reset_btn = new QPushButton(embed::getIconPixmap("reload"), "", bufferSizeBox);
	bufferSize_reset_btn->setFixedSize(32, 32);
	connect(bufferSize_reset_btn, SIGNAL(clicked()),
		this, SLOT(resetBufferSize()));
	bufferSize_reset_btn->setToolTip(
		tr("Reset to default value"));

	bufferSizeSubLayout->addWidget(bufferSize_reset_btn);
	bufferSizeLayout->addLayout(bufferSizeSubLayout);

	m_bufferSizeLbl = new QLabel(bufferSizeBox);
	bufferSizeLayout->addWidget(m_bufferSizeLbl);

	m_bufferSizeWarnLbl = new QLabel(bufferSizeBox);
	m_bufferSizeWarnLbl->setWordWrap(true);
	bufferSizeLayout->addWidget(m_bufferSizeWarnLbl);

	setBufferSize(m_bufferSizeSlider->value());


	// Audio layout ordering.
	audio_layout->addWidget(audioInterfaceBox);
	audio_layout->addWidget(as_w);
	audio_layout->addWidget(sampleRateBox);
	audio_layout->addWidget(bufferSizeBox);
	audio_layout->addStretch();



	// MIDI widget.
	auto midi_w = new QWidget(settings_w);
	auto midi_layout = new QVBoxLayout(midi_w);
	midi_layout->setSpacing(10);
	midi_layout->setContentsMargins(0, 0, 0, 0);
	labelWidget(midi_w, tr("MIDI"));

	// MIDI interface group
	QGroupBox * midiInterfaceBox = new QGroupBox(tr("MIDI interface"), midi_w);
	QVBoxLayout * midiInterfaceLayout = new QVBoxLayout(midiInterfaceBox);

	m_midiInterfaces = new QComboBox(midiInterfaceBox);
	midiInterfaceLayout->addWidget(m_midiInterfaces);

	// Ifaces-settings-widget.
	auto ms_w = new QWidget(midi_w);

	auto ms_w_layout = new QHBoxLayout(ms_w);
	ms_w_layout->setSpacing(0);
	ms_w_layout->setContentsMargins(0, 0, 0, 0);

#ifdef LMMS_HAVE_ALSA
	m_midiIfaceSetupWidgets[MidiAlsaSeq::name()] =
			MidiSetupWidget::create<MidiAlsaSeq>(ms_w);
	m_midiIfaceSetupWidgets[MidiAlsaRaw::name()] =
			MidiSetupWidget::create<MidiAlsaRaw>(ms_w);
#endif

#ifdef LMMS_HAVE_JACK
	m_midiIfaceSetupWidgets[MidiJack::name()] =
			MidiSetupWidget::create<MidiJack>(ms_w);
#endif

#ifdef LMMS_HAVE_OSS
	m_midiIfaceSetupWidgets[MidiOss::name()] =
			MidiSetupWidget::create<MidiOss>(ms_w);
#endif

#ifdef LMMS_HAVE_SNDIO
	m_midiIfaceSetupWidgets[MidiSndio::name()] =
			MidiSetupWidget::create<MidiSndio>(ms_w);
#endif

#ifdef LMMS_BUILD_WIN32
	m_midiIfaceSetupWidgets[MidiWinMM::name()] =
			MidiSetupWidget::create<MidiWinMM>(ms_w);
#endif

#ifdef LMMS_BUILD_APPLE
    m_midiIfaceSetupWidgets[MidiApple::name()] =
			MidiSetupWidget::create<MidiApple>(ms_w);
#endif

	m_midiIfaceSetupWidgets[MidiDummy::name()] =
			MidiSetupWidget::create<MidiDummy>(ms_w);


	for(MswMap::iterator it = m_midiIfaceSetupWidgets.begin();
		it != m_midiIfaceSetupWidgets.end(); ++it)
	{
		m_midiIfaceNames[
			MidiSetupWidget::tr(it.key().toUtf8())] = it.key();
	}
	for(trMap::iterator it = m_midiIfaceNames.begin();
		it != m_midiIfaceNames.end(); ++it)
	{
		QWidget * midiWidget = m_midiIfaceSetupWidgets[it.value()];
		midiWidget->hide();
		ms_w_layout->addWidget(midiWidget);
		m_midiInterfaces->addItem(it.key());
	}

	QString midiDevName = ConfigManager::inst()->value("audioengine", "mididev");
	if (m_midiInterfaces->findText(midiDevName) < 0)
	{
		midiDevName = Engine::audioEngine()->midiClientName();
		ConfigManager::inst()->setValue("audioengine", "mididev", midiDevName);
	}
	m_midiInterfaces->setCurrentIndex(m_midiInterfaces->findText(midiDevName));
	m_midiIfaceSetupWidgets[midiDevName]->show();

	connect(m_midiInterfaces, SIGNAL(activated(const QString&)),
			this, SLOT(midiInterfaceChanged(const QString&)));


	// MIDI autoassign group
	QGroupBox * midiAutoAssignBox = new QGroupBox(tr("Automatically assign MIDI controller to selected track"), midi_w);
	QVBoxLayout * midiAutoAssignLayout = new QVBoxLayout(midiAutoAssignBox);

	m_assignableMidiDevices = new QComboBox(midiAutoAssignBox);
	midiAutoAssignLayout->addWidget(m_assignableMidiDevices);
	m_assignableMidiDevices->addItem("none");
	if ( !Engine::audioEngine()->midiClient()->isRaw() )
	{
		m_assignableMidiDevices->addItems(Engine::audioEngine()->midiClient()->readablePorts());
	}
	else
	{
		m_assignableMidiDevices->addItem("all");
	}
	int current = m_assignableMidiDevices->findText(ConfigManager::inst()->value("midi", "midiautoassign"));
	if (current >= 0)
	{
		m_assignableMidiDevices->setCurrentIndex(current);
	}

	// MIDI Recording tab
	auto* midiRecordingTab = new QGroupBox(tr("Behavior when recording"), midi_w);
	auto* midiRecordingLayout = new QVBoxLayout(midiRecordingTab);
	{
		auto *box = addCheckBox(tr("Auto-quantize notes in Piano Roll"),
								midiRecordingTab, midiRecordingLayout,
								m_midiAutoQuantize, SLOT(toggleMidiAutoQuantization(bool)),
								false);
		box->setToolTip(tr("If enabled, notes will be automatically quantized when recording them from a MIDI controller. If disabled, they are always recorded at the highest possible resolution."));
	}

	// MIDI layout ordering.
	midi_layout->addWidget(midiInterfaceBox);
	midi_layout->addWidget(ms_w);
	midi_layout->addWidget(midiAutoAssignBox);
	midi_layout->addWidget(midiRecordingTab);
	midi_layout->addStretch();



	// Paths widget.
	auto paths_w = new QWidget(settings_w);

	auto paths_layout = new QVBoxLayout(paths_w);
	paths_layout->setSpacing(10);
	paths_layout->setContentsMargins(0, 0, 0, 0);

	labelWidget(paths_w, tr("Paths"));


	// Paths scroll area.
	auto pathsScroll = new QScrollArea(paths_w);
	pathsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	pathsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// Path selectors widget.
	auto pathSelectors = new QWidget(paths_w);

	// Path selectors layout.
	auto pathSelectorsLayout = new QVBoxLayout;
	pathSelectorsLayout->setSpacing(10);
	pathSelectorsLayout->setContentsMargins(0, 0, 0, 0);

	auto addPathEntry = [&](const QString& caption, const QString& content, const char* setSlot, const char* openSlot,
							QLineEdit*& lineEdit, const char* pixmap = "project_open") {
		auto pathEntryGroupBox = new QGroupBox(caption, pathSelectors);
		QHBoxLayout * pathEntryLayout = new QHBoxLayout(pathEntryGroupBox);

		lineEdit = new QLineEdit(content, pathEntryGroupBox);
		connect(lineEdit, SIGNAL(textChanged(const QString&)),
			this, setSlot);

		pathEntryLayout->addWidget(lineEdit, 1);

		auto selectBtn = new QPushButton(embed::getIconPixmap(pixmap, 16, 16), "", pathEntryGroupBox);
		selectBtn->setFixedSize(24, 24);
		connect(selectBtn, SIGNAL(clicked()), this, openSlot);

		pathEntryLayout->addWidget(selectBtn, 0);

		pathSelectorsLayout->addWidget(pathEntryGroupBox);
		pathSelectorsLayout->addSpacing(10);
	};

	addPathEntry(tr("LMMS working directory"), m_workingDir,
		SLOT(setWorkingDir(const QString&)),
		SLOT(openWorkingDir()),
		m_workingDirLineEdit);
	addPathEntry(tr("VST plugins directory"), m_vstDir,
		SLOT(setVSTDir(const QString&)),
		SLOT(openVSTDir()),
		m_vstDirLineEdit);
	addPathEntry(tr("LADSPA plugins directories"), m_ladspaDir,
		SLOT(setLADSPADir(const QString&)),
		SLOT(openLADSPADir()),
		m_ladspaDirLineEdit, "add_folder");
	addPathEntry(tr("SF2 directory"), m_sf2Dir,
		SLOT(setSF2Dir(const QString&)),
		SLOT(openSF2Dir()),
		m_sf2DirLineEdit);
#ifdef LMMS_HAVE_FLUIDSYNTH
	addPathEntry(tr("Default SF2"), m_sf2File,
		SLOT(setSF2File(const QString&)),
		SLOT(openSF2File()),
		m_sf2FileLineEdit);
#endif
	addPathEntry(tr("GIG directory"), m_gigDir,
		SLOT(setGIGDir(const QString&)),
		SLOT(openGIGDir()),
		m_gigDirLineEdit);
	addPathEntry(tr("Theme directory"), m_themeDir,
		SLOT(setThemeDir(const QString&)),
		SLOT(openThemeDir()),
		m_themeDirLineEdit);
	addPathEntry(tr("Background artwork"), m_backgroundPicFile,
		SLOT(setBackgroundPicFile(const QString&)),
		SLOT(openBackgroundPicFile()),
		m_backgroundPicFileLineEdit);

	pathSelectorsLayout->addStretch();

	pathSelectors->setLayout(pathSelectorsLayout);

	pathsScroll->setWidget(pathSelectors);
	pathsScroll->setWidgetResizable(true);

	paths_layout->addWidget(pathsScroll, 1);
	paths_layout->addStretch();

	// Add all main widgets to the layout of the settings widget
	// This is needed so that we automatically get the correct sizes.
	settingsLayout->addWidget(general_w);
	settingsLayout->addWidget(performance_w);
	settingsLayout->addWidget(audio_w);
	settingsLayout->addWidget(midi_w);
	settingsLayout->addWidget(paths_w);

	// Major tabs ordering.
	m_tabBar->addTab(general_w,
			tr("General"), 0, false, true, false)->setIcon(
					embed::getIconPixmap("setup_general"));
	m_tabBar->addTab(performance_w,
			tr("Performance"), 1, false, true, false)->setIcon(
					embed::getIconPixmap("setup_performance"));
	m_tabBar->addTab(audio_w,
			tr("Audio"), 2, false, true, false)->setIcon(
					embed::getIconPixmap("setup_audio"));
	m_tabBar->addTab(midi_w,
			tr("MIDI"), 3, false, true, false)->setIcon(
					embed::getIconPixmap("setup_midi"));
	m_tabBar->addTab(paths_w,
			tr("Paths"), 4, true, true, false)->setIcon(
					embed::getIconPixmap("setup_directories"));

	m_tabBar->setActiveTab(static_cast<int>(tab_to_open));

	// Horizontal layout ordering.
	hlayout->addSpacing(2);
	hlayout->addWidget(m_tabBar);
	hlayout->addSpacing(10);
	hlayout->addWidget(settings_w);
	hlayout->addSpacing(10);

	// Extras widget and layout.
	auto extras_w = new QWidget(this);
	auto extras_layout = new QHBoxLayout(extras_w);
	extras_layout->setSpacing(0);
	extras_layout->setContentsMargins(0, 0, 0, 0);

	// Restart warning label.
	restartWarningLbl = new QLabel(
			tr("Some changes require restarting."), extras_w);
	restartWarningLbl->hide();

	// OK button.
	auto ok_btn = new QPushButton(embed::getIconPixmap("apply"), tr("OK"), extras_w);
	connect(ok_btn, SIGNAL(clicked()),
			this, SLOT(accept()));

	// Cancel button.
	auto cancel_btn = new QPushButton(embed::getIconPixmap("cancel"), tr("Cancel"), extras_w);
	connect(cancel_btn, SIGNAL(clicked()),
			this, SLOT(reject()));

	// Extras layout ordering.
	extras_layout->addSpacing(10);
	extras_layout->addWidget(restartWarningLbl);
	extras_layout->addStretch();
	extras_layout->addWidget(ok_btn);
	extras_layout->addSpacing(10);
	extras_layout->addWidget(cancel_btn);
	extras_layout->addSpacing(10);

	// Vertical layout ordering.
	vlayout->addWidget(main_w, 1);
	vlayout->addSpacing(10);
	vlayout->addWidget(extras_w);
	vlayout->addSpacing(10);

	// Ensure that we cannot make the dialog smaller than it wants to be
	setMinimumWidth(width());

	show();
}




SetupDialog::~SetupDialog()
{
	Engine::projectJournal()->setJournalling(true);
}




void SetupDialog::accept()
{
	/* Hide dialog before setting values. This prevents an obscure bug
	where non-embedded VST windows would steal focus and prevent LMMS
	from taking mouse input, rendering the application unusable. */
	QDialog::accept();

	ConfigManager::inst()->setValue("tooltips", "disabled",
					QString::number(!m_tooltips));
	ConfigManager::inst()->setValue("ui", "displaywaveform",
					QString::number(m_displayWaveform));
	ConfigManager::inst()->setValue("ui", "printnotelabels",
					QString::number(m_printNoteLabels));
	ConfigManager::inst()->setValue("ui", "showfaderticks",
					QString::number(m_showFaderTicks));
	ConfigManager::inst()->setValue("ui", "compacttrackbuttons",
					QString::number(m_compactTrackButtons));
	ConfigManager::inst()->setValue("ui", "oneinstrumenttrackwindow",
					QString::number(m_oneInstrumentTrackWindow));
	ConfigManager::inst()->setValue("ui", "sidebaronright",
					QString::number(m_sideBarOnRight));
	ConfigManager::inst()->setValue("ui", "letpreviewsfinish",
					QString::number(m_letPreviewsFinish));
	ConfigManager::inst()->setValue("app", "sololegacybehavior",
					QString::number(m_soloLegacyBehavior));
	ConfigManager::inst()->setValue("ui", "trackdeletionwarning",
					QString::number(m_trackDeletionWarning));
	ConfigManager::inst()->setValue("ui", "mixerchanneldeletionwarning",
					QString::number(m_mixerChannelDeletionWarning));
	ConfigManager::inst()->setValue("app", "nommpz",
					QString::number(!m_MMPZ));
	ConfigManager::inst()->setValue("app", "disablebackup",
					QString::number(!m_disableBackup));
	ConfigManager::inst()->setValue("app", "openlastproject",
					QString::number(m_openLastProject));
	ConfigManager::inst()->setValue("app", "loopmarkermode", m_loopMarkerMode);
	ConfigManager::inst()->setValue("app", "language", m_lang);
	ConfigManager::inst()->setValue("ui", "saveinterval",
					QString::number(m_saveInterval));
	ConfigManager::inst()->setValue("ui", "enableautosave",
					QString::number(m_enableAutoSave));
	ConfigManager::inst()->setValue("ui", "enablerunningautosave",
					QString::number(m_enableRunningAutoSave));
	ConfigManager::inst()->setValue("ui", "smoothscroll",
					QString::number(m_smoothScroll));
	ConfigManager::inst()->setValue("ui", "animateafp",
					QString::number(m_animateAFP));
	ConfigManager::inst()->setValue("ui", "vstembedmethod",
					m_vstEmbedComboBox->currentData().toString());
	ConfigManager::inst()->setValue("ui", "vstalwaysontop",
					QString::number(m_vstAlwaysOnTop));
	ConfigManager::inst()->setValue("ui", "disableautoquit",
					QString::number(m_disableAutoQuit));
	ConfigManager::inst()->setValue("audioengine", "audiodev",
					m_audioIfaceNames[m_audioInterfaces->currentText()]);
	ConfigManager::inst()->setValue("app", "nanhandler",
					QString::number(m_NaNHandler));
	ConfigManager::inst()->setValue("audioengine", "samplerate",
					QString::number(m_sampleRate));
	ConfigManager::inst()->setValue("audioengine", "framesperaudiobuffer",
					QString::number(m_bufferSize));
	ConfigManager::inst()->setValue("audioengine", "mididev",
					m_midiIfaceNames[m_midiInterfaces->currentText()]);
	ConfigManager::inst()->setValue("midi", "midiautoassign",
					m_assignableMidiDevices->currentText());
	ConfigManager::inst()->setValue("midi", "autoquantize", QString::number(m_midiAutoQuantize));


	ConfigManager::inst()->setWorkingDir(QDir::fromNativeSeparators(m_workingDir));
	ConfigManager::inst()->setVSTDir(QDir::fromNativeSeparators(m_vstDir));
	ConfigManager::inst()->setLADSPADir(QDir::fromNativeSeparators(m_ladspaDir));
	ConfigManager::inst()->setSF2Dir(QDir::fromNativeSeparators(m_sf2Dir));
#ifdef LMMS_HAVE_FLUIDSYNTH
	ConfigManager::inst()->setSF2File(m_sf2File);
#endif
	ConfigManager::inst()->setGIGDir(QDir::fromNativeSeparators(m_gigDir));
	ConfigManager::inst()->setThemeDir(QDir::fromNativeSeparators(m_themeDir));
	ConfigManager::inst()->setBackgroundPicFile(m_backgroundPicFile);

	// Tell all audio-settings-widgets to save their settings.
	for(AswMap::iterator it = m_audioIfaceSetupWidgets.begin();
		it != m_audioIfaceSetupWidgets.end(); ++it)
	{
		it.value()->saveSettings();
	}
	// Tell all MIDI-settings-widgets to save their settings.
	for(MswMap::iterator it = m_midiIfaceSetupWidgets.begin();
		it != m_midiIfaceSetupWidgets.end(); ++it)
	{
		it.value()->saveSettings();
	}
	ConfigManager::inst()->saveConfigFile();
}




// General settings slots.

void SetupDialog::toggleTooltips(bool enabled)
{
	m_tooltips = enabled;
}


void SetupDialog::toggleDisplayWaveform(bool enabled)
{
	m_displayWaveform = enabled;
}


void SetupDialog::toggleNoteLabels(bool enabled)
{
	m_printNoteLabels = enabled;
}

void SetupDialog::toggleShowFaderTicks(bool enabled)
{
	m_showFaderTicks = enabled;
}

void SetupDialog::toggleCompactTrackButtons(bool enabled)
{
	m_compactTrackButtons = enabled;
}


void SetupDialog::toggleOneInstrumentTrackWindow(bool enabled)
{
	m_oneInstrumentTrackWindow = enabled;
}


void SetupDialog::toggleSideBarOnRight(bool enabled)
{
	m_sideBarOnRight = enabled;
}


void SetupDialog::toggleLetPreviewsFinish(bool enabled)
{
	m_letPreviewsFinish = enabled;
}


void SetupDialog::toggleTrackDeletionWarning(bool enabled)
{
	m_trackDeletionWarning = enabled;
}

void SetupDialog::toggleMixerChannelDeletionWarning(bool enabled)
{
	m_mixerChannelDeletionWarning = enabled;
}


void SetupDialog::toggleMMPZ(bool enabled)
{
	m_MMPZ = enabled;
}


void SetupDialog::toggleDisableBackup(bool enabled)
{
	m_disableBackup = enabled;
}


void SetupDialog::toggleOpenLastProject(bool enabled)
{
	m_openLastProject = enabled;
}


void SetupDialog::loopMarkerModeChanged()
{
	m_loopMarkerMode = m_loopMarkerComboBox->currentData().toString();
}


void SetupDialog::setLanguage(int lang)
{
	m_lang = m_languages[lang];
}


void SetupDialog::toggleSoloLegacyBehavior(bool enabled)
{
	m_soloLegacyBehavior = enabled;
}


// Performance settings slots.

void SetupDialog::setAutoSaveInterval(int value)
{
	m_saveInterval = value;
	m_saveIntervalSlider->setValue(m_saveInterval);
	QString minutes = m_saveInterval > 1 ? tr("minutes") : tr("minute");
	minutes = QString("%1 %2").arg(QString::number(m_saveInterval), minutes);
	minutes = m_enableAutoSave ?  minutes : tr("Disabled");
	m_saveIntervalLbl->setText(
		tr("Autosave interval: %1").arg(minutes));
}


void SetupDialog::toggleAutoSave(bool enabled)
{
	m_enableAutoSave = enabled;
	m_saveIntervalSlider->setEnabled(enabled);
	m_runningAutoSave->setVisible(enabled);
	setAutoSaveInterval(m_saveIntervalSlider->value());
}


void SetupDialog::toggleRunningAutoSave(bool enabled)
{
	m_enableRunningAutoSave = enabled;
}


void SetupDialog::resetAutoSave()
{
	setAutoSaveInterval(MainWindow::DEFAULT_SAVE_INTERVAL_MINUTES);
	m_autoSave->setChecked(true);
	m_runningAutoSave->setChecked(false);
}


void SetupDialog::toggleSmoothScroll(bool enabled)
{
	m_smoothScroll = enabled;
}


void SetupDialog::toggleAnimateAFP(bool enabled)
{
	m_animateAFP = enabled;
}


void SetupDialog::vstEmbedMethodChanged()
{
	m_vstEmbedMethod = m_vstEmbedComboBox->currentData().toString();
	m_vstAlwaysOnTopCheckBox->setVisible(m_vstEmbedMethod == "none");
}


void SetupDialog::toggleVSTAlwaysOnTop(bool enabled)
{
	m_vstAlwaysOnTop = enabled;
}


void SetupDialog::toggleDisableAutoQuit(bool enabled)
{
	m_disableAutoQuit = enabled;
}

void SetupDialog::audioInterfaceChanged(const QString & iface)
{
	for(AswMap::iterator it = m_audioIfaceSetupWidgets.begin();
		it != m_audioIfaceSetupWidgets.end(); ++it)
	{
		it.value()->hide();
	}

	m_audioIfaceSetupWidgets[m_audioIfaceNames[iface]]->show();
}


void SetupDialog::updateBufferSizeWarning(int value)
{
	QString text = "<ul>";
	// 'value' is not a power of 2 (for value > 0) and under 256. On buffer sizes larger than 256
	// lmms works with chunks of size 256 and only the final mix will use the actual buffer size.
	// Plugins don't see a larger buffer size than 256 so anything larger than this is functionally
	// a 'power of 2' value.
	if(((value & (value - 1)) != 0) && value < 256)
	{
		text += "<li>" + tr("The currently selected value is not a power of 2 "
					"(32, 64, 128, 256). Some plugins may not be available.") + "</li>";
	}
	if(value <= 32)
	{
		text += "<li>" + tr("The currently selected value is less than or equal to 32. "
					"Some plugins may not be available.") + "</li>";
	}
	text += "</ul>";
	m_bufferSizeWarnLbl->setText(text);
}


void SetupDialog::setBufferSize(int value)
{
	const int step = DEFAULT_BUFFER_SIZE / BUFFERSIZE_RESOLUTION;
	if(value > step && value % step)
	{
		int mod_value = value % step;
		if(mod_value < step / 2)
		{
			m_bufferSizeSlider->setValue(value - mod_value);
		}
		else
		{
			m_bufferSizeSlider->setValue(value + step - mod_value);
		}
		return;
	}

	if(m_bufferSizeSlider->value() != value)
	{
		m_bufferSizeSlider->setValue(value);
	}

	m_bufferSize = value * BUFFERSIZE_RESOLUTION;
	m_bufferSizeLbl->setText(tr("Frames: %1\nLatency: %2 ms").arg(m_bufferSize).arg(
		1000.0f * m_bufferSize / Engine::audioEngine()->outputSampleRate(), 0, 'f', 1));
	updateBufferSizeWarning(m_bufferSize);
}


void SetupDialog::resetBufferSize()
{
	setBufferSize(DEFAULT_BUFFER_SIZE / BUFFERSIZE_RESOLUTION);
}


// MIDI settings slots.

void SetupDialog::midiInterfaceChanged(const QString & iface)
{
	for(MswMap::iterator it = m_midiIfaceSetupWidgets.begin();
		it != m_midiIfaceSetupWidgets.end(); ++it)
	{
		it.value()->hide();
	}

	m_midiIfaceSetupWidgets[m_midiIfaceNames[iface]]->show();
}

void SetupDialog::toggleMidiAutoQuantization(bool enabled)
{
	m_midiAutoQuantize = enabled;
}


// Paths settings slots.

void SetupDialog::openWorkingDir()
{
	QString new_dir = FileDialog::getExistingDirectory(this,
		tr("Choose the LMMS working directory"), m_workingDir);
	if (!new_dir.isEmpty())
	{
		m_workingDirLineEdit->setText(new_dir);
	}
}


void SetupDialog::setWorkingDir(const QString & workingDir)
{
	m_workingDir = workingDir;
}


void SetupDialog::openVSTDir()
{
	QString new_dir = FileDialog::getExistingDirectory(this,
		tr("Choose your VST plugins directory"), m_vstDir);
	if (!new_dir.isEmpty())
	{
		m_vstDirLineEdit->setText(new_dir);
	}
}


void SetupDialog::setVSTDir(const QString & vstDir)
{
	m_vstDir = vstDir;
}


void SetupDialog::openLADSPADir()
{
	QString new_dir = FileDialog::getExistingDirectory(this,
		tr("Choose your LADSPA plugins directory"), m_ladspaDir);
	if (!new_dir.isEmpty())
	{
		if(m_ladspaDirLineEdit->text() == "")
		{
			m_ladspaDirLineEdit->setText(new_dir);
		}
		else
		{
			m_ladspaDirLineEdit->setText(m_ladspaDirLineEdit->text() + "," +
								new_dir);
		}
	}
}


void SetupDialog::setLADSPADir(const QString & ladspaDir)
{
	m_ladspaDir = ladspaDir;
}


void SetupDialog::openSF2Dir()
{
	QString new_dir = FileDialog::getExistingDirectory(this,
		tr("Choose your SF2 directory"), m_sf2Dir);
	if (!new_dir.isEmpty())
	{
		m_sf2DirLineEdit->setText(new_dir);
	}
}


void SetupDialog::setSF2Dir(const QString & sf2Dir)
{
	m_sf2Dir = sf2Dir;
}


void SetupDialog::openSF2File()
{
#ifdef LMMS_HAVE_FLUIDSYNTH
	QString new_file = FileDialog::getOpenFileName(this,
		// TODO should this be linked to SoundFont player's formats in some way?
		tr("Choose your default soundfont"), m_sf2File, "SoundFont files (*.sf2 *.sf3)");

	if (!new_file.isEmpty())
	{
		m_sf2FileLineEdit->setText(new_file);
	}
#endif
}


void SetupDialog::setSF2File(const QString & sf2File)
{
#ifdef LMMS_HAVE_FLUIDSYNTH
	m_sf2File = sf2File;
#endif
}


void SetupDialog::openGIGDir()
{
	QString new_dir = FileDialog::getExistingDirectory(this,
		tr("Choose your GIG directory"), m_gigDir);
	if(!new_dir.isEmpty())
	{
		m_gigDirLineEdit->setText(new_dir);
	}
}


void SetupDialog::setGIGDir(const QString & gigDir)
{
	m_gigDir = gigDir;
}


void SetupDialog::openThemeDir()
{
	QString new_dir = FileDialog::getExistingDirectory(this,
		tr("Choose your theme directory"), m_themeDir);
	if(!new_dir.isEmpty())
	{
		m_themeDirLineEdit->setText(new_dir);
	}
}


void SetupDialog::setThemeDir(const QString & themeDir)
{
	m_themeDir = themeDir;
}


void SetupDialog::openBackgroundPicFile()
{
	QList<QByteArray> fileTypesList = QImageReader::supportedImageFormats();
	QString fileTypes;
	for(int i = 0; i < fileTypesList.count(); i++)
	{
		if(fileTypesList[i] != fileTypesList[i].toUpper())
		{
			if(!fileTypes.isEmpty())
			{
				fileTypes += " ";
			}
			fileTypes += "*." + QString(fileTypesList[i]);
		}
	}

	QString dir = (m_backgroundPicFile.isEmpty()) ?
		m_themeDir :
		m_backgroundPicFile;
	QString new_file = FileDialog::getOpenFileName(this,
		tr("Choose your background picture"), dir, "Picture files (" + fileTypes + ")");

	if(!new_file.isEmpty())
	{
		m_backgroundPicFileLineEdit->setText(new_file);
	}
}


void SetupDialog::setBackgroundPicFile(const QString & backgroundPicFile)
{
	m_backgroundPicFile = backgroundPicFile;
}




void SetupDialog::showRestartWarning()
{
	restartWarningLbl->show();
}


} // namespace lmms::gui
