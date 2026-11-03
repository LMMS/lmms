/*
 * AudioPipeWire.h - Audio driver for PipeWire
 *
 * Copyright (c) 2024- Michael Gregorius
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

#include "AudioPipeWire.h"

#ifdef LMMS_HAVE_PIPEWIRE

#include "AudioEngine.h"
#include "ConfigManager.h"
#include "lmms_constants.h"

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

#include <QAction>
#include <QFormLayout>
#include <QMenu>
#include <QToolButton>

#include <algorithm>
#include <cstring>
#include <atomic>
#include <vector>


namespace lmms
{

namespace
{

const auto audioPipeWireClass = QString{"audiopipewire"};
const auto outputKey = QString{"output"};
const auto inputKey = QString{"input"};

// An empty output selects the default device; an empty or "-" input disables capture.
const auto disconnectedRepresentation = QString{"-"};

} // namespace

// Callback method which is called by PipeWire whenever the output stream must be processed
static void on_output_process(void *userdata)
{
	AudioPipeWire* data = static_cast<AudioPipeWire*>(userdata);
	data->processOutputStreams();
}

// Callback method which is called by PipeWire whenever the capture stream must be processed
static void on_capture_process(void *userdata)
{
	AudioPipeWire* data = static_cast<AudioPipeWire*>(userdata);
	data->processCaptureStreams();
}

// This callback method is used to determine the negotiated sample rates for output and capture
static void notifyFormat(void* userdata, bool capture, uint32_t id, const spa_pod* param)
{
	if (id != SPA_PARAM_Format || param == nullptr) { return; }

	uint32_t mediaType = 0;
	uint32_t mediaSubtype = 0;
	if (spa_format_parse(param, &mediaType, &mediaSubtype) < 0) { return; }
	if (mediaType != SPA_MEDIA_TYPE_audio || mediaSubtype != SPA_MEDIA_SUBTYPE_raw) { return; }

	spa_audio_info_raw info{};
	if (spa_format_audio_raw_parse(param, &info) < 0) { return; }

	static_cast<AudioPipeWire*>(userdata)->onFormatNegotiated(capture, info.rate);
}

static void on_output_param_changed(void* userdata, uint32_t id, const spa_pod* param)
{
	notifyFormat(userdata, false, id, param);
}

static void on_capture_param_changed(void* userdata, uint32_t id, const spa_pod* param)
{
	notifyFormat(userdata, true, id, param);
}

static const struct pw_stream_events output_stream_events = {
        .version = PW_VERSION_STREAM_EVENTS,
        .param_changed = on_output_param_changed,
        .process = on_output_process,
};

static const struct pw_stream_events capture_stream_events = {
        .version = PW_VERSION_STREAM_EVENTS,
        .param_changed = on_capture_param_changed,
        .process = on_capture_process,
};

static void fillWithSilence(float* buffer, unsigned long numberOfFrames)
{
	for (unsigned long i = 0; i < numberOfFrames * DEFAULT_CHANNELS; ++i)
	{
		*buffer++ = 0;
	}
}

struct AudioPipeWire::Impl
{
public:
	~Impl()
	{
		if (m_captureStream) { pw_stream_destroy(m_captureStream); }
		if (m_outputStream) { pw_stream_destroy(m_outputStream); }
		if (m_core) { pw_core_disconnect(m_core); }
		if (m_context) { pw_context_destroy(m_context); }
		if (m_loop) { pw_thread_loop_destroy(m_loop); }
	}

	pw_thread_loop* m_loop = nullptr;
	pw_context* m_context = nullptr;
    pw_core* m_core = nullptr;
    pw_stream* m_outputStream = nullptr;
	pw_stream* m_captureStream = nullptr;

	std::vector<SampleFrame> m_captureBuffer;

	// 0 until the respective stream has negotiated its format
	std::atomic<uint32_t> m_playbackRate = 0;
	std::atomic<uint32_t> m_captureRate = 0;

	// This is a scratch space like buffer that is used by PipeWire to create POD (plain old data) objects on the stack.
	uint8_t m_podBuilderBuffer[1024];
};

AudioPipeWire::AudioPipeWire(bool& successful, AudioEngine* audioEngine) :
	AudioDevice(DEFAULT_CHANNELS, audioEngine)
{
	m_impl = std::make_unique<Impl>();

	m_impl->m_captureBuffer.resize(this->audioEngine()->framesPerAudioBuffer());

	m_supportsCapture = true;

	// Pipewire initializations
	pw_init(nullptr, nullptr);
	m_impl->m_loop = pw_thread_loop_new("LMMS", nullptr);
	m_impl->m_context = pw_context_new(pw_thread_loop_get_loop(m_impl->m_loop), NULL /* properties */, 0 /* user_data size */);
	m_impl->m_core = pw_context_connect(m_impl->m_context, NULL /* properties */, 0 /* user_data size */);

	// Fetch the preferred output and capture device from the configuration
	const auto cm = ConfigManager::inst();
	const auto outputNode = cm->value(audioPipeWireClass, outputKey).toUtf8();
	const auto inputNode = cm->value(audioPipeWireClass, inputKey).toUtf8();
	const auto lmmsSampleRate = sampleRate();

	// Set the playback properties, among them the preferred sample rate and target output device
	auto playbackProps = pw_properties_new(
					PW_KEY_MEDIA_TYPE, "Audio",
					PW_KEY_MEDIA_CATEGORY, "Playback",
					PW_KEY_MEDIA_ROLE, "Music",
					PW_KEY_NODE_RATE, QString{"1/%1"}.arg(lmmsSampleRate).toUtf8().constData(),
					NULL);

	if (!outputNode.isEmpty())
	{
		pw_properties_set(playbackProps, PW_KEY_TARGET_OBJECT, outputNode.constData());
	}

	// Now create the actual output stream using the properties
	m_impl->m_outputStream = pw_stream_new_simple(
				pw_thread_loop_get_loop(m_impl->m_loop),
				"LMMS",
				playbackProps,
				&output_stream_events,
				this);
	
	// Finally connect the output stream
	const struct spa_pod *params[1];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(m_impl->m_podBuilderBuffer, sizeof(m_impl->m_podBuilderBuffer));

	auto audioInfoRaw = SPA_AUDIO_INFO_RAW_INIT(
							.format = SPA_AUDIO_FORMAT_F32,
							.rate = lmmsSampleRate,
							.channels = DEFAULT_CHANNELS);

	params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &audioInfoRaw);
 
	pw_stream_connect(m_impl->m_outputStream,
						PW_DIRECTION_OUTPUT,
						PW_ID_ANY,
						static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS),
						params, 1);

	// Prepare the capture stream if it is configured
	if (!inputNode.isEmpty() && inputNode != disconnectedRepresentation.toUtf8())
	{
		auto captureProps = pw_properties_new(
					PW_KEY_MEDIA_TYPE, "Audio",
					PW_KEY_MEDIA_CATEGORY, "Capture",
					PW_KEY_MEDIA_ROLE, "Music",
					PW_KEY_TARGET_OBJECT, inputNode.constData(),
					NULL);

		m_impl->m_captureStream = pw_stream_new_simple(
					pw_thread_loop_get_loop(m_impl->m_loop),
					"LMMS Capture",
					captureProps,
					&capture_stream_events,
					this);

		pw_stream_connect(m_impl->m_captureStream,
							PW_DIRECTION_INPUT,
							PW_ID_ANY,
							static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS),
							params, 1);
	}

	// A tempory loop is started here, so that we can find out the negotiated sample rates via callbacks
	// and inform LMMS about it.
	//
	// The loop is stopped again afterwards so that startProcessingImpl() can finally start it for good.
	successful = false;
	if (m_impl->m_core)
	{
		pw_thread_loop_start(m_impl->m_loop);
		pw_thread_loop_lock(m_impl->m_loop);

		// Initialize a deadline time spec 2 seconds from the current time
		timespec deadline{};
		pw_thread_loop_get_time(m_impl->m_loop, &deadline, 2'000'000'000LL);

		auto negotiated = [this] { return m_impl->m_playbackRate != 0 && (!m_impl->m_captureStream || m_impl->m_captureRate != 0); };
		while (!negotiated())
		{
			if (pw_thread_loop_timed_wait_full(m_impl->m_loop, &deadline) != 0) { break; }
		}

		// Without a negotiated rate LMMS cannot know what PipeWire runs at.
		// In case the rate was not negotiated, the "successful" flag remains false
		// and lets the engine try another device.
		if (m_impl->m_playbackRate != 0)
		{
			successful = true;
			if (m_impl->m_playbackRate != sampleRate()) { setSampleRate(static_cast<sample_rate_t>(m_impl->m_playbackRate.load())); }
		}

		// Finally check if we have different sample rates for output and capture
		if (m_impl->m_captureRate != 0 && m_impl->m_captureRate != sampleRate())
		{
			// Make driver initialization fail if the capture rate differs from LMMS internal sample rate
			successful = false;
			pw_log_warn("Capture rate %u differs from LMMS internal playback rate %u", m_impl->m_captureRate.load(), sampleRate());
		}

		pw_thread_loop_unlock(m_impl->m_loop);
		pw_thread_loop_stop(m_impl->m_loop);
	}
}

