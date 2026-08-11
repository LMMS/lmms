#ifndef LMMS_AGENT_CONTROL_H
#define LMMS_AGENT_CONTROL_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QSet>
#include <QStringList>
#include <QTimer>

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>

#include "AutomationClip.h"
#include "Controller.h"
#include "EnvelopeAndLfoParameters.h"
#include "InstrumentFunctions.h"
#include "InstrumentSoundShaping.h"
#include "Metronome.h"
#include "MidiEventProcessor.h"
#include "MidiPort.h"
#include "Mixer.h"
#include "Note.h"
#include "PatternStore.h"
#include "RenderManager.h"
#include "Sample.h"
#include "TimePos.h"
#include "Track.h"
#include "ToolPlugin.h"

class QObject;
class QWidget;

namespace lmms
{

class EffectChain;
class EffectControls;
class InstrumentTrack;
class MidiClip;
class SampleTrack;
namespace gui
{
class AgentControlView;
}

class AgentControlService : public QObject
{
	Q_OBJECT
public:
	static AgentControlService* instance();
	~AgentControlService() override;

	QString handleCommand(const QString& text);
	QString handleJson(const QJsonObject& obj);
	QJsonObject handleRequest(const QJsonObject& obj);

signals:
	void logMessage(const QString& msg);
	void commandResult(const QString& msg);

private slots:
	void onNewConnection();
	void onSocketReady();
	void onSocketClosed();
	void drainCaptureQueue();

private:
	struct Snapshot
	{
		QString id;
		QString label;
		QJsonObject state;
		int actionCounter = 0;
	};

	// ---- v2: MIDI note capture ---------------------------------------------
	struct CapturedEvent
	{
		quint8 type = 0;    // MidiEvent::EventType
		quint8 key = 0;
		quint8 velocity = 0;
		qint64 ms = 0;      // monotonic timestamp relative to capture start
	};

	class CaptureProcessor : public MidiEventProcessor
	{
	public:
		explicit CaptureProcessor(AgentControlService* service) : m_service(service) {}
		void processInEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset) override;
		void processOutEvent(const MidiEvent& event, const TimePos& time, f_cnt_t offset) override {}
	private:
		AgentControlService* m_service;
	};

	// ---- v2: render state ---------------------------------------------------
	struct RenderJob
	{
		QString id;
		RenderManager* manager = nullptr;
		QString outputPath;
		int progress = 0;
		bool done = false;
		bool cancelled = false;
		bool restoreExportSettings = false;
		QString error;
	};

	AgentControlService();

	QString dispatchCommandText(const QString& rawText);
	QStringList tokenizeCommand(const QString& rawText) const;
	bool isUnknownResponse(const QString& response) const;
	bool isFamiliarIntentText(const QString& rawText) const;
	bool applyHeuristicMappings(const QString& rawText, QString& mappedCommand, QString& reason) const;
	bool resolveWithOllama(
		const QString& rawText,
		QString& mappedCommand,
		QString& intent,
		QJsonObject& arguments,
		QString& riskLevel,
		double& confidence,
		QString& reason,
		QString& error ) const;
	bool maybeRunTextAgentFallback(const QString& rawText, QString& result, QString& error);
	QString inferIntentForCommand(const QString& commandText) const;
	QString normalizedRiskForIntent(const QString& intent) const;
	bool isIntentEnabled(const QString& intent) const;
	bool commandNeedsConfirmation(
		const QString& intent,
		const QString& commandText,
		const QString& riskHint = QString() ) const;
	bool isConfirmationUtterance(const QString& rawText) const;
	QString executeWithSafetyGate(
		const QString& commandText,
		const QString& source,
		double confidence,
		const QString& reason,
		const QString& riskHint = QString() );
	void emitTrace(const QString& stage, const QJsonObject& payload = QJsonObject());

