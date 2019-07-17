#ifndef SPAPROC_H
#define SPAPROC_H

#include <QMap>
#include <spa/spa_fwd.h>
#include <spa/audio_fwd.h>

#include "DataFile.h"
#include "LinkedModelGroups.h"
#include "lmms_basics.h"

class AutomatableModel;

class SpaProc : public LinkedModelGroup
{
	friend class SpaViewBase;


	QMap<QString, AutomatableModel *> m_connectedModels;
public:
	SpaProc(Model *parent, const spa::descriptor* desc, std::size_t curProc,
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
			TypedPorts() = default;
			TypedPorts(char type) : m_type(type) {}
			//! Should not be called, but QVector can't guarantee it
			TypedPorts(const TypedPorts &) = delete;
		};

		//! these are forwarded to the user in the LMMS-internal GUI
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
private:
	friend struct LmmsVisitor;
	friend struct TypeChecker;
	bool m_valid = true;
	const DataFile::Types m_settingsType;

	//! load a file into the plugin, but don't do anything in LMMS
//	void loadFile(const QString &file);

protected:
//	QMutex m_pluginMutex;

	void initPlugin();
	void shutdownPlugin();
};

#endif // SPAPROC_H