AudioPipeWire::~AudioPipeWire()
{
	stopProcessing();
}

void AudioPipeWire::processOutputStreams()
{
	// Fetch the buffer from the stream
	struct pw_buffer* b = pw_stream_dequeue_buffer(m_impl->m_outputStream);
	if (b == nullptr)
	{
		pw_log_warn("out of buffers: %m");
		return;
	}

	const struct spa_buffer* buf = b->buffer;

	// Fetch the destination buffer where we will write the result
	float* dst = static_cast<float*>(buf->datas[0].data);
	if (dst == nullptr) return;

	const auto stride = sizeof(float) * DEFAULT_CHANNELS;
	auto n_frames = buf->datas[0].maxsize / stride;
	if (b->requested)
	{
		n_frames = SPA_MIN(b->requested, n_frames);
	}

	if (!AudioDevice::isRunning())
	{
		fillWithSilence(dst, n_frames);
	}
	else
	{
		audioEngine()->renderNextBuffer({dst, channels(), static_cast<f_cnt_t>(n_frames)});
	}

	buf->datas[0].chunk->offset = 0;
	buf->datas[0].chunk->stride = stride;
	buf->datas[0].chunk->size = n_frames * stride;

	pw_stream_queue_buffer(m_impl->m_outputStream, b);
}

