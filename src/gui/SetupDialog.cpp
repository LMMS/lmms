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


#include <QComboBox>
#include <QImageReader>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollArea>

#include "debug.h"
#include "embed.h"
#include "Engine.h"
#include "FileDialog.h"
#include "gui_templates.h"
#include "MainWindow.h"
#include "Mixer.h"
#include "ProjectJournal.h"
#include "SetupDialog.h"
#include "TabBar.h"
#include "TabButton.h"
#include "ToolTip.h"


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


constexpr int BUFFERSIZE_RESOLUTION = 32;

inline void labelWidget(QWidget * w, const QString & txt)
{
	QLabel * title = new QLabel(txt, w);
	QFont f = title->font();
	f.setBold(true);
	title->setFont(pointSize<12>(f));


	assert(dynamic_cast<QBoxLayout *>(w->layout()) != NULL);

	dynamic_cast<QBoxLayout *>(w->layout())->addSpacing(5);
	dynamic_cast<QBoxLayout *>(w->layout())->addWidget(title);
}




SetupDialog::SetupDialog(ConfigTabs tab_to_open) :
	m_displaydBFS(ConfigManager::inst()->value(
			"app", "displaydbfs").toInt()),
	m_tooltips(!ConfigManager::inst()->value(
			"tooltips", "disabled").toInt()),
	m_displayWaveform(ConfigManager::inst()->value(
			"ui", "displaywaveform").toInt()),
	m_printNoteLabels(ConfigManager::inst()->value(
			"ui", "printnotelabels").toInt()),
	m_compactTrackButtons(ConfigManager::inst()->value(
			"ui", "compacttrackbuttons").toInt()),
	m_oneInstrumentTrackWindow(ConfigManager::inst()->value(
			"ui", "oneinstrumenttrackwindow").toInt()),
	m_MMPZ(!ConfigManager::inst()->value(
			"app", "nommpz").toInt()),
	m_disableBackup(!ConfigManager::inst()->value(
			"app", "disablebackup").toInt()),
	m_openLastProject(ConfigManager::inst()->value(
			"app", "openlastproject").toInt()),
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
	m_syncVSTPlugins(ConfigManager::inst()->value(
			"ui", "syncvstplugins", "1").toInt()),
	m_disableAutoQuit(ConfigManager::inst()->value(
			"ui", "disableautoquit", "1").toInt()),
	m_NaNHandler(ConfigManager::inst()->value(
			"app", "nanhandler", "1").toInt()),
	m_hqAudioDev(ConfigManager::inst()->value(
			"mixer", "hqaudio").toInt()),
	m_bufferSize(ConfigManager::inst()->value(
			"mixer", "framesperaudiobuffer").toInt()),
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
	// TODO: Equivalent to the new setWindowFlag(Qt::WindowContextHelpButtonHint, false)
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setModal(true);
	setFixedSize(454, 400);

	Engine::projectJournal()->setJournalling(false);


	// Constants for positioning LED check boxes.
	const int XDelta = 10;
	const int YDelta = 18;

	// Main widget.
	QWidget * main_w = new QWidget(this);


	// Vertical layout.
	QVBoxLayout * vlayout = new QVBoxLayout(this);
	vlayout->setSpacing(0);
	vlayout->setMargin(0);

	// Horizontal layout.
	QHBoxLayout * hlayout = new QHBoxLayout(main_w);
	hlayout->setSpacing(0);
	hlayout->setMargin(0);

	// Tab bar for the main tabs.
	m_tabBar = new TabBar(main_w, QBoxLayout::TopToBottom);
	m_tabBar->setExclusive(true);
	m_tabBar->setFixedWidth(72);

	// Settings widget.
	QWidget * settings_w = new QWidget(main_w);
	settings_w->setFixedSize(360, 360);

	// General widget.
	QWidget * general_w = new QWidget(settings_w);
	QVBoxLayout * general_layout = new QVBoxLayout(general_w);
	general_layout->setSpacing(10);
	general_layout->setMargin(0);
	labelWidget(general_w, tr("General"));


	auto addLedCheckBox = [&XDelta, &YDelta, this](
		const char* ledText,
		TabWidget* tw,
		int& counter,
		bool initialState,
		const char* toggledSlot,
		bool showRestartWarning
	){
		LedCheckBox * checkBox = new LedCheckBox(tr(ledText), tw);
		counter++;
		checkBox->move(XDelta, YDelta * counter);
		checkBox->setChecked(initialState);
		connect(checkBox, SIGNAL(toggled(bool)), this, toggledSlot);
		if (showRestartWarning)
		{
			connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(showRestartWarning()));
		}
	};


	int counter = 0;

	// GUI tab.
	TabWidget * gui_tw = new TabWidget(
			tr("Graphical user interface (GUI)"), general_w);


	addLedCheckBox("Display volume as dBFS ", gui_tw, counter,
		m_displaydBFS, SLOT(toggleDisplaydBFS(bool)), true);
	addLedCheckBox("Enable tooltips", gui_tw, counter,
		m_tooltips, SLOT(toggleTooltips(bool)), true);
	addLedCheckBox("Enable master oscilloscope by default", gui_tw, counter,
		m_displayWaveform, SLOT(toggleDisplayWaveform(bool)), true);
	addLedCheckBox("Enable all note labels in piano roll", gui_tw, counter,
		m_printNoteLabels, SLOT(toggleNoteLabels(bool)), false);
	addLedCheckBox("Enable compact track buttons", gui_tw, counter,
		m_compactTrackButtons, SLOT(toggleCompactTrackButtons(bool)), true);
	addLedCheckBox("Enable one instrument-track-window mode", gui_tw, counter,
		m_oneInstrumentTrackWindow, SLOT(toggleOneInstrumentTrackWindow(bool)), true);

	gui_tw->setFixedHeight(YDelta + YDelta * counter);


	counter = 0;

	// Projects tab.
	TabWidget * projects_tw = new TabWidget(
			tr("Projects"), general_w);


	addLedCheckBox("Compress project files by default", projects_tw, counter,
		m_MMPZ, SLOT(toggleMMPZ(bool)), true);
	addLedCheckBox("Create a backup file when saving a project", projects_tw, counter,
		m_disableBackup, SLOT(toggleDisableBackup(bool)), false);
	addLedCheckBox("Reopen last project on startup", projects_tw, counter,
		m_openLastProject, SLOT(toggleOpenLastProject(bool)), false);

	projects_tw->setFixedHeight(YDelta + YDelta * counter);

	// Language tab.
	TabWidget * lang_tw = new TabWidget(
			tr("Language"), general_w);
	lang_tw->setFixedHeight(48);
	QComboBox * changeLang = new QComboBox(lang_tw);
	changeLang->move(XDelta, 20);

	QDir dir(ConfigManager::inst()->localeDir());
	QStringList fileNames = dir.entryList(QStringList("*.qm"));
	for(int i = 0; i < fileNames.size(); ++i)
	{
		// Get locale extracted by filename.
		fileNames[i].truncate(fileNames[i].lastIndexOf('.'));
		m_languages.append(fileNames[i]);
		QString lang = QLocale(m_languages.last()).nativeLanguageName();
		changeLang->addItem(lang);
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


	// General layout ordering.
	general_layout->addWidget(gui_tw);
	general_layout->addWidget(projects_tw);
	general_layout->addWidget(lang_tw);
	general_layout->addStretch();




	// Performance widget.
	QWidget * performance_w = new QWidget(settings_w);
	QVBoxLayout * performance_layout = new QVBoxLayout(performance_w);
	performance_layout->setSpacing(10);
	performance_layout->setMargin(0);
	labelWidget(performance_w,
			tr("Performance"));


	// Autosave tab.
	TabWidget * auto_save_tw = new TabWidget(
			tr("Autosave"), performance_w);
	auto_save_tw->setFixedHeight(106);

	m_saveIntervalSlider = new QSlider(Qt::Horizontal, auto_save_tw);
	m_saveIntervalSlider->setValue(m_saveInterval);
	m_saveIntervalSlider->setRange(1, 20);
	m_saveIntervalSlider->setTickInterval(1);
	m_saveIntervalSlider->setPageStep(1);
	m_saveIntervalSlider->setGeometry(10, 18, 340, 18);
	m_saveIntervalSlider->setTickPosition(QSlider::TicksBelow);

	connect(m_saveIntervalSlider, SIGNAL(valueChanged(int)),
			this, SLOT(setAutoSaveInterval(int)));

	m_saveIntervalLbl = new QLabel(auto_save_tw);
	m_saveIntervalLbl->setGeometry(10, 40, 200, 24);
	setAutoSaveInterval(m_saveIntervalSlider->value());

	m_autoSave = new LedCheckBox(
			tr("Enable autosave"), auto_save_tw);
	m_autoSave->move(10, 70);
	m_autoSave->setChecked(m_enableAutoSave);
	connect(m_autoSave, SIGNAL(toggled(bool)),
			this, SLOT(toggleAutoSave(bool)));

	m_runningAutoSave = new LedCheckBox(
			tr("Allow autosave while playing"), auto_save_tw);
	m_runningAutoSave->move(20, 88);
	m_runningAutoSave->setChecked(m_enableRunningAutoSave);
	connect(m_runningAutoSave, SIGNAL(toggled(bool)),
			this, SLOT(toggleRunningAutoSave(bool)));

	QPushButton * autoSaveResetBtn = new QPushButton(
			embed::getIconPixmap("reload"), "", auto_save_tw);
	autoSaveResetBtn->setGeometry(320, 70, 28, 28);
	connect(autoSaveResetBtn, SIGNAL(clicked()),
			this, SLOT(resetAutoSave()));

	m_saveIntervalSlider->setEnabled(m_enableAutoSave);
	m_runningAutoSave->setVisible(m_enableAutoSave);


	counter = 0;

	// UI effect vs. performance tab.
	TabWidget * ui_fx_tw = new TabWidget(
			tr("User interface (UI) effects vs. performance"), performance_w);

	addLedCheckBox("Smooth scroll in song editor", ui_fx_tw, counter,
		m_smoothScroll, SLOT(toggleSmoothScroll(bool)), false);
	addLedCheckBox("Display playback cursor in AudioFileProcessor", ui_fx_tw, counter,
		m_animateAFP, SLOT(toggleAnimateAFP(bool)), false);

	ui_fx_tw->setFixedHeight(YDelta + YDelta * counter);


	counter = 0;

	// Plugins tab.
	TabWidget * plugins_tw = new TabWidget(
			tr("Plugins"), performance_w);

	m_vstEmbedLbl = new QLabel(plugins_tw);
	m_vstEmbedLbl->move(XDelta, YDelta * ++counter);
	m_vstEmbedLbl->setText(tr("VST plugins embedding:"));

	m_vstEmbedComboBox = new QComboBox(plugins_tw);
	m_vstEmbedComboBox->move(XDelta, YDelta * ++counter);

	QStringList embedMethods = ConfigManager::availabeVstEmbedMethods();
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

	counter += 2;

	m_vstAlwaysOnTopCheckBox = new LedCheckBox(
			tr("Keep plugin windows on top when not embedded"), plugins_tw);
	m_vstAlwaysOnTopCheckBox->move(20, 66);
	m_vstAlwaysOnTopCheckBox->setChecked(m_vstAlwaysOnTop);
	m_vstAlwaysOnTopCheckBox->setVisible(m_vstEmbedMethod == "none");
	connect(m_vstAlwaysOnTopCheckBox, SIGNAL(toggled(bool)),
			this, SLOT(toggleVSTAlwaysOnTop(bool)));

	addLedCheckBox("Sync VST plugins to host playback", plugins_tw, counter,
		m_syncVSTPlugins, SLOT(toggleSyncVSTPlugins(bool)), false);

	addLedCheckBox("Keep effects running even without input", plugins_tw, counter,
		m_disableAutoQuit, SLOT(toggleDisableAutoQuit(bool)), false);

	plugins_tw->setFixedHeight(YDelta + YDelta * counter);


	// Performance layout ordering.
	performance_layout->addWidget(auto_save_tw);
	performance_layout->addWidget(ui_fx_tw);
	performance_layout->addWidget(plugins_tw);
	performance_layout->addStretch();



	// Audio widget.
	QWidget * audio_w = new QWidget(settings_w);
	QVBoxLayout * audio_layout = new QVBoxLayout(audio_w);
	audio_layout->setSpacing(10);
	audio_layout->setMargin(0);
	labelWidget(audio_w,
			tr("Audio"));

	// Audio interface tab.
	TabWidget * audioiface_tw = new TabWidget(
			tr("Audio interface"), audio_w);
	audioiface_tw->setFixedHeight(56);

	m_audioInterfaces = new QComboBox(audioiface_tw);
	m_audioInterfaces->setGeometry(10, 20, 240, 28);


	// Ifaces-settings-widget.
	QWidget * as_w = new QWidget(audio_w);
	as_w->setFixedHeight(60);

	QHBoxLayout * as_w_layout = new QHBoxLayout(as_w);
	as_w_layout->setSpacing(0);
	as_w_layout->setMargin(0);

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
			tr(it.key().toLatin1())] = it.key();
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
	QString audioDevName = ConfigManager::inst()->value("mixer", "audiodev");
	if (m_audioInterfaces->findText(audioDevName) < 0)
	{
		audioDevName = Engine::mixer()->audioDevName();
		ConfigManager::inst()->setValue("mixer", "audiodev", audioDevName);
	}
	m_audioInterfaces->
		setCurrentIndex(m_audioInterfaces->findText(audioDevName));
	m_audioIfaceSetupWidgets[audioDevName]->show();

	connect(m_audioInterfaces, SIGNAL(activated(const QString &)),
			this, SLOT(audioInterfaceChanged(const QString &)));

	// Advanced setting, hidden for now
	if(false)
	{
		LedCheckBox * useNaNHandler = new LedCheckBox(
			tr("Use built-in NaN handler"), audio_w);
		useNaNHandler->setChecked(m_NaNHandler);
	}

	// HQ mode LED.
	LedCheckBox * hqaudio = new LedCheckBox(
			tr("HQ mode for output audio device"), audio_w);
	hqaudio->move(10, 0);
	hqaudio->setChecked(m_hqAudioDev);
	connect(hqaudio, SIGNAL(toggled(bool)),
			this, SLOT(toggleHQAudioDev(bool)));


	// Buffer size tab.
	TabWidget * bufferSize_tw = new TabWidget(
			tr("Buffer size"), audio_w);
	bufferSize_tw->setFixedHeight(76);

	m_bufferSizeSlider = new QSlider(Qt::Horizontal, bufferSize_tw);
	m_bufferSizeSlider->setRange(1, 128);
	m_bufferSizeSlider->setTickInterval(8);
	m_bufferSizeSlider->setPageStep(8);
	m_bufferSizeSlider->setValue(m_bufferSize / BUFFERSIZE_RESOLUTION);
	m_bufferSizeSlider->setGeometry(10, 18, 340, 18);
	m_bufferSizeSlider->setTickPosition(QSlider::TicksBelow);

	connect(m_bufferSizeSlider, SIGNAL(valueChanged(int)),
			this, SLOT(setBufferSize(int)));
	connect(m_bufferSizeSlider, SIGNAL(valueChanged(int)),
			this, SLOT(showRestartWarning()));

	m_bufferSizeLbl = new QLabel(bufferSize_tw);
	m_bufferSizeLbl->setGeometry(10, 40, 200, 24);
	setBufferSize(m_bufferSizeSlider->value());

	QPushButton * bufferSize_reset_btn = new QPushButton(
			embed::getIconPixmap("reload"), "", bufferSize_tw);
	bufferSize_reset_btn->setGeometry(320, 40, 28, 28);
	connect(bufferSize_reset_btn, SIGNAL(clicked()),
			this, SLOT(resetBufferSize()));
	ToolTip::add(bufferSize_reset_btn,
			tr("Reset to default value"));


	// Audio layout ordering.
	audio_layout->addWidget(audioiface_tw);
	audio_layout->addWidget(as_w);
	audio_layout->addWidget(hqaudio);
	audio_layout->addWidget(bufferSize_tw);
	audio_layout->addStretch();



	// MIDI widget.
	QWidget * midi_w = new QWidget(settings_w);
	QVBoxLayout * midi_layout = new QVBoxLayout(midi_w);
	midi_layout->setSpacing(10);
	midi_layout->setMargin(0);
	labelWidget(midi_w,
			tr("MIDI"));

	// MIDI interface tab.
	TabWidget * midiiface_tw = new TabWidget(
			tr("MIDI interface"), midi_w);
	midiiface_tw->setFixedHeight(56);

	m_midiInterfaces = new QComboBox(midiiface_tw);
	m_midiInterfaces->setGeometry(10, 20, 240, 28);

	// Ifaces-settings-widget.
	QWidget * ms_w = new QWidget(midi_w);
	ms_w->setFixedHeight(60);

	QHBoxLayout * ms_w_layout = new QHBoxLayout(ms_w);
	ms_w_layout->setSpacing(0);
	ms_w_layout->setMargin(0);

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
			tr(it.key().toLatin1())] = it.key();
	}
	for(trMap::iterator it = m_midiIfaceNames.begin();
		it != m_midiIfaceNames.end(); ++it)
	{
		QWidget * midiWidget = m_midiIfaceSetupWidgets[it.value()];
		midiWidget->hide();
		ms_w_layout->addWidget(midiWidget);
		m_midiInterfaces->addItem(it.key());
	}

	QString midiDevName = ConfigManager::inst()->value("mixer", "mididev");
	if (m_midiInterfaces->findText(midiDevName) < 0)
	{
		midiDevName = Engine::mixer()->midiClientName();
		ConfigManager::inst()->setValue("mixer", "mididev", midiDevName);
	}
	m_midiInterfaces->setCurrentIndex(m_midiInterfaces->findText(midiDevName));
	m_midiIfaceSetupWidgets[midiDevName]->show();

	connect(m_midiInterfaces, SIGNAL(activated(const QString &)),
			this, SLOT(midiInterfaceChanged(const QString &)));


	// MIDI layout ordering.
	midi_layout->addWidget(midiiface_tw);
	midi_layout->addWidget(ms_w);
	midi_layout->addStretch();



	// Paths widget.
	QWidget * paths_w = new QWidget(settings_w);

	QVBoxLayout * paths_layout = new QVBoxLayout(paths_w);
	paths_layout->setSpacing(10);
	paths_layout->setMargin(0);

	labelWidget(paths_w, tr("Paths"));


	// Paths scroll area.
	QScrollArea * pathsScroll = new QScrollArea(paths_w);
	pathsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	pathsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// Path selectors widget.
	QWidget * pathSelectors = new QWidget(paths_w);

	const int txtLength = 284;
	const int btnStart = 300;

	// Path selectors layout.
	QVBoxLayout * pathSelectorsLayout = new QVBoxLayout;
	pathSelectorsLayout->setSpacing(10);

	auto addPathEntry = [&](const char* caption,
		const QString& content,
		const char* setSlot,
		const char* openSlot,
		QLineEdit*& lineEdit,
		const char* pixmap = "project_open")
	{
		TabWidget * newTw = new TabWidget(tr(caption),
					pathSelectors);
		newTw->setFixedHeight(48);

		lineEdit = new QLineEdit(content, newTw);
		lineEdit->setGeometry(10, 20, txtLength, 16);
		connect(lineEdit, SIGNAL(textChanged(const QString &)),
			this, setSlot);

		QPushButton * selectBtn = new QPushButton(
			embed::getIconPixmap(pixmap, 16, 16),
			"", newTw);
		selectBtn->setFixedSize(24, 24);
		selectBtn->move(btnStart, 16);
		connect(selectBtn, SIGNAL(clicked()), this, openSlot);

		pathSelectorsLayout->addWidget(newTw);
		pathSelectorsLayout->addSpacing(10);
	};

	addPathEntry("LMMS working directory", m_workingDir,
		SLOT(setWorkingDir(const QString &)),
		SLOT(openWorkingDir()),
		m_workingDirLineEdit);
	addPathEntry("VST plugins directory", m_vstDir,
		SLOT(setVSTDir(const QString &)),
		SLOT(openVSTDir()),
		m_vstDirLineEdit);
	addPathEntry("LADSPA plugins directories", m_ladspaDir,
		SLOT(setLADSPADir(const QString &)),
		SLOT(openLADSPADir()),
		m_ladspaDirLineEdit, "add_folder");
	addPathEntry("SF2 directory", m_sf2Dir,
		SLOT(setSF2Dir(const QString &)),
		SLOT(openSF2Dir()),
		m_sf2DirLineEdit);