	QString dispatchTokens(const QStringList& tokens, const QString& rawText);
	QJsonObject dispatchTool(const QString& toolName, const QJsonObject& args);
	QJsonObject projectStateObject() const;
	QJsonObject trackObject(const Track* track, int index) const;
	QJsonArray trackArray() const;
	QJsonArray listPatternArray() const;
	QJsonObject diffState(const QJsonObject& before, const QJsonObject& after) const;
	QJsonObject successResponse(const QJsonObject& result = QJsonObject(),
		const QJsonObject& stateDelta = QJsonObject(),
		const QJsonArray& warnings = QJsonArray()) const;
	QJsonObject errorResponse(const QString& errorCode, const QString& errorMessage,
		const QJsonArray& warnings = QJsonArray()) const;
	QString trackTypeName(Track::Type type) const;
	Track* resolveTrackRef(const QJsonObject& args) const;
	InstrumentTrack* resolveInstrumentTrack(const QJsonObject& args) const;
	SampleTrack* resolveSampleTrack(const QJsonObject& args) const;
	QString resolveInstrumentPlugin(const QString& pluginName, QString& displayName) const;
	QString resolveEffectPlugin(const QString& effectName, QString& displayName) const;
	QString toCommandResponseText(const QJsonObject& response) const;
	QJsonArray effectArrayForTrack(Track* track) const;
	QJsonArray availableWindows() const;
	QJsonArray availableTools() const;
	QJsonArray availableInstruments() const;
	QJsonArray availableEffects() const;
	QJsonArray searchProjectAudio(const QString& query) const;
	bool createSnapshot(const QString& label, QJsonObject& result, QString& error);
	bool rollbackSnapshot(const QString& snapshotId, QJsonObject& result, QString& error);
	bool undoLastAction(QJsonObject& result, QString& error);
	bool diffSinceSnapshot(const QString& snapshotId, QJsonObject& result, QString& error) const;
	bool loadSampleToTrack(const QString& samplePath, const QString& trackName, QJsonObject& result, QString& error);
	bool setTempoValue(int tempo, QJsonObject& result, QString& error);
	bool renameTrack(const QString& trackName, const QString& newName, QJsonObject& result, QString& error);
	bool selectTrack(const QString& trackName, QJsonObject& result, QString& error);
	bool setTrackMute(const QString& trackName, bool mute, QJsonObject& result, QString& error);
	bool setTrackSolo(const QString& trackName, bool solo, QJsonObject& result, QString& error);
	bool createPatternClip(const QJsonObject& args, QJsonObject& result, QString& error);
	bool addNotesToPattern(const QJsonObject& args, QJsonObject& result, QString& error);
	bool addStepsToPattern(const QJsonObject& args, QJsonObject& result, QString& error);
	int trackIndex(const Track* track) const;

	bool importFromDownloads(const QString& fileName, QString& error);
	bool importAudioFile(const QString& path, QString& error);
	bool importProjectFile(const QString& path, QString& error);
	bool addKickPattern(QString& error);
	bool addSnarePattern(QString& error);
	bool addHiHatPattern(QString& error);
	bool addCrashPattern(QString& error);
	bool addRidePattern(QString& error);

	bool createTrack(Track::Type type, QString& result, QString& error);
	bool createInstrumentTrack(const QString& pluginName, QString& result, QString& error);
	bool handleSlicerWorkflow(const QString& rawText, const QStringList& tokens, QString& result, QString& error);
	bool ensureSlicerTrack(InstrumentTrack*& track, bool createIfMissing, QString& error);
	bool focusInstrumentTrackWindow(InstrumentTrack* track, QString& error);
	bool loadFileIntoSlicer(const QString& fileQuery, QString& result, QString& error);
	bool sliceSlicerEqual(int segments, QString& result, QString& error);
	bool sliceSlicerByTransients(QString& result, QString& error);
	bool showWindowCommand(const QString& windowName, QString& result, QString& error);
	bool showToolCommand(const QString& toolName, QString& result, QString& error);
	bool newProject(QString& result, QString& error);
	bool openProject(const QString& path, QString& result, QString& error);
	void scheduleDeferredOpenProject(const QString& fullPath);
	void processDeferredOpenProject();
	bool saveProject(QString& result, QString& error);
	bool saveProjectAs(const QString& path, QString& result, QString& error);
	bool addEffectToTrack(const QString& effectName, const QString& trackName, QString& result, QString& error);
	bool removeEffectFromTrack(const QString& effectName, const QString& trackName, QString& result, QString& error);

	Track* findTrackByName(const QString& trackName) const;
	Track* findLastTrackOfTypes(const QList<Track::Type>& types) const;
	InstrumentTrack* findInstrumentTrack(const QString& trackName) const;
	InstrumentTrack* findLastSlicerTrack() const;
	SampleTrack* createSampleTrack(const QString& name) const;
	EffectChain* effectChainForTrack(Track* track) const;