void AudioPipeWire::processCaptureStreams()
{
	struct pw_buffer* b = pw_stream_dequeue_buffer(m_impl->m_captureStream);
	if (b == nullptr) { return; }

	const struct spa_buffer* buf = b->buffer;
	const auto* src = static_cast<const float*>(buf->datas[0].data);

	if (src != nullptr)
	{
		const auto stride = sizeof(float) * DEFAULT_CHANNELS;
		auto framesLeft = buf->datas[0].chunk->size / stride;

		while (framesLeft > 0)
		{
			const auto frames = std::min<size_t>(framesLeft, m_impl->m_captureBuffer.size());
			for (size_t i = 0; i < frames; ++i)
			{
				m_impl->m_captureBuffer[i] = SampleFrame(src[0], src[1]);
				src += DEFAULT_CHANNELS;
			}

			if (AudioDevice::isRunning())
			{
				this->audioEngine()->pushInputFrames(m_impl->m_captureBuffer.data(), static_cast<f_cnt_t>(frames));
			}
			framesLeft -= frames;
		}
	}

	pw_stream_queue_buffer(m_impl->m_captureStream, b);
}

void AudioPipeWire::onFormatNegotiated(bool capture, uint32_t rate)
{
	(capture ? m_impl->m_captureRate : m_impl->m_playbackRate) = rate;
	pw_thread_loop_signal(m_impl->m_loop, false);
}

void AudioPipeWire::startProcessingImpl()
{
	pw_thread_loop_start(m_impl->m_loop);
}

void AudioPipeWire::stopProcessingImpl()
{
	if (m_impl->m_loop)
	{
		pw_thread_loop_stop(m_impl->m_loop);
	}
}


// === Setup widget related code ===
//
// Please note that as of now there's also PipeWire related code found in this section,
// because we start a short PipeWire main loop to enumerate all sinks and sources that
// are presented in the dialog.

