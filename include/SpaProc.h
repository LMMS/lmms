#ifndef SPAPROC_H
#define SPAPROC_H

#include <QMap>
#include <spa/spa_fwd.h>
#include <spa/audio_fwd.h>

#include "DataFile.h"
#include "LinkedModelGroups.h"
#include "lmms_basics.h"
#include "Note.h"
#include "../src/3rdparty/ringbuffer/include/ringbuffer/ringbuffer.h"

class AutomatableModel;

class SpaProc : public LinkedModelGroup
{
	Q_OBJECT
	friend class SpaViewBase;

public:
	SpaProc(Model *parent, const spa::descriptor* desc,
			DataFile::Types settingsType);
	~SpaProc() override;
	//! Check if ctor succeeded
	bool isValid() const { return m_valid; }

	void writeOsc(const char *dest, const char *args, va_list va);
	void writeOsc(const char *dest, const char *args, ...);


	void loadFile(const QString &file);

	void run(unsigned frames);

	//! Return the net port, or 0 if there is currently none
	unsigned netPort() const;

	const spa::descriptor *m_spaDescriptor = nullptr;
	spa::plugin *m_plugin = nullptr;

	uint64_t m_loadTicket = 0, m_saveTicket = 0, m_restoreTicket = 0;

protected:
//	void reloadPlugin();
public:
	//! create or return an OSC model for port "dest"
	//! RT safe if the model already exists
	AutomatableModel *modelAtPort(const QString &dest);

	int m_audioInCount = 0, m_audioOutCount = 0;

	std::size_t controlCount() const { return LinkedModelGroup::modelNum(); }

	struct LmmsPorts
	{
		unsigned samplecount;
		unsigned buffersize;
		long samplerate; // TODO: use const?
		std::vector<float> m_lUnprocessed, m_rUnprocessed, m_lProcessed,
			m_rProcessed;
		// only for directly connected ports (not OSC)
		struct TypedPorts
		{
			char m_type;
			// storage, plugin references this
			union
			{
				float m_f;
				int m_i;
				bool m_b;
			} m_val;
			// models where we copy the values from/to
			union // TODO: use AutomatableModel?
			{
				class FloatModel *m_floatModel;
				class IntModel *m_intModel;
				class BoolModel *m_boolModel;
			} m_connectedModel;
			std::string m_id;
			TypedPorts() = default;
			TypedPorts(char type, std::string id) : m_type(type), m_id(id) {}
		};

		//! these are forwarded to the user in the LMMS-internal GUI
		//! inited at plugin initialization time
		//! right after initing, they are added to LinkedModelGroup class
		std::vector<TypedPorts> m_userPorts;
		LmmsPorts(int bufferSize);
		std::unique_ptr<spa::audio::osc_ringbuffer> rb;
	} m_ports;

	void copyModelsToPorts();

	void copyBuffersFromCore(const sampleFrame *buf, unsigned offset, unsigned num, fpp_t frames);
	void copyBuffersToCore(sampleFrame *buf, unsigned offset, unsigned num, fpp_t frames) const;

	void uiExtShow(bool doShow);
	void saveState(QDomDocument &doc, QDomElement &that);
	void loadState(const QDomElement &that);
	void reloadPlugin();

	void handleMidiInputEvent(const class MidiEvent &event,
		const class TimePos &time, f_cnt_t offset);

private:
	friend struct LmmsVisitor;
	friend struct TypeChecker;
	std::atomic_flag m_writeOscInUse;
	bool m_valid = true;
	const DataFile::Types m_settingsType;

	int m_runningNotes[NumKeys];

	// MIDI
	// many things here may be moved into the `Instrument` class
	constexpr const static std::size_t m_maxMidiInputEvents = 1024;
	//! spinlock for the MIDI ringbuffer (for MIDI events going to the plugin)
	std::atomic_flag m_ringLock = ATOMIC_FLAG_INIT;

	//! MIDI ringbuffer (for MIDI events going to the plugin)
	ringbuffer_t<struct MidiInputEvent> m_midiInputBuf;
	//! MIDI ringbuffer reader
	ringbuffer_reader_t<struct MidiInputEvent> m_midiInputReader;

	//! load a file into the plugin, but don't do anything in LMMS
//	void loadFile(const QString &file);

protected:
//	QMutex m_pluginMutex;

	void initPlugin();
	void shutdownPlugin();
};

#endif // SPAPROC_H