	bool addSampleClip(SampleTrack* track, const QString& samplePath, int tickPos);
	QString extractAudioQuery(const QString& rawText, const QStringList& tokens) const;
	QString resolveDownloadsFile(const QString& fileName) const;
	QString resolveDownloadsAudioQuery(const QString& query) const;
	QString defaultKickSample() const;
	QString defaultSnareSample() const;
	QString defaultHiHatSample() const;
	QString defaultCrashSample() const;
	QString defaultRideSample() const;
	QString canonicalPath(const QString& path) const;
	QString joinTokens(const QStringList& tokens, int startIndex) const;
	QString normalizeName(const QString& text) const;

	// ---- v2 tool surface -----------------------------------------------------

	QJsonObject dispatchV2Tool( const QString& tool, const QJsonObject& args );

	// domain dispatch entries (one per translation unit)
	std::optional<QJsonObject> dispatchProjectTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchArrangementTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchNoteTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchPatternTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchSampleTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchInstrumentTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchSoundTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchEffectTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchMixerTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchAutomationTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchControllerTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchRenderTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchRecordTool( const QString& tool, const QJsonObject& args );
	std::optional<QJsonObject> dispatchMiscTool( const QString& tool, const QJsonObject& args );

	// shared v2 helpers
	AutomatableModel* resolveModelAddress( const QString& address, QString& canonicalAddress, QString& error ) const;
	AutomatableModel* modelForTrackPath( InstrumentTrack* track, const QStringList& path, QString& error ) const;
	QJsonObject describeModel( AutomatableModel* model, const QString& address );
	QJsonObject describeModelsForTrack( InstrumentTrack* track, const QString& trackName );
	MixerChannel* resolveMixerChannelRef( const QString& ref, QString& error ) const;
	Effect* findEffectInChain( EffectChain* chain, const QString& effectName ) const;
	AutomatableModel* findParamByName( EffectControls* controls, const QString& paramName ) const;
	MidiClip* resolveMidiClip( const QJsonObject& args, QString& error ) const;
	AutomationClip* resolveAutomationClipRef( const QString& clipRef, QString& error ) const;
	QString automationClipRef( AutomationClip* clip ) const;
	InstrumentTrack* resolveInstrumentTrackOrLast( const QJsonObject& args, QString& error ) const;
	SampleTrack* resolveSampleTrackOrLast( const QJsonObject& args, QString& error ) const;
	bool setModelValue( AutomatableModel* model, double value, QString& error );
	TimePos timePosFromArgs( const QJsonObject& args, bool& ok ) const;
	bool startRender( const OutputSettings& settings, const QString& outputPath,
		bool tracksMode, RenderJob& job, QString& error );

	// v2 state members
	RenderJob m_renderJob;
	CaptureProcessor m_captureProcessor{ this };
	std::unique_ptr<MidiPort> m_capturePort;
	std::mutex m_captureMutex;
	QVector<CapturedEvent> m_captureQueue;
	bool m_captureActive = false;
	qint64 m_captureStartMs = 0;
	TimePos m_captureStartPos;
	InstrumentTrack* m_captureTrack = nullptr;
	int m_captureClipIndex = -1;
	int m_captureQuantize = 0;
	QHash<int, tick_t> m_capturePendingNotes;
	QTimer m_captureDrainTimer;

	QTcpServer m_server;
	QSet<QTcpSocket*> m_clients;
	QHash<QTcpSocket*, QByteArray> m_readBuffers;
	QHash<QString, Snapshot> m_snapshots;
	int m_snapshotCounter = 0;
	int m_actionCounter = 0;
	bool m_projectTransitionQueued = false;
	QString m_deferredOpenProjectPath;
	int m_deferredOpenProjectRetries = 0;
	QString m_selectedTrackName;
	QString m_lastImportedAudioPath;
	QString m_lastLoadedInstrument;
	QString m_pendingConfirmationCommand;
	QString m_pendingConfirmationIntent;
	qint64 m_pendingConfirmationExpiresMs = 0;
};

class AgentControlPlugin : public ToolPlugin
{
	Q_OBJECT
public:
	AgentControlPlugin();
	~AgentControlPlugin() override;

	static AgentControlService* service();

	QString nodeName() const override;
	void saveSettings(QDomDocument&, QDomElement&) override;
	void loadSettings(const QDomElement&) override;
	gui::PluginView* instantiateView(QWidget*) override;

	QString handleCommand(const QString& text);
	QString handleJson(const QJsonObject& obj);

signals:
	void logMessage(const QString& msg);
	void commandResult(const QString& msg);
};

} // namespace lmms

#endif // LMMS_AGENT_CONTROL_H