namespace
{
// Used to save the name of a PipeWire node as a property on a QObject
const char* const nodeNameProperty = "nodeName";

// Describes the properties of a PipeWire node. Used for the device dialog.
struct NodeEntry
{
	QString name;
	QString description;
};

// A list of potential PipeWire outputs (sinks) and inputs (sources)
struct NodeLists
{
	std::vector<NodeEntry> sinks;
	std::vector<NodeEntry> sources;
};

// User data which is only used during the enumeration of the devices in a dedicated synchronous loop.
struct EnumerationUserData
{
	pw_main_loop* loop = nullptr;
	NodeLists nodes;
	int pendingSeq = 0;
};

// This callback function extracts the sinks and sources. It is called individually for each sink/sources (and some other properties).
void onRegistryGlobal(void* userdata, uint32_t /*id*/, uint32_t /*permissions*/, const char* type, uint32_t /*version*/, const spa_dict* props)
{
	if (props == nullptr || std::strcmp(type, PW_TYPE_INTERFACE_Node) != 0) { return; }

	const char* mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
	const char* name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
	if (mediaClass == nullptr || name == nullptr) { return; }

	const char* description = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
	NodeEntry entry{QString::fromUtf8(name), QString::fromUtf8(description != nullptr ? description : name)};

	auto data = static_cast<EnumerationUserData*>(userdata);
	if (std::strcmp(mediaClass, "Audio/Sink") == 0) { data->nodes.sinks.push_back(entry); }
	else if (std::strcmp(mediaClass, "Audio/Source") == 0) { data->nodes.sources.push_back(entry); }
}

// Callback which ends the enumeration main loop once all events have been triggered
void onCoreDone(void* userdata, uint32_t id, int seq)
{
	auto data = static_cast<EnumerationUserData*>(userdata);
	if (id == PW_ID_CORE && seq == data->pendingSeq) { pw_main_loop_quit(data->loop); }
}

/**
 * @brief Returns a snapshot of all available sinks and sources.
 * 
 * The sinks and sources are sorted by description.
 * 
 * @return NodeLists A snapshot of all available sinks and sources or
 *         an empty lists if no PipeWire daemon is reachable.
 */
NodeLists listAudioNodes()
{
	// The idea is to setup a main loop, evaluate the relevant events, e.g., registry events which
	// enumerate the sinks and sources, and to immediately tear everything down again after that.
	//
	// This is very similar to the mechanism that is used in the JACK driver.

	pw_init(nullptr, nullptr);

	EnumerationUserData data;
	data.loop = pw_main_loop_new(nullptr);
	auto context = pw_context_new(pw_main_loop_get_loop(data.loop), nullptr, 0);
	auto core = pw_context_connect(context, nullptr, 0);

	if (core != nullptr)
	{
		spa_hook coreListener{};
		spa_hook registryListener{};

		static const pw_core_events coreEvents = {
			.version = PW_VERSION_CORE_EVENTS,
			.done = onCoreDone,
		};
		static const pw_registry_events registryEvents = {
			.version = PW_VERSION_REGISTRY_EVENTS,
			.global = onRegistryGlobal,
		};

		pw_core_add_listener(core, &coreListener, &coreEvents, &data);
		auto registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
		pw_registry_add_listener(registry, &registryListener, &registryEvents, &data);

		data.pendingSeq = pw_core_sync(core, PW_ID_CORE, 0);
		pw_main_loop_run(data.loop);

		spa_hook_remove(&registryListener);
		spa_hook_remove(&coreListener);
		pw_proxy_destroy(reinterpret_cast<pw_proxy*>(registry));
		pw_core_disconnect(core);
	}

	pw_context_destroy(context);
	pw_main_loop_destroy(data.loop);

	// Sort sinks and sources by description
	auto byDescription = [](const NodeEntry& a, const NodeEntry& b) { return a.description < b.description; };
	std::sort(data.nodes.sinks.begin(), data.nodes.sinks.end(), byDescription);
	std::sort(data.nodes.sources.begin(), data.nodes.sources.end(), byDescription);

	return data.nodes;
}

QToolButton* buildDeviceButton(QWidget* parent, const QString& currentNodeName, const QString& emptyLabel,
	const std::vector<NodeEntry>& entries)
{
	auto toolButton = new QToolButton(parent);
	toolButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	toolButton->setPopupMode(QToolButton::InstantPopup);

	auto select = [toolButton, emptyLabel](const QString& nodeName, const QString& label)
	{
		toolButton->setProperty(nodeNameProperty, nodeName);
		toolButton->setText(nodeName.isEmpty() ? emptyLabel : label);
	};

	auto menu = new QMenu(toolButton);

	auto emptyAction = menu->addAction(emptyLabel);
	QObject::connect(emptyAction, &QAction::triggered, toolButton, [select]() { select(QString{}, QString{}); });

	select(QString{}, QString{});
	for (const auto& entry : entries)
	{
		auto action = menu->addAction(entry.description);
		QObject::connect(action, &QAction::triggered, toolButton, [select, entry]() { select(entry.name, entry.description); });

		if (entry.name == currentNodeName) { select(entry.name, entry.description); }
	}

	// Keep a saved device selected even if it is currently not available
	if (!currentNodeName.isEmpty() && toolButton->property(nodeNameProperty).toString().isEmpty())
	{
		select(currentNodeName, currentNodeName);
	}

	toolButton->setMenu(menu);
	return toolButton;
}

}


AudioPipeWire::setupWidget::setupWidget(QWidget* parent)
	: AudioDeviceSetupWidget(AudioPipeWire::name(), parent)
{
	QFormLayout* form = new QFormLayout(this);
	form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

	const auto nodes = listAudioNodes();
	const auto cm = ConfigManager::inst();

	m_outputDevice = buildDeviceButton(this, cm->value(audioPipeWireClass, outputKey), tr("Default"), nodes.sinks);
	form->addRow(tr("Output:"), m_outputDevice);

	m_inputDevice = buildDeviceButton(this, cm->value(audioPipeWireClass, inputKey), disconnectedRepresentation, nodes.sources);
	form->addRow(tr("Input:"), m_inputDevice);
}

AudioPipeWire::setupWidget::~setupWidget()
{
}

void AudioPipeWire::setupWidget::saveSettings()
{
	const auto cm = ConfigManager::inst();
	cm->setValue(audioPipeWireClass, outputKey, m_outputDevice->property(nodeNameProperty).toString());
	cm->setValue(audioPipeWireClass, inputKey, m_inputDevice->property(nodeNameProperty).toString());
}

} // namespace lmms

#endif // LMMS_HAVE_PIPEWIRE