#ifdef LMMS_HAVE_FLUIDSYNTH
	addPathEntry("Default SF2", m_sf2File,
		SLOT(setSF2File(const QString &)),
		SLOT(openSF2File()),
		m_sf2FileLineEdit);
#endif
	addPathEntry("GIG directory", m_gigDir,
		SLOT(setGIGDir(const QString &)),
		SLOT(openGIGDir()),
		m_gigDirLineEdit);
	addPathEntry("Theme directory", m_themeDir,
		SLOT(setThemeDir(const QString &)),
		SLOT(openThemeDir()),
		m_themeDirLineEdit);
	addPathEntry("Background artwork", m_backgroundPicFile,
		SLOT(setBackgroundPicFile(const QString &)),
		SLOT(openBackgroundPicFile()),
		m_backgroundPicFileLineEdit);

	pathSelectorsLayout->addStretch();

	pathSelectors->setLayout(pathSelectorsLayout);

	pathsScroll->setWidget(pathSelectors);
	pathsScroll->setWidgetResizable(true);

	paths_layout->addWidget(pathsScroll);
	paths_layout->addStretch();

	// Major tabs ordering.
	m_tabBar->addTab(general_w,
			tr("General"), 0, false, true)->setIcon(
					embed::getIconPixmap("setup_general"));
	m_tabBar->addTab(performance_w,
			tr("Performance"), 1, false, true)->setIcon(
					embed::getIconPixmap("setup_performance"));
	m_tabBar->addTab(audio_w,
			tr("Audio"), 2, false, true)->setIcon(
					embed::getIconPixmap("setup_audio"));
	m_tabBar->addTab(midi_w,
			tr("MIDI"), 3, false, true)->setIcon(
					embed::getIconPixmap("setup_midi"));
	m_tabBar->addTab(paths_w,
			tr("Paths"), 4, true, true)->setIcon(
					embed::getIconPixmap("setup_directories"));

	m_tabBar->setActiveTab(tab_to_open);

	// Horizontal layout ordering.
	hlayout->addSpacing(2);
	hlayout->addWidget(m_tabBar);
	hlayout->addSpacing(10);
	hlayout->addWidget(settings_w);
	hlayout->addSpacing(10);

	// Extras widget and layout.
	QWidget * extras_w = new QWidget(this);
	QHBoxLayout * extras_layout = new QHBoxLayout(extras_w);
	extras_layout->setSpacing(0);
	extras_layout->setMargin(0);

	// Restart warning label.
	restartWarningLbl = new QLabel(
			tr("Some changes require restarting."), extras_w);
	restartWarningLbl->hide();

	// OK button.
	QPushButton * ok_btn = new QPushButton(
			embed::getIconPixmap("apply"),
			tr("OK"), extras_w);
	connect(ok_btn, SIGNAL(clicked()),
			this, SLOT(accept()));

	// Cancel button.
	QPushButton * cancel_btn = new QPushButton(
			embed::getIconPixmap("cancel"),
			tr("Cancel"), extras_w);
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
	vlayout->addWidget(main_w);
	vlayout->addSpacing(10);
	vlayout->addWidget(extras_w);
	vlayout->addSpacing(10);

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

	ConfigManager::inst()->setValue("app", "displaydbfs",
					QString::number(m_displaydBFS));
	ConfigManager::inst()->setValue("tooltips", "disabled",
					QString::number(!m_tooltips));
	ConfigManager::inst()->setValue("ui", "displaywaveform",
					QString::number(m_displayWaveform));
	ConfigManager::inst()->setValue("ui", "printnotelabels",
					QString::number(m_printNoteLabels));
	ConfigManager::inst()->setValue("ui", "compacttrackbuttons",
					QString::number(m_compactTrackButtons));
	ConfigManager::inst()->setValue("ui", "oneinstrumenttrackwindow",
					QString::number(m_oneInstrumentTrackWindow));
	ConfigManager::inst()->setValue("app", "nommpz",
					QString::number(!m_MMPZ));
	ConfigManager::inst()->setValue("app", "disablebackup",
					QString::number(!m_disableBackup));
	ConfigManager::inst()->setValue("app", "openlastproject",
					QString::number(m_openLastProject));
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
	ConfigManager::inst()->setValue("ui", "syncvstplugins",
					QString::number(m_syncVSTPlugins));
	ConfigManager::inst()->setValue("ui", "disableautoquit",
					QString::number(m_disableAutoQuit));
	ConfigManager::inst()->setValue("mixer", "audiodev",
					m_audioIfaceNames[m_audioInterfaces->currentText()]);
	ConfigManager::inst()->setValue("app", "nanhandler",
					QString::number(m_NaNHandler));
	ConfigManager::inst()->setValue("mixer", "hqaudio",
					QString::number(m_hqAudioDev));
	ConfigManager::inst()->setValue("mixer", "framesperaudiobuffer",
					QString::number(m_bufferSize));
	ConfigManager::inst()->setValue("mixer", "mididev",
					m_midiIfaceNames[m_midiInterfaces->currentText()]);


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

void SetupDialog::toggleDisplaydBFS(bool enabled)
{
	m_displaydBFS = enabled;
}


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


void SetupDialog::toggleCompactTrackButtons(bool enabled)
{
	m_compactTrackButtons = enabled;
}


void SetupDialog::toggleOneInstrumentTrackWindow(bool enabled)
{
	m_oneInstrumentTrackWindow = enabled;
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


void SetupDialog::setLanguage(int lang)
{
	m_lang = m_languages[lang];
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


void SetupDialog::toggleSyncVSTPlugins(bool enabled)
{
	m_syncVSTPlugins = enabled;
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




// Audio settings slots.

void SetupDialog::toggleHQAudioDev(bool enabled)
{
	m_hqAudioDev = enabled;
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
		1000.0f * m_bufferSize / Engine::mixer()->processingSampleRate(), 0, 'f', 1));
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
		tr("Choose your default SF2"), m_sf2File, "SoundFont 2 files (*.sf2)");

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
	if(new_dir != QString::null)
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
	if(new_dir != QString::null)
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

	if(new_file != QString::null)
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
