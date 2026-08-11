#include "AgentControl.h"

#include <QDateTime>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpressionMatch>
#include <QMetaObject>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>
#include <QVector>

#include "ConfigManager.h"
#include "Clip.h"
#include "Effect.h"
#include "EffectChain.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "ImportFilter.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "LmmsCommonMacros.h"
#include "MainWindow.h"
#include "MidiClip.h"
#include "Note.h"
#include "PluginFactory.h"
#include "ProjectJournal.h"
#include "SongEditor.h"
#include "SampleClip.h"
#include "SampleTrack.h"
#include "Song.h"
#include "TimePos.h"
#include "Track.h"
#include "InstrumentTrackView.h"
#include "InstrumentTrackWindow.h"
#include "plugin_export.h"

#include "AgentControlView.h"

#include <set>

namespace lmms
{

namespace
{

struct IntentProfile
{
	QString intent;
	QString risk;
	QString capabilityKey;
	QStringList aliases;
};

QString normalizeManifestToken( const QString& value )
{
	QString normalized = value.toLower();
	normalized.replace( QRegularExpression( R"([^a-z0-9])" ), QString() );
	return normalized;
}

QList<IntentProfile> defaultIntentProfiles()
{
	return
	{
		{ QStringLiteral( "project.new" ), QStringLiteral( "confirm" ), QStringLiteral( "PROJECT_NEW" ), { QStringLiteral( "newproject" ) } },
		{ QStringLiteral( "project.open" ), QStringLiteral( "confirm" ), QStringLiteral( "PROJECT_OPEN" ), { QStringLiteral( "openproject" ) } },
		{ QStringLiteral( "project.save" ), QStringLiteral( "safe" ), QStringLiteral( "PROJECT_SAVE" ), { QStringLiteral( "saveproject" ), QStringLiteral( "saveprojectas" ) } },
		{ QStringLiteral( "project.export" ), QStringLiteral( "confirm" ), QStringLiteral( "PROJECT_EXPORT" ), { QStringLiteral( "exportsong" ), QStringLiteral( "rendersong" ), QStringLiteral( "exporttracks" ) } },
		{ QStringLiteral( "window.open" ), QStringLiteral( "safe" ), QStringLiteral( "WINDOW_OPEN" ), { QStringLiteral( "opensongeditor" ), QStringLiteral( "openpianoroll" ), QStringLiteral( "openautomationeditor" ), QStringLiteral( "openmixer" ), QStringLiteral( "showwindow" ) } },
		{ QStringLiteral( "track.create" ), QStringLiteral( "safe" ), QStringLiteral( "TRACK_CREATE" ), { QStringLiteral( "createsampletrack" ), QStringLiteral( "createinstrumenttrack" ), QStringLiteral( "createautomationtrack" ), QStringLiteral( "createpatterntrack" ) } },
		{ QStringLiteral( "track.state" ), QStringLiteral( "safe" ), QStringLiteral( "TRACK_STATE" ), { QStringLiteral( "mutetrack" ), QStringLiteral( "solotrack" ), QStringLiteral( "unmutetrack" ), QStringLiteral( "unsolotrack" ) } },
		{ QStringLiteral( "slicer.open" ), QStringLiteral( "safe" ), QStringLiteral( "SLICER_OPEN" ), { QStringLiteral( "openslicer" ), QStringLiteral( "showslicer" ) } },
		{ QStringLiteral( "slicer.import" ), QStringLiteral( "safe" ), QStringLiteral( "SLICER_IMPORT" ), { QStringLiteral( "intoslicer" ), QStringLiteral( "loadslicer" ) } },
		{ QStringLiteral( "slicer.split.equal" ), QStringLiteral( "safe" ), QStringLiteral( "SLICER_SPLIT_EQUAL" ), { QStringLiteral( "equalsegments" ), QStringLiteral( "equalslices" ), QStringLiteral( "splitinto" ) } },
		{ QStringLiteral( "slicer.split.transient" ), QStringLiteral( "safe" ), QStringLiteral( "SLICER_SPLIT_TRANSIENT" ), { QStringLiteral( "transientsegments" ), QStringLiteral( "transientslices" ) } },
		{ QStringLiteral( "transport.play" ), QStringLiteral( "safe" ), QStringLiteral( "TRANSPORT_PLAY" ), { QStringLiteral( "play" ), QStringLiteral( "startplayback" ), QStringLiteral( "resumeplayback" ) } },
		{ QStringLiteral( "transport.pause" ), QStringLiteral( "safe" ), QStringLiteral( "TRANSPORT_PAUSE" ), { QStringLiteral( "pause" ), QStringLiteral( "pauseplayback" ) } },
		{ QStringLiteral( "transport.stop" ), QStringLiteral( "safe" ), QStringLiteral( "TRANSPORT_STOP" ), { QStringLiteral( "stop" ), QStringLiteral( "stopplayback" ) } },
		{ QStringLiteral( "tempo.set" ), QStringLiteral( "safe" ), QStringLiteral( "TEMPO_SET" ), { QStringLiteral( "settempo" ), QStringLiteral( "bpm" ), QStringLiteral( "raisetempo" ), QStringLiteral( "lowertempo" ) } },
		{ QStringLiteral( "pattern.rhythm" ), QStringLiteral( "safe" ), QStringLiteral( "PATTERN_RHYTHM" ), { QStringLiteral( "addkick" ), QStringLiteral( "addsnare" ), QStringLiteral( "add808" ), QStringLiteral( "addhihat" ), QStringLiteral( "addhats" ), QStringLiteral( "addcrash" ), QStringLiteral( "addride" ), QStringLiteral( "addcymbal" ) } },
		{ QStringLiteral( "instrument.load" ), QStringLiteral( "safe" ), QStringLiteral( "INSTRUMENT_LOAD" ), { QStringLiteral( "loadinstrument" ), QStringLiteral( "newinstrument" ), QStringLiteral( "addpiano" ) } },
		{ QStringLiteral( "mixer.effect.add" ), QStringLiteral( "safe" ), QStringLiteral( "MIXER_EFFECT_ADD" ), { QStringLiteral( "addeffect" ) } },
		{ QStringLiteral( "mixer.effect.remove" ), QStringLiteral( "confirm" ), QStringLiteral( "MIXER_EFFECT_REMOVE" ), { QStringLiteral( "removeeffect" ) } },
		{ QStringLiteral( "undo" ), QStringLiteral( "confirm" ), QStringLiteral( "UNDO" ), { QStringLiteral( "undo" ) } }
	};
}

bool loadIntentProfilesFromManifest( const QString& manifestPath, QList<IntentProfile>& profiles, QString& error )
{
	QFile file( manifestPath );
	if( !file.open( QIODevice::ReadOnly ) )
	{
		error = QObject::tr( "could not open %1" ).arg( manifestPath );
		return false;
	}

	QJsonParseError parseError{};
	const QJsonDocument doc = QJsonDocument::fromJson( file.readAll(), &parseError );
	if( parseError.error != QJsonParseError::NoError || !doc.isObject() )
	{
		error = QObject::tr( "invalid JSON (%1)" ).arg( parseError.errorString() );
		return false;
	}

	const QJsonArray intents = doc.object().value( QStringLiteral( "intents" ) ).toArray();
	if( intents.isEmpty() )
	{
		error = QObject::tr( "missing intents array" );
		return false;
	}

	QList<IntentProfile> loaded;
	for( const QJsonValue& value : intents )
	{
		if( !value.isObject() )
		{
			continue;
		}
		const QJsonObject obj = value.toObject();
		const QString intent = obj.value( QStringLiteral( "intent" ) ).toString().trimmed();
		if( intent.isEmpty() )
		{
			continue;
		}

		QString risk = obj.value( QStringLiteral( "risk_level" ) ).toString().trimmed().toLower();
		if( risk != QStringLiteral( "confirm" ) )
		{
			risk = QStringLiteral( "safe" );
		}
		const QString capability = obj.value( QStringLiteral( "capability_flag" ) ).toString().trimmed();
		QStringList aliases;
		for( const QJsonValue& alias : obj.value( QStringLiteral( "aliases" ) ).toArray() )
		{
			const QString normalizedAlias = normalizeManifestToken( alias.toString() );
			if( !normalizedAlias.isEmpty() )
			{
				aliases << normalizedAlias;
			}
		}
		aliases.removeDuplicates();
		loaded.push_back( IntentProfile{ intent, risk, capability, aliases } );
	}

	if( loaded.isEmpty() )
	{
		error = QObject::tr( "no valid intent entries found" );
		return false;
	}
	profiles = loaded;
	return true;
}

bool validateLlmInterpretationObject( const QJsonObject& obj, QString& error )
{
	const auto hasType = [&obj]( const QString& key, QJsonValue::Type type )
	{
		return obj.contains( key ) && obj.value( key ).type() == type;
	};

	if( !hasType( QStringLiteral( "familiar" ), QJsonValue::Bool ) )
	{
		error = QObject::tr( "missing familiar bool" );
		return false;
	}
	if( !obj.value( QStringLiteral( "familiar" ) ).toBool( false ) )
	{
		error = QObject::tr( "Ollama marked command as unfamiliar" );
		return false;
	}
	if( !hasType( QStringLiteral( "intent" ), QJsonValue::String ) )
	{
		error = QObject::tr( "missing intent string" );
		return false;
	}
	if( !hasType( QStringLiteral( "command" ), QJsonValue::String ) )
	{
		error = QObject::tr( "missing command string" );
		return false;
	}
	if( !hasType( QStringLiteral( "arguments" ), QJsonValue::Object ) )
	{
		error = QObject::tr( "missing arguments object" );
		return false;
	}
	if( !hasType( QStringLiteral( "confidence" ), QJsonValue::Double ) )
	{
		error = QObject::tr( "missing confidence number" );
		return false;
	}
	const double confidence = obj.value( QStringLiteral( "confidence" ) ).toDouble( -1.0 );
	if( confidence < 0.0 || confidence > 1.0 )
	{
		error = QObject::tr( "confidence out of range" );
		return false;
	}
	if( !hasType( QStringLiteral( "risk_level" ), QJsonValue::String ) )
	{
		error = QObject::tr( "missing risk_level string" );
		return false;
	}
	const QString risk = obj.value( QStringLiteral( "risk_level" ) ).toString().trimmed().toLower();
	if( risk != QStringLiteral( "safe" ) && risk != QStringLiteral( "confirm" ) )
	{
		error = QObject::tr( "invalid risk_level value" );
		return false;
	}
	if( !hasType( QStringLiteral( "reason" ), QJsonValue::String ) )
	{
		error = QObject::tr( "missing reason string" );
		return false;
	}
	return true;
}

const QList<IntentProfile>& intentProfiles()
{
	static QList<IntentProfile> kProfiles;
	static bool initialized = false;
	if( initialized )
	{
		return kProfiles;
	}

	initialized = true;
	kProfiles = defaultIntentProfiles();

	QString manifestPath = qEnvironmentVariable( "LMMS_COMMAND_MANIFEST" ).trimmed();
	if( manifestPath.isEmpty() )
	{
		const QString localRelative = QStringLiteral( "lmmsagent/integrations/lmms/AgentControl/command_manifest.v3.json" );
		const QString cwdRelative = QDir::current().absoluteFilePath( localRelative );
		if( QFileInfo::exists( cwdRelative ) )
		{
			manifestPath = cwdRelative;
		}
		else
		{
			const QString v2Relative = QStringLiteral( "lmmsagent/integrations/lmms/AgentControl/command_manifest.v2.json" );
			const QString v2Path = QDir::current().absoluteFilePath( v2Relative );
			if( QFileInfo::exists( v2Path ) )
			{
				manifestPath = v2Path;
			}
		}
	}

	if( !manifestPath.isEmpty() )
	{
		QString manifestError;
		QList<IntentProfile> loadedProfiles;
		if( loadIntentProfilesFromManifest( manifestPath, loadedProfiles, manifestError ) )
		{
			kProfiles = loadedProfiles;
		}
		else
		{
			qWarning() << "AgentControl: failed to load command manifest" << manifestPath << ":" << manifestError;
		}
	}

	return kProfiles;
}

QString capabilityEnvForIntent( const QString& intent )
{
	for( const auto& profile : intentProfiles() )
	{
		if( intent == profile.intent )
		{
			if( !profile.capabilityKey.trimmed().isEmpty() )
			{
				return QStringLiteral( "LMMS_CAP_" ) + profile.capabilityKey;
			}
			break;
		}
	}

	QString normalized = intent.toUpper();
	normalized.replace( QRegularExpression( R"([^A-Z0-9]+)" ), QStringLiteral( "_" ) );
	return QStringLiteral( "LMMS_CAP_" ) + normalized;
}

} // namespace

extern "C"
{
Plugin::Descriptor PLUGIN_EXPORT AgentControl_plugin_descriptor =
{
	LMMS_STRINGIFY( PLUGIN_NAME ),
	"Agent Control",
	QT_TRANSLATE_NOOP( "PluginBrowser", "Voice/text command bridge for LMMS" ),
	"codex",
	0x0100,
	Plugin::Type::Tool,
	nullptr,
	nullptr,
	nullptr,
};

PLUGIN_EXPORT Plugin * lmms_plugin_main( Model *, void * )
{
	return new AgentControlPlugin;
}
}

AgentControlService* AgentControlService::instance()
{
	static AgentControlService* service = nullptr;
	if( service == nullptr )
	{
		service = new AgentControlService;
		if( auto *app = QCoreApplication::instance() )
		{
			service->setParent( app );
		}
	}
	return service;
}

AgentControlService::AgentControlService()
{
	connect( &m_server, &QTcpServer::newConnection, this, &AgentControlService::onNewConnection );

	if( !m_server.listen( QHostAddress::LocalHost, 7777 ) )
	{
		emit logMessage( tr( "IPC server failed: %1" ).arg( m_server.errorString() ) );
	}
	else
	{
		emit logMessage( tr( "IPC listening on 127.0.0.1:%1" ).arg( m_server.serverPort() ) );
	}
}

AgentControlService::~AgentControlService()
{
	for( auto *client : m_clients )
	{
		if( client != nullptr )
		{
			client->disconnect( this );
			client->close();
			client->deleteLater();
		}
	}
	m_clients.clear();
	m_readBuffers.clear();
	m_server.close();
}

AgentControlPlugin::AgentControlPlugin() :
	ToolPlugin( &AgentControl_plugin_descriptor, nullptr )
{
	auto *svc = service();
	connect( svc, &AgentControlService::logMessage, this, &AgentControlPlugin::logMessage );
	connect( svc, &AgentControlService::commandResult, this, &AgentControlPlugin::commandResult );
}

AgentControlPlugin::~AgentControlPlugin() = default;

AgentControlService* AgentControlPlugin::service()
{
	return AgentControlService::instance();
}

QString AgentControlPlugin::nodeName() const
{
	return AgentControl_plugin_descriptor.name;
}

void AgentControlPlugin::saveSettings( QDomDocument&, QDomElement& )
{
}

void AgentControlPlugin::loadSettings( const QDomElement& )
{
}

gui::PluginView* AgentControlPlugin::instantiateView( QWidget* )
{
	return new gui::AgentControlView( this );
}

QString AgentControlPlugin::handleCommand( const QString& rawText )
{
	return service()->handleCommand( rawText );
}

QString AgentControlPlugin::handleJson( const QJsonObject& obj )
{
	return service()->handleJson( obj );
}

QString AgentControlService::handleCommand( const QString& rawText )
{
	const QString trimmed = rawText.trimmed();
	if( trimmed.isEmpty() )
	{
		return tr( "No command provided" );
	}

	const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
	if( !m_pendingConfirmationCommand.isEmpty() )
	{
		if( nowMs > m_pendingConfirmationExpiresMs )
		{
			emitTrace( "confirm_expired", QJsonObject
			{
				{ "intent", m_pendingConfirmationIntent },
				{ "command", m_pendingConfirmationCommand }
			} );
			m_pendingConfirmationCommand.clear();
			m_pendingConfirmationIntent.clear();
			m_pendingConfirmationExpiresMs = 0;
		}
		else
		{
			const QString normalized = normalizeName( trimmed );
			if( isConfirmationUtterance( trimmed ) )
			{
				const QString command = m_pendingConfirmationCommand;
				const QString intent = m_pendingConfirmationIntent;
				m_pendingConfirmationCommand.clear();
				m_pendingConfirmationIntent.clear();
				m_pendingConfirmationExpiresMs = 0;
				const QString confirmedResult = dispatchCommandText( command );
				emitTrace( "confirm_executed", QJsonObject
				{
					{ "intent", intent },
					{ "command", command },
					{ "result", confirmedResult }
				} );
				return tr( "Confirmed \"%1\". %2" ).arg( command, confirmedResult );
			}
			if( normalized == "no" || normalized == "cancel" || normalized == "stop" )
			{
				m_pendingConfirmationCommand.clear();
				m_pendingConfirmationIntent.clear();
				m_pendingConfirmationExpiresMs = 0;
				emitTrace( "confirm_cancelled" );
				return tr( "Cancelled pending command." );
			}
			if( !isFamiliarIntentText( trimmed ) )
			{
				return tr( "Awaiting confirmation. Say \"yes\" to run \"%1\" or \"cancel\"." )
					.arg( m_pendingConfirmationCommand );
			}

			// Override pending confirmation when a new command intent arrives.
			m_pendingConfirmationCommand.clear();
			m_pendingConfirmationIntent.clear();
			m_pendingConfirmationExpiresMs = 0;
		}
	}

	// Multi-step chaining for direct text/socket commands.
	// Voice mode already performs chain splitting before dispatch, but this keeps
	// the same behavior available across all command entry points.
	QStringList chainedSegments;
	{
		QString chain = trimmed;
		chain.replace(
			QRegularExpression( R"(\b(?:and then|then|after that|next)\b)", QRegularExpression::CaseInsensitiveOption ),
			QStringLiteral( "|" ) );
		chain.replace(
			QRegularExpression(
				R"(\band\s+(?=(?:open|show|new|create|make|import|load|set|add|remove|mute|solo|undo|divide|split|slice|raise|lower|increase|decrease|save|export|play|pause|stop)\b))",
				QRegularExpression::CaseInsensitiveOption ),
			QStringLiteral( "|" ) );
		chain.replace( QRegularExpression( R"([,;])" ), QStringLiteral( "|" ) );
		chainedSegments = chain.split( '|', Qt::SkipEmptyParts );
	}
	if( chainedSegments.size() > 1 )
	{
		QStringList stepResults;
		stepResults.reserve( chainedSegments.size() );
		for( const QString& segment : chainedSegments )
		{
			const QString step = segment.trimmed();
			if( step.isEmpty() )
			{
				continue;
			}
			stepResults << handleCommand( step );
		}
		if( !stepResults.isEmpty() )
		{
			return stepResults.join( QStringLiteral( ". " ) );
		}
	}

	struct Candidate
	{
		QString command;
		QString intent;
		QString source;
		QString reason;
		QString risk;
		double confidence = 0.0;
		bool viable = false;
		bool exact = false;
	};

	const bool familiarRawIntent = isFamiliarIntentText( trimmed );
	Candidate directCandidate;
	directCandidate.command = trimmed;
	directCandidate.intent = inferIntentForCommand( trimmed );
	directCandidate.source = QStringLiteral( "deterministic" );
	directCandidate.confidence = 0.96;
	directCandidate.exact = true;
	directCandidate.viable = familiarRawIntent;

	Candidate mappedCandidate;
	QString mappedCommand;
	QString mapReason;
	if( applyHeuristicMappings( trimmed, mappedCommand, mapReason ) &&
		!mappedCommand.isEmpty() &&
		normalizeName( mappedCommand ) != normalizeName( trimmed ) )
	{
		mappedCandidate.command = mappedCommand;
		mappedCandidate.source = QStringLiteral( "heuristic" );
		mappedCandidate.reason = mapReason;
		mappedCandidate.intent = inferIntentForCommand( mappedCommand );
		mappedCandidate.confidence = 0.80;
		mappedCandidate.viable = isFamiliarIntentText( mappedCommand );
		if( mappedCandidate.viable )
		{
			// If heuristics found a cleaner deterministic command, prefer it over raw text.
			directCandidate.viable = false;
			directCandidate.exact = false;
		}
	}

	Candidate llmCandidate;
	const QString ollamaModel = qEnvironmentVariable( "LMMS_OLLAMA_MODEL" ).trimmed();
	if( !ollamaModel.isEmpty() )
	{
		QString ollamaError;
		QJsonObject llmArgs;
		if( resolveWithOllama(
			trimmed,
			llmCandidate.command,
			llmCandidate.intent,
			llmArgs,
			llmCandidate.risk,
			llmCandidate.confidence,
			llmCandidate.reason,
			ollamaError ) )
		{
			llmCandidate.source = QStringLiteral( "llm" );
			if( llmCandidate.intent.isEmpty() )
			{
				llmCandidate.intent = inferIntentForCommand( llmCandidate.command );
			}
			const double llmConfidenceFloor = qEnvironmentVariable( "LMMS_OLLAMA_CONFIDENCE" ).trimmed().toDouble() > 0.0
				? qEnvironmentVariable( "LMMS_OLLAMA_CONFIDENCE" ).trimmed().toDouble()
				: 0.72;
			llmCandidate.viable = isFamiliarIntentText( llmCandidate.command ) &&
				llmCandidate.confidence >= llmConfidenceFloor;
		}
		else if( !ollamaError.isEmpty() )
		{
			emit logMessage( tr( "Ollama parse skipped: %1" ).arg( ollamaError ) );
		}
	}

	emitTrace( "candidate_summary", QJsonObject
	{
		{ "input", trimmed },
		{ "familiar_raw", familiarRawIntent },
		{ "direct_ok", directCandidate.viable },
		{ "mapped_ok", mappedCandidate.viable },
		{ "llm_ok", llmCandidate.viable },
		{ "llm_confidence", llmCandidate.confidence }
	} );

	Candidate chosen;
	bool hasChosen = false;
	if( directCandidate.viable )
	{
		// Exact deterministic match wins unless it is disabled by capability policy.
		chosen = directCandidate;
		hasChosen = true;
	}
	else if( mappedCandidate.viable )
	{
		chosen = mappedCandidate;
		hasChosen = true;
	}

	if( llmCandidate.viable )
	{
		const double arbitrationMargin = qEnvironmentVariable( "LMMS_HYBRID_MARGIN" ).trimmed().toDouble() > 0.0
			? qEnvironmentVariable( "LMMS_HYBRID_MARGIN" ).trimmed().toDouble()
			: 0.12;
		if( !hasChosen )
		{
			chosen = llmCandidate;
			hasChosen = true;
		}
		else if( !chosen.exact &&
			llmCandidate.confidence >= chosen.confidence + arbitrationMargin &&
			!llmCandidate.reason.trimmed().isEmpty() )
		{
			chosen = llmCandidate;
			hasChosen = true;
		}
	}

	if( hasChosen )
	{
		const QString execution = executeWithSafetyGate(
			chosen.command,
			chosen.source,
			chosen.confidence,
			chosen.reason,
			chosen.risk );
		if( !isUnknownResponse( execution ) )
		{
			return execution;
		}
		emitTrace( "candidate_failed_dispatch", QJsonObject
		{
			{ "input", trimmed },
			{ "chosen_source", chosen.source },
			{ "chosen_command", chosen.command },
			{ "result", execution }
		} );
	}

	// Hard safety gate: do not run planner fallbacks for unrelated speech.
	if( !familiarRawIntent )
	{
		return dispatchCommandText( trimmed );
	}

	QString plannerResult;
	QString plannerError;
	if( maybeRunTextAgentFallback( trimmed, plannerResult, plannerError ) )
	{
		return plannerResult;
	}
	if( !plannerError.isEmpty() )
	{
		emit logMessage( tr( "Text-agent fallback skipped: %1" ).arg( plannerError ) );
	}

	return dispatchCommandText( trimmed );
}

QString AgentControlService::dispatchCommandText( const QString& rawText )
{
	const QStringList tokens = tokenizeCommand( rawText );
	return dispatchTokens( tokens, rawText );
}

QStringList AgentControlService::tokenizeCommand( const QString& rawText ) const
{
	const auto rawParts = rawText.split( ' ', Qt::SkipEmptyParts );
	QStringList parts;
	parts.reserve( rawParts.size() );

	for( QString token : rawParts )
	{
		while( !token.isEmpty() && QStringLiteral( ".,!?;:\"'" ).contains( token.at( 0 ) ) )
		{
			token.remove( 0, 1 );
		}
		while( !token.isEmpty() && QStringLiteral( ".,!?;:\"'" ).contains( token.at( token.size() - 1 ) ) )
		{
			token.chop( 1 );
		}
		if( !token.isEmpty() )
		{
			parts << token;
		}
	}
	return parts;
}

bool AgentControlService::isUnknownResponse( const QString& response ) const
{
	const QString normalized = response.trimmed();
	return normalized.startsWith( "Unknown command:", Qt::CaseInsensitive ) ||
		normalized.startsWith( "Unknown window:", Qt::CaseInsensitive ) ||
		normalized.startsWith( "Unknown instrument:", Qt::CaseInsensitive );
}

bool AgentControlService::isFamiliarIntentText( const QString& rawText ) const
{
	const QString normalized = normalizeName( rawText );
	const QStringList tokens = tokenizeCommand( rawText );
	if( tokens.isEmpty() )
	{
		return false;
	}

	const QSet<QString> simpleCommands =
	{
		"undo", "version", "help", "play", "pause", "stop"
	};
	if( simpleCommands.contains( normalizeName( tokens.first() ) ) )
	{
		return true;
	}

	const QSet<QString> actionKeywords =
	{
		"open", "show", "new", "create", "make", "import", "load", "set", "add", "remove",
		"mute", "unmute", "solo", "unsolo", "undo", "tempo", "bpm", "divide",
		"split", "slice", "raise", "lower", "increase", "decrease", "save", "list",
		"play", "pause", "stop", "resume", "start"
	};
	const QSet<QString> domainKeywords =
	{
		"project", "track", "instrument", "sample", "automation", "pattern", "mixer",
		"piano", "roll", "editor", "song", "slicer", "splicer", "segment", "transient",
		"slice", "effect", "plugin", "window", "tool", "file", "downloads",
		"hihat", "hat", "hats", "crash", "ride", "cymbal", "arpeggiator", "lane"
	};

	bool hasAction = false;
	bool hasDomain = false;
	for( const QString& token : tokens )
	{
		const QString lowered = normalizeName( token );
		if( actionKeywords.contains( lowered ) )
		{
			hasAction = true;
		}
		if( domainKeywords.contains( lowered ) )
		{
			hasDomain = true;
		}
	}
	if( hasAction && hasDomain )
	{
		return true;
	}

	const QStringList keywords =
	{
		"open", "show", "new", "create", "make", "import", "load", "set", "add", "remove",
		"mute", "unmute", "solo", "unsolo", "undo", "tempo", "bpm", "divide",
		"split", "slice", "openslicer", "showslicer", "manualmap", "beginnerhelp",
		"play", "pause", "stop", "resume", "startplayback", "stopplayback",
		"hihat", "crash", "ride", "cymbal", "arpeggiator", "lane"
	};

	for( const QString& keyword : keywords )
	{
		if( normalized.contains( keyword ) )
		{
			return true;
		}
	}
	return false;
}

bool AgentControlService::applyHeuristicMappings(
	const QString& rawText,
	QString& mappedCommand,
	QString& reason ) const
{
	reason.clear();
	mappedCommand = rawText.trimmed();
	if( mappedCommand.isEmpty() )
	{
		return false;
	}

	const auto setReasonIfEmpty = [&reason]( const QString& value )
	{
		if( reason.isEmpty() )
		{
			reason = value;
		}
	};

	auto lower = mappedCommand.toLower();

	// Remove polite wrappers so command verbs are easier to parse.
	bool removedConversationFiller = false;
	const QRegularExpression leadingFiller(
		R"(^(?:(?:hey|hi|yo|okay|ok|please|pls)\s+)*(?:(?:can|could|would)\s+(?:you|u)\s+)?(?:please|pls\s+)?(?:just\s+)?(?:hey\s+)?(?:can|could|would)?\s*(?:\b(?:you|u)\b\s*)?)",
		QRegularExpression::CaseInsensitiveOption );
	for( int i = 0; i < 2; ++i )
	{
		const QString before = mappedCommand;
		mappedCommand.replace( leadingFiller, QString() );
		mappedCommand.replace(
			QRegularExpression( R"(\s+(?:please|pls|thanks|thank\s+you)\s*$)",
				QRegularExpression::CaseInsensitiveOption ),
			QString() );
		mappedCommand = mappedCommand.simplified();
		if( mappedCommand == before )
		{
			break;
		}
		removedConversationFiller = true;
	}
	if( removedConversationFiller )
	{
		setReasonIfEmpty( tr( "removed conversational filler" ) );
	}
	lower = mappedCommand.toLower();

	if( lower.contains( "splicer" ) )
	{
		mappedCommand.replace( QRegularExpression( R"(\bsplicer\b)", QRegularExpression::CaseInsensitiveOption ),
			QStringLiteral( "slicer" ) );
		setReasonIfEmpty( tr( "corrected plugin name typo" ) );
	}

	struct RegexReplacement
	{
		const char* pattern;
		const char* replacement;
		const char* why;
	};
	const RegexReplacement regexReplacements[] =
	{
		{ R"(\bautomaton\b)", "automation", "corrected common typo" },
		{ R"(\btepmo\b)", "tempo", "corrected common typo" },
		{ R"(\binsrument\b)", "instrument", "corrected common typo" },
		{ R"(\btrakc\b)", "track", "corrected common typo" },
		{ R"(\bpattarn\b)", "pattern", "corrected common typo" },
		{ R"(\beditr\b)", "editor", "corrected common typo" }
	};
	for( const auto& replacement : regexReplacements )
	{
		const QString before = mappedCommand;
		mappedCommand.replace(
			QRegularExpression( QString::fromLatin1( replacement.pattern ),
				QRegularExpression::CaseInsensitiveOption ),
			QString::fromLatin1( replacement.replacement ) );
		if( mappedCommand != before )
		{
			setReasonIfEmpty( tr( replacement.why ) );
		}
	}
	const QString beforeArticleDrop = mappedCommand;
	mappedCommand.replace(
		QRegularExpression( R"(\b(create|new|open|show|set|import|load|add|remove|make)\s+(?:a|an)\s+)",
			QRegularExpression::CaseInsensitiveOption ),
		QStringLiteral( "\\1 " ) );
	if( mappedCommand != beforeArticleDrop )
	{
		setReasonIfEmpty( tr( "normalized command wording" ) );
	}

	// Normalize "make ... track" to existing create-track actions.
	if( QRegularExpression( R"(^\s*make\s+.+\btrack\b)",
			QRegularExpression::CaseInsensitiveOption ).match( mappedCommand ).hasMatch() )
	{
		mappedCommand.replace( QRegularExpression( R"(^\s*make\b)", QRegularExpression::CaseInsensitiveOption ),
			QStringLiteral( "create" ) );
		setReasonIfEmpty( tr( "normalized create-track phrasing" ) );
	}

	// Normalize "open the ... window/panel/view" variants.
	const QString normalizedLower = normalizeName( mappedCommand );
	if( normalizedLower.startsWith( "open" ) || normalizedLower.startsWith( "show" ) )
	{
		mappedCommand.replace(
			QRegularExpression( R"(\b(the|window|panel|view)\b)", QRegularExpression::CaseInsensitiveOption ),
			QString() );
		mappedCommand = mappedCommand.simplified();

		const QString openLower = normalizeName( mappedCommand );
		if( openLower.contains( "song" ) && ( openLower.contains( "editor" ) || openLower.contains( "edit" ) ) )
		{
			mappedCommand = QStringLiteral( "open song editor" );
			setReasonIfEmpty( tr( "normalized song editor window name" ) );
		}
		else if( openLower.contains( "automation" ) && openLower.contains( "editor" ) )
		{
			mappedCommand = QStringLiteral( "open automation editor" );
			setReasonIfEmpty( tr( "normalized automation editor window name" ) );
		}
		else if( openLower.contains( "piano" ) && openLower.contains( "roll" ) )
		{
			mappedCommand = QStringLiteral( "open piano roll" );
			setReasonIfEmpty( tr( "normalized piano roll window name" ) );
		}
		else if( openLower.contains( "mixer" ) || openLower.contains( "fxmixer" ) || openLower.contains( "effects" ) )
		{
			mappedCommand = QStringLiteral( "open mixer" );
			setReasonIfEmpty( tr( "normalized mixer window name" ) );
		}
	}
	const QString collapsed = normalizeName( mappedCommand );

	// Convert inferred track-intent statements to explicit commands.
	if( !collapsed.startsWith( "new" ) && !collapsed.startsWith( "create" ) )
	{
		if( collapsed.contains( "sampletrack" ) )
		{
			mappedCommand = QStringLiteral( "create sample track" );
			setReasonIfEmpty( tr( "inferred sample track creation intent" ) );
		}
		else if( collapsed.contains( "instrumenttrack" ) )
		{
			mappedCommand = QStringLiteral( "create instrument track" );
			setReasonIfEmpty( tr( "inferred instrument track creation intent" ) );
		}
		else if( collapsed.contains( "automationtrack" ) )
		{
			mappedCommand = QStringLiteral( "create automation track" );
			setReasonIfEmpty( tr( "inferred automation track creation intent" ) );
		}
		else if( collapsed.contains( "patterntrack" ) )
		{
			mappedCommand = QStringLiteral( "create pattern track" );
			setReasonIfEmpty( tr( "inferred pattern track creation intent" ) );
		}
	}

	if( collapsed.contains( "arpeggiator" ) && collapsed.contains( "lane" ) )
	{
		mappedCommand = QStringLiteral( "create arpeggiator lane" );
		setReasonIfEmpty( tr( "normalized arpeggiator lane phrasing" ) );
	}

	// "lets go 128 bpm" -> "set tempo to 128"
	const QRegularExpression bpmPhrase( R"(\b(\d{2,3})\s*bpm\b)", QRegularExpression::CaseInsensitiveOption );
	const auto bpmMatch = bpmPhrase.match( mappedCommand );
	if( bpmMatch.hasMatch() && !normalizeName( mappedCommand ).contains( "settempo" ) )
	{
		mappedCommand = tr( "set tempo to %1" ).arg( bpmMatch.captured( 1 ) );
		setReasonIfEmpty( tr( "inferred tempo set intent" ) );
	}

	// Normalize common transport phrasing.
	if( QRegularExpression( R"(\b(start|resume)\s+(playback|song)\b)",
			QRegularExpression::CaseInsensitiveOption ).match( mappedCommand ).hasMatch() )
	{
		mappedCommand = QStringLiteral( "play" );
		setReasonIfEmpty( tr( "normalized transport command" ) );
	}
	else if( QRegularExpression( R"(\bpause\s+(playback|song)\b)",
		QRegularExpression::CaseInsensitiveOption ).match( mappedCommand ).hasMatch() )
	{
		mappedCommand = QStringLiteral( "pause" );
		setReasonIfEmpty( tr( "normalized transport command" ) );
	}
	else if( QRegularExpression( R"(\bstop\s+(playback|song)\b)",
		QRegularExpression::CaseInsensitiveOption ).match( mappedCommand ).hasMatch() )
	{
		mappedCommand = QStringLiteral( "stop" );
		setReasonIfEmpty( tr( "normalized transport command" ) );
	}

	// "divide into 16 segments" -> "divide into 16 equal segments"
	if( QRegularExpression( R"(\bdivide\s+into\s+\d+\s+segments?\b)",
			QRegularExpression::CaseInsensitiveOption ).match( mappedCommand ).hasMatch() &&
		!QRegularExpression( R"(\bequal\b)", QRegularExpression::CaseInsensitiveOption ).match( mappedCommand ).hasMatch() )
	{
		mappedCommand.replace(
			QRegularExpression( R"(\bsegments?\b)", QRegularExpression::CaseInsensitiveOption ),
			QStringLiteral( "equal segments" ) );
		setReasonIfEmpty( tr( "assumed equal segments" ) );
	}

	// "from download(s)" is metadata, not part of filename.
	mappedCommand.replace(
		QRegularExpression( R"(\bfrom\s+(?:my\s+)?downloads?(?:\s+folder)?\b)",
			QRegularExpression::CaseInsensitiveOption ),
		QString() );
	mappedCommand = mappedCommand.simplified();

	const QStringList knownTokens =
	{
		"open", "show", "new", "create", "make", "import", "load", "set", "add", "remove",
		"mute", "unmute", "solo", "unsolo", "undo", "tempo", "bpm", "divide", "split",
		"slice", "slicer", "track", "instrument", "sample", "automation", "pattern",
		"mixer", "project", "downloads", "equal", "segments", "transient",
		"hihat", "hat", "hats", "crash", "ride", "cymbal", "arpeggiator", "lane",
		"song", "editor", "piano", "roll", "window", "play", "pause", "stop",
		"start", "resume", "playback"
	};
	auto editDistance = []( const QString& a, const QString& b )
	{
		const int n = a.size();
		const int m = b.size();
		if( n == 0 )
		{
			return m;
		}
		if( m == 0 )
		{
			return n;
		}

		QVector<int> prev( m + 1 );
		QVector<int> curr( m + 1 );
		for( int j = 0; j <= m; ++j )
		{
			prev[j] = j;
		}
		for( int i = 1; i <= n; ++i )
		{
			curr[0] = i;
			for( int j = 1; j <= m; ++j )
			{
				const int cost = ( a[i - 1] == b[j - 1] ) ? 0 : 1;
				curr[j] = qMin( qMin( curr[j - 1] + 1, prev[j] + 1 ), prev[j - 1] + cost );
			}
			prev.swap( curr );
		}
		return prev[m];
	};
	auto isSingleAdjacentTransposition = []( const QString& a, const QString& b )
	{
		if( a.size() != b.size() || a.size() < 2 )
		{
			return false;
		}
		int firstMismatch = -1;
		int secondMismatch = -1;
		for( int i = 0; i < a.size(); ++i )
		{
			if( a[i] != b[i] )
			{
				if( firstMismatch < 0 )
				{
					firstMismatch = i;
				}
				else if( secondMismatch < 0 )
				{
					secondMismatch = i;
				}
				else
				{
					return false;
				}
			}
		}
		if( firstMismatch < 0 || secondMismatch < 0 )
		{
			return false;
		}
		if( secondMismatch != firstMismatch + 1 )
		{
			return false;
		}
		return a[firstMismatch] == b[secondMismatch] && a[secondMismatch] == b[firstMismatch];
	};

	QStringList fuzzyTokens = tokenizeCommand( mappedCommand );
	bool changedByFuzzy = false;
	for( QString& token : fuzzyTokens )
	{
		const QString lowered = normalizeName( token );
		if( lowered.size() < 4 || knownTokens.contains( lowered ) )
		{
			continue;
		}

		QString best;
		int bestDistance = 2;
		for( const QString& known : knownTokens )
		{
			int distance = editDistance( lowered, known );
			if( distance > 1 && isSingleAdjacentTransposition( lowered, known ) )
			{
				distance = 1;
			}
			if( distance < bestDistance )
			{
				best = known;
				bestDistance = distance;
			}
		}
		if( bestDistance <= 1 && !best.isEmpty() )
		{
			token = best;
			changedByFuzzy = true;
		}
	}
	if( changedByFuzzy )
	{
		mappedCommand = fuzzyTokens.join( ' ' );
		if( reason.isEmpty() )
		{
			reason = tr( "applied fuzzy token correction" );
		}
	}

	// Final normalization after fuzzy correction.
	if( QRegularExpression( R"(^\s*make\s+.+\btrack\b)",
			QRegularExpression::CaseInsensitiveOption ).match( mappedCommand ).hasMatch() )
	{
		mappedCommand.replace( QRegularExpression( R"(^\s*make\b)", QRegularExpression::CaseInsensitiveOption ),
			QStringLiteral( "create" ) );
		setReasonIfEmpty( tr( "normalized create-track phrasing" ) );
	}

	mappedCommand = mappedCommand.simplified();

	return normalizeName( mappedCommand ) != normalizeName( rawText );
}

bool AgentControlService::resolveWithOllama(
	const QString& rawText,
	QString& mappedCommand,
	QString& intent,
	QJsonObject& arguments,
	QString& riskLevel,
	double& confidence,
	QString& reason,
	QString& error ) const
{
	mappedCommand.clear();
	intent.clear();
	arguments = QJsonObject();
	riskLevel.clear();
	confidence = 0.0;
	reason.clear();
	error.clear();

	const QString model = qEnvironmentVariable( "LMMS_OLLAMA_MODEL" ).trimmed();
	if( model.isEmpty() )
	{
		error = tr( "LMMS_OLLAMA_MODEL is not set" );
		return false;
	}

	const QString endpoint = qEnvironmentVariable( "LMMS_OLLAMA_URL" ).trimmed().isEmpty()
		? QStringLiteral( "http://127.0.0.1:11434/api/chat" )
		: qEnvironmentVariable( "LMMS_OLLAMA_URL" ).trimmed();

	const QString systemPrompt =
		QStringLiteral(
			"You map noisy LMMS voice commands to the closest executable LMMS command. "
			"Use these command families: open/show/new/create/import/load/set/add/remove/mute/solo/undo/tempo/slicer/save/export. "
			"Return JSON only with this schema: "
			"{\"familiar\":bool,\"intent\":string,\"command\":string,\"arguments\":object,"
			"\"confidence\":number,\"risk_level\":\"safe|confirm\",\"reason\":string}. "
			"If unclear or unrelated, return familiar=false with low confidence and empty command." );

	const QJsonObject payload
	{
		{ "model", model },
		{ "stream", false },
		{ "messages", QJsonArray
			{
				QJsonObject{ { "role", "system" }, { "content", systemPrompt } },
				QJsonObject{ { "role", "user" }, { "content", rawText } }
			}
		}
	};

	QNetworkAccessManager manager;
	QNetworkRequest request{ QUrl( endpoint ) };
	request.setHeader( QNetworkRequest::ContentTypeHeader, QStringLiteral( "application/json" ) );

	QNetworkReply *reply = manager.post( request, QJsonDocument( payload ).toJson( QJsonDocument::Compact ) );
	QEventLoop loop;
	QObject::connect( reply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
	const int timeoutMs = qEnvironmentVariableIntValue( "LMMS_OLLAMA_TIMEOUT_MS" ) > 0
		? qEnvironmentVariableIntValue( "LMMS_OLLAMA_TIMEOUT_MS" )
		: 2500;
	QTimer::singleShot( qMax( 800, timeoutMs ), &loop, &QEventLoop::quit );
	loop.exec();

	if( reply->isRunning() )
	{
		reply->abort();
		reply->deleteLater();
		error = tr( "Ollama timed out" );
		return false;
	}

	if( reply->error() != QNetworkReply::NoError )
	{
		error = reply->errorString();
		reply->deleteLater();
		return false;
	}

	const QByteArray body = reply->readAll();
	reply->deleteLater();
	const QJsonDocument outerDoc = QJsonDocument::fromJson( body );
	if( !outerDoc.isObject() )
	{
		error = tr( "Invalid Ollama response" );
		return false;
	}

	QString content = outerDoc.object().value( "message" ).toObject().value( "content" ).toString().trimmed();
	if( content.isEmpty() )
	{
		error = tr( "Ollama returned empty content" );
		return false;
	}

	// Accept wrapped JSON in markdown fences.
	content.remove( QRegularExpression( R"(^```(?:json)?\s*)", QRegularExpression::CaseInsensitiveOption ) );
	content.remove( QRegularExpression( R"(\s*```$)", QRegularExpression::CaseInsensitiveOption ) );
	const QRegularExpression jsonBlock( R"(\{[\s\S]*\})" );
	const auto match = jsonBlock.match( content );
	if( match.hasMatch() )
	{
		content = match.captured( 0 );
	}

	const QJsonDocument mappedDoc = QJsonDocument::fromJson( content.toUtf8() );
	if( !mappedDoc.isObject() )
	{
		error = tr( "Ollama content was not valid JSON" );
		return false;
	}

	const QJsonObject mappedObj = mappedDoc.object();
	if( !validateLlmInterpretationObject( mappedObj, error ) )
	{
		return false;
	}

	mappedCommand = mappedObj.value( "command" ).toString().trimmed();
	intent = mappedObj.value( "intent" ).toString().trimmed();
	arguments = mappedObj.value( "arguments" ).toObject();
	confidence = mappedObj.value( "confidence" ).toDouble( 0.0 );
	riskLevel = mappedObj.value( "risk_level" ).toString().trimmed().toLower();
	reason = mappedObj.value( "reason" ).toString().trimmed();
	if( mappedCommand.isEmpty() )
	{
		error = tr( "Ollama returned an empty command" );
		return false;
	}
	if( riskLevel != QStringLiteral( "safe" ) && riskLevel != QStringLiteral( "confirm" ) )
	{
		riskLevel = QStringLiteral( "safe" );
	}
	return true;
}

bool AgentControlService::maybeRunTextAgentFallback(
	const QString& rawText,
	QString& result,
	QString& error )
{
	result.clear();
	error.clear();

	const int enabled = qEnvironmentVariableIntValue( "LMMS_TEXT_AGENT_FALLBACK" );
	if( enabled != 1 )
	{
		error = tr( "LMMS_TEXT_AGENT_FALLBACK is disabled" );
		return false;
	}

	const QString scriptPath = qEnvironmentVariable( "LMMS_TEXT_AGENT_SCRIPT" ).trimmed();
	if( scriptPath.isEmpty() )
	{
		error = tr( "LMMS_TEXT_AGENT_SCRIPT is not set" );
		return false;
	}
	if( !QFileInfo::exists( scriptPath ) )
	{
		error = tr( "Text-agent script not found: %1" ).arg( scriptPath );
		return false;
	}

	QProcess process;
	process.start( scriptPath, QStringList{ rawText } );
	if( !process.waitForStarted( 1000 ) )
	{
		error = tr( "Failed to start text-agent script" );
		return false;
	}
	if( !process.waitForFinished( 5000 ) )
	{
		process.kill();
		process.waitForFinished( 500 );
		error = tr( "Text-agent fallback timed out" );
		return false;
	}
	if( process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 )
	{
		error = QString::fromUtf8( process.readAllStandardError() ).trimmed();
		if( error.isEmpty() )
		{
			error = tr( "Text-agent script failed" );
		}
		return false;
	}

	const QString output = QString::fromUtf8( process.readAllStandardOutput() ).trimmed();
	if( output.isEmpty() )
	{
		error = tr( "Text-agent returned empty output" );
		return false;
	}

	QString mappedCommand = output;
	if( output.startsWith( '{' ) )
	{
		const QJsonDocument doc = QJsonDocument::fromJson( output.toUtf8() );
		if( doc.isObject() )
		{
			const QJsonObject obj = doc.object();
			mappedCommand = obj.value( "command" ).toString().trimmed();
			if( mappedCommand.isEmpty() )
			{
				mappedCommand = obj.value( "text" ).toString().trimmed();
			}
		}
	}
	else
	{
		mappedCommand = output.section( '\n', 0, 0 ).trimmed();
	}

	if( mappedCommand.isEmpty() )
	{
		error = tr( "Text-agent output did not contain a command" );
		return false;
	}
	if( !isFamiliarIntentText( mappedCommand ) )
	{
		error = tr( "Planner suggested an unfamiliar command; ignored for safety" );
		return false;
	}

	const QString mappedResult = dispatchCommandText( mappedCommand );
	if( isUnknownResponse( mappedResult ) )
	{
		error = tr( "Planner suggestion was not executable: %1" ).arg( mappedCommand );
		return false;
	}

	result = tr( "Planner interpreted as \"%1\". %2" ).arg( mappedCommand, mappedResult );
	return true;
}

QString AgentControlService::inferIntentForCommand( const QString& commandText ) const
{
	const QString normalized = normalizeName( commandText );
	if( normalized.isEmpty() )
	{
		return {};
	}

	if( normalized == QStringLiteral( "undo" ) )
	{
		return QStringLiteral( "undo" );
	}
	if( normalized.contains( QStringLiteral( "openslicer" ) ) || normalized == QStringLiteral( "slicer" ) )
	{
		return QStringLiteral( "slicer.open" );
	}
	if( normalized.contains( QStringLiteral( "intoslicer" ) ) ||
		( normalized.contains( QStringLiteral( "import" ) ) && normalized.contains( QStringLiteral( "slicer" ) ) ) )
	{
		return QStringLiteral( "slicer.import" );
	}
	if( normalized.contains( QStringLiteral( "transient" ) ) &&
		( normalized.contains( QStringLiteral( "divide" ) ) || normalized.contains( QStringLiteral( "split" ) ) ) )
	{
		return QStringLiteral( "slicer.split.transient" );
	}
	if( ( normalized.contains( QStringLiteral( "divide" ) ) || normalized.contains( QStringLiteral( "split" ) ) ) &&
		normalized.contains( QStringLiteral( "segment" ) ) )
	{
		return QStringLiteral( "slicer.split.equal" );
	}
	if( normalized.startsWith( QStringLiteral( "newproject" ) ) )
	{
		return QStringLiteral( "project.new" );
	}
	if( normalized.startsWith( QStringLiteral( "openproject" ) ) )
	{
		return QStringLiteral( "project.open" );
	}
	if( normalized.startsWith( QStringLiteral( "saveproject" ) ) )
	{
		return QStringLiteral( "project.save" );
	}
	if( normalized.contains( QStringLiteral( "exportsong" ) ) ||
		normalized.contains( QStringLiteral( "rendersong" ) ) ||
		normalized.contains( QStringLiteral( "exporttracks" ) ) )
	{
		return QStringLiteral( "project.export" );
	}
	if( ( normalized.startsWith( QStringLiteral( "open" ) ) || normalized.startsWith( QStringLiteral( "show" ) ) ) &&
		( normalized.contains( QStringLiteral( "editor" ) ) || normalized.contains( QStringLiteral( "mixer" ) ) ||
		  normalized.contains( QStringLiteral( "rack" ) ) || normalized.contains( QStringLiteral( "notes" ) ) ) )
	{
		return QStringLiteral( "window.open" );
	}
	if( normalized.contains( QStringLiteral( "create" ) ) && normalized.contains( QStringLiteral( "track" ) ) )
	{
		return QStringLiteral( "track.create" );
	}
	if( normalized.startsWith( QStringLiteral( "mute" ) ) ||
		normalized.startsWith( QStringLiteral( "unmute" ) ) ||
		normalized.startsWith( QStringLiteral( "solo" ) ) ||
		normalized.startsWith( QStringLiteral( "unsolo" ) ) )
	{
		return QStringLiteral( "track.state" );
	}
	if( normalized.contains( QStringLiteral( "settempo" ) ) ||
		normalized.contains( QStringLiteral( "bpm" ) ) ||
		normalized.contains( QStringLiteral( "raisetempo" ) ) ||
		normalized.contains( QStringLiteral( "lowertempo" ) ) )
	{
		return QStringLiteral( "tempo.set" );
	}
	if( normalized == QStringLiteral( "play" ) || normalized.contains( QStringLiteral( "startplayback" ) ) ||
		normalized.contains( QStringLiteral( "resumeplayback" ) ) )
	{
		return QStringLiteral( "transport.play" );
	}
	if( normalized == QStringLiteral( "pause" ) || normalized.contains( QStringLiteral( "pauseplayback" ) ) )
	{
		return QStringLiteral( "transport.pause" );
	}
	if( normalized == QStringLiteral( "stop" ) || normalized.contains( QStringLiteral( "stopplayback" ) ) )
	{
		return QStringLiteral( "transport.stop" );
	}
	if( normalized.contains( QStringLiteral( "addkick" ) ) ||
		normalized.contains( QStringLiteral( "addsnare" ) ) ||
		normalized.contains( QStringLiteral( "add808" ) ) ||
		normalized.contains( QStringLiteral( "addhihat" ) ) ||
		normalized.contains( QStringLiteral( "addhats" ) ) ||
		normalized.contains( QStringLiteral( "addcrash" ) ) ||
		normalized.contains( QStringLiteral( "addride" ) ) ||
		normalized.contains( QStringLiteral( "addcymbal" ) ) )
	{
		return QStringLiteral( "pattern.rhythm" );
	}
	if( normalized.contains( QStringLiteral( "arpeggiator" ) ) &&
		normalized.contains( QStringLiteral( "lane" ) ) )
	{
		return QStringLiteral( "track.create" );
	}
	if( normalized.contains( QStringLiteral( "loadinstrument" ) ) ||
		normalized.contains( QStringLiteral( "newinstrument" ) ) ||
		normalized.contains( QStringLiteral( "addpiano" ) ) )
	{
		return QStringLiteral( "instrument.load" );
	}
	if( normalized.startsWith( QStringLiteral( "addeffect" ) ) )
	{
		return QStringLiteral( "mixer.effect.add" );
	}
	if( normalized.startsWith( QStringLiteral( "removeeffect" ) ) )
	{
		return QStringLiteral( "mixer.effect.remove" );
	}

	for( const auto& profile : intentProfiles() )
	{
		for( const QString& alias : profile.aliases )
		{
			if( normalized.contains( alias ) )
			{
				return profile.intent;
			}
		}
	}

	return {};
}

QString AgentControlService::normalizedRiskForIntent( const QString& intent ) const
{
	for( const auto& profile : intentProfiles() )
	{
		if( intent == profile.intent )
		{
			return profile.risk;
		}
	}
	return QStringLiteral( "safe" );
}

bool AgentControlService::isIntentEnabled( const QString& intent ) const
{
	if( intent.trimmed().isEmpty() )
	{
		return true;
	}
	const QString envName = capabilityEnvForIntent( intent );
	const QString raw = qEnvironmentVariable( envName.toUtf8().constData() ).trimmed().toLower();
	if( raw.isEmpty() )
	{
		return true;
	}
	return !( raw == QStringLiteral( "0" ) ||
		raw == QStringLiteral( "false" ) ||
		raw == QStringLiteral( "off" ) ||
		raw == QStringLiteral( "disabled" ) );
}

bool AgentControlService::commandNeedsConfirmation(
	const QString& intent,
	const QString& commandText,
	const QString& riskHint ) const
{
	QString risk = riskHint.trimmed().toLower();
	if( risk.isEmpty() )
	{
		risk = normalizedRiskForIntent( intent );
	}
	if( risk == QStringLiteral( "confirm" ) )
	{
		return true;
	}

	const QString normalized = normalizeName( commandText );
	if( normalized.contains( QStringLiteral( "removeeffect" ) ) ||
		normalized.contains( QStringLiteral( "deletetrack" ) ) ||
		normalized.contains( QStringLiteral( "newproject" ) ) ||
		normalized.contains( QStringLiteral( "openproject" ) ) ||
		normalized.contains( QStringLiteral( "saveasdefaulttemplate" ) ) ||
		normalized.contains( QStringLiteral( "export" ) ) )
	{
		return true;
	}
	return false;
}

bool AgentControlService::isConfirmationUtterance( const QString& rawText ) const
{
	const QString normalized = normalizeName( rawText );
	return normalized == QStringLiteral( "yes" ) ||
		normalized == QStringLiteral( "yep" ) ||
		normalized == QStringLiteral( "confirm" ) ||
		normalized == QStringLiteral( "confirmed" ) ||
		normalized == QStringLiteral( "dothething" ) ||
		normalized == QStringLiteral( "doit" ) ||
		normalized == QStringLiteral( "proceed" );
}

QString AgentControlService::executeWithSafetyGate(
	const QString& commandText,
	const QString& source,
	double confidence,
	const QString& reason,
	const QString& riskHint )
{
	const QString intent = inferIntentForCommand( commandText );
	if( !isIntentEnabled( intent ) )
	{
		const QString capability = capabilityEnvForIntent( intent );
		return tr( "Capability disabled for intent \"%1\". Set %2=1 to enable." )
			.arg( intent, capability );
	}

	const double confirmConfidence = qEnvironmentVariable( "LMMS_CONFIRM_CONFIDENCE" ).trimmed().toDouble() > 0.0
		? qEnvironmentVariable( "LMMS_CONFIRM_CONFIDENCE" ).trimmed().toDouble()
		: 0.74;
	const bool lowConfidenceLlm = source == QStringLiteral( "llm" ) && confidence < confirmConfidence;
	const bool needsConfirmation = commandNeedsConfirmation( intent, commandText, riskHint ) || lowConfidenceLlm;
	if( needsConfirmation )
	{
		QString commandForConfirm = commandText;
		if( intent == QStringLiteral( "mixer.effect.remove" ) )
		{
			const QStringList parts = tokenizeCommand( commandText );
			int fromIdx = -1;
			for( int i = 0; i < parts.size(); ++i )
			{
				if( parts[i].compare( QStringLiteral( "from" ), Qt::CaseInsensitive ) == 0 )
				{
					fromIdx = i;
					break;
				}
			}
			if( fromIdx < 0 )
			{
				if( Track *track = findLastTrackOfTypes( { Track::Type::Instrument, Track::Type::Sample } ) )
				{
					commandForConfirm = commandText + QStringLiteral( " from " ) + track->name();
				}
			}
		}

		const int confirmWindowMs = qEnvironmentVariableIntValue( "LMMS_CONFIRM_WINDOW_MS" ) > 0
			? qEnvironmentVariableIntValue( "LMMS_CONFIRM_WINDOW_MS" )
			: 9000;
		m_pendingConfirmationCommand = commandForConfirm;
		m_pendingConfirmationIntent = intent;
		m_pendingConfirmationExpiresMs = QDateTime::currentMSecsSinceEpoch() + qMax( 3000, confirmWindowMs );
		emitTrace( "confirm_required", QJsonObject
		{
			{ "intent", intent },
			{ "command", commandForConfirm },
			{ "source", source },
			{ "confidence", confidence },
			{ "risk", riskHint.isEmpty() ? normalizedRiskForIntent( intent ) : riskHint }
		} );
		return tr( "Confirmation required: \"%1\". Say \"yes\" to execute or \"cancel\"." )
			.arg( commandText );
	}

	const QString result = dispatchCommandText( commandText );
	emitTrace( "command_executed", QJsonObject
	{
		{ "intent", intent },
		{ "source", source },
		{ "command", commandText },
		{ "confidence", confidence },
		{ "result", result }
	} );

	if( source == QStringLiteral( "deterministic" ) )
	{
		return result;
	}

	QString explanation = tr( "Interpreted as \"%1\"" ).arg( commandText );
	if( !reason.trimmed().isEmpty() )
	{
		explanation += tr( " (%1)" ).arg( reason.trimmed() );
	}
	if( confidence > 0.0 )
	{
		explanation += tr( " [confidence %1]" ).arg( QString::number( confidence, 'f', 2 ) );
	}
	return explanation + QStringLiteral( ". " ) + result;
}

void AgentControlService::emitTrace( const QString& stage, const QJsonObject& payload )
{
	const int traceEnabled = qEnvironmentVariableIntValue( "LMMS_VOICE_TRACE" );
	if( traceEnabled != 1 )
	{
		return;
	}

	QJsonObject event = payload;
	event.insert( "stage", stage );
	event.insert( "ts_ms", QString::number( QDateTime::currentMSecsSinceEpoch() ) );
	emit logMessage( QStringLiteral( "TRACE %1" )
		.arg( QString::fromUtf8( QJsonDocument( event ).toJson( QJsonDocument::Compact ) ) ) );
}

QString AgentControlService::handleJson( const QJsonObject& obj )
{
	const QJsonObject response = handleRequest( obj );
	return toCommandResponseText( response );
}

QJsonObject AgentControlService::handleRequest( const QJsonObject& obj )
{
	if( obj.contains( "tool" ) )
	{
		const QString toolName = obj.value( "tool" ).toString().trimmed();
		const QJsonObject args = obj.value( "args" ).toObject();
		if( toolName.isEmpty() )
		{
			return errorResponse( "missing_tool", tr( "Missing tool field" ) );
		}
		return dispatchTool( toolName, args );
	}

	QStringList tokens;
	const QString command = obj.value( "command" ).toString().trimmed();
	if( command.isEmpty() )
	{
		return errorResponse( "missing_command", tr( "Missing command field" ) );
	}

	tokens = command.split( ' ', Qt::SkipEmptyParts );

	const QString file = obj.value( "file" ).toString().trimmed();
	if( !file.isEmpty() )
	{
		tokens << file;
	}

	const QString path = obj.value( "path" ).toString().trimmed();
	if( !path.isEmpty() )
	{
		tokens << path;
	}

	const QString plugin = obj.value( "plugin" ).toString().trimmed();
	if( !plugin.isEmpty() )
	{
		tokens << plugin;
	}

	const QString track = obj.value( "track" ).toString().trimmed();
	if( !track.isEmpty() )
	{
		tokens << "to" << track;
	}

	return successResponse( QJsonObject
	{
		{ "message", dispatchTokens( tokens, command ) }
	} );
}

QString AgentControlService::dispatchTokens( const QStringList& tokens, const QString& rawText )
{
	QString result;
	QString error;
	if( tokens.isEmpty() )
	{
		return tr( "No command provided" );
	}

	const QString first = tokens[0].toLower();
	const QString normalizedRaw = normalizeName( rawText );
	auto containsPhrase = [&normalizedRaw, this]( const QString& phrase )
	{
		return normalizedRaw.contains( normalizeName( phrase ) );
	};

	auto beginnerHelpText = []()
	{
		return QString(
			"Beginner commands:\n"
			"- set tempo to 140\n"
			"- open mixer | open song editor | open piano roll\n"
			"- create instrument track | create sample track | create automation track\n"
			"- load instrument triple oscillator\n"
			"- add effect reverb | remove effect reverb\n"
			"- mute track <name> | solo track <name> | undo\n"
			"- manual map | manual map automation | manual map samples\n"
			"- compose from score sheet | add rhythm | sample as melodic instrument\n"
		"- sidechain through mixer | edit existing song | export song" );
	};

	if( ( first == "agent" && tokens.size() >= 2 && tokens[1].compare( "version", Qt::CaseInsensitive ) == 0 ) ||
		( first == "version" ) )
	{
		return tr( "AgentControl build %1 %2 (direct parser enabled: tempo, beginner help, manual map, playbook aliases)" )
			.arg( QString::fromLatin1( __DATE__ ) )
			.arg( QString::fromLatin1( __TIME__ ) );
	}

	auto manualMapText = []( const QString& area )
	{
		const QString key = area.trimmed().toLower();
		if( key.contains( "automation" ) )
		{
			return QString(
				"Manual map (Automation):\n"
				"- Automated now: create automation track, snapshots, undo, rollback\n"
				"- Guided now: draw curves, controller links, song-global automation\n"
				"- Deferred: typed automation point editing API" );
		}
		if( key.contains( "sample" ) )
		{
			return QString(
				"Manual map (Samples):\n"
				"- Automated now: create/load sample track, import audio/midi/hydrogen, search project audio\n"
				"- Guided now: sample prep and browser audition workflow\n"
				"- Deferred: typed trim/slice/normalize APIs" );
		}
		if( key.contains( "song" ) || key.contains( "track" ) )
		{
			return QString(
				"Manual map (Song Editor):\n"
				"- Automated now: create/rename/select/mute/solo tracks, create pattern\n"
				"- Guided now: timeline clip gestures (move/split/clone)\n"
				"- Deferred: typed clip-editing API" );
		}
		return QString(
			"Manual map (LMMS 0.4.12 compatibility):\n"
			"- Main window/navigation\n"
			"- Song editor/tracks\n"
			"- Piano roll composition\n"
			"- Beat+Bassline rhythm\n"
			"- Instruments/presets\n"
			"- FX mixer/effects\n"
			"- Automation/controllers\n"
			"- Import/samples\n"
			"Try: manual map song editor | manual map automation | manual map samples" );
	};
	const bool mentionsTempo = tokens.contains( "tempo", Qt::CaseInsensitive ) ||
		tokens.contains( "bpm", Qt::CaseInsensitive );
	auto parseFirstInteger = []( const QStringList& parts, int startIndex, int& valueOut )
	{
		const QRegularExpression intPattern( R"((-?\d+))" );
		for( int i = qMax( 0, startIndex ); i < parts.size(); ++i )
		{
			const auto match = intPattern.match( parts[i] );
			if( match.hasMatch() )
			{
				bool ok = false;
				const int parsed = match.captured( 1 ).toInt( &ok );
				if( ok )
				{
					valueOut = parsed;
					return true;
				}
			}
		}
		return false;
	};

	if( handleSlicerWorkflow( rawText, tokens, result, error ) )
	{
		return error.isEmpty() ? result : error;
	}

	// Transport controls: keep these deterministic and low latency.
	if( first == "play" || ( first == "start" && containsPhrase( "playback" ) ) ||
		( first == "resume" && containsPhrase( "playback" ) ) )
	{
		Song *song = Engine::getSong();
		if( song == nullptr )
		{
			return tr( "No active song" );
		}
		song->playSong();
		return tr( "Playback started" );
	}

	if( first == "pause" )
	{
		Song *song = Engine::getSong();
		if( song == nullptr )
		{
			return tr( "No active song" );
		}
		song->togglePause();
		return tr( "Playback toggled" );
	}

	if( first == "stop" && !containsPhrase( "confirm" ) )
	{
		Song *song = Engine::getSong();
		if( song == nullptr )
		{
			return tr( "No active song" );
		}
		song->stop();
		return tr( "Playback stopped" );
	}

	// Keep direct command mode usable for quick tempo changes from the plugin UI.
	// Natural-language planning still lives in the text/voice agent.
	if( mentionsTempo &&
		( first == "set" || first == "increase" || first == "decrease" ||
		  first == "raise" || first == "lower" || first == "tempo" ) )
	{
		const int toIndex = tokens.indexOf( "to" );
		const int byIndex = tokens.indexOf( "by" );
		const bool isDecrease = ( first == "decrease" || first == "lower" );
		const bool isRelative = ( first == "increase" || first == "decrease" || first == "raise" || first == "lower" );

		int targetTempo = 0;
		bool hasTarget = false;

		if( first == "tempo" )
		{
			hasTarget = parseFirstInteger( tokens, 1, targetTempo );
		}
		else if( isRelative && byIndex >= 0 )
		{
			int delta = 0;
			hasTarget = parseFirstInteger( tokens, byIndex + 1, delta );
			if( hasTarget )
			{
				if( const auto *song = Engine::getSong() )
				{
					targetTempo = song->getTempo() + ( isDecrease ? -delta : delta );
				}
				else
				{
					hasTarget = false;
				}
			}
		}
		else if( toIndex >= 0 || first == "set" || isRelative )
		{
			hasTarget = parseFirstInteger( tokens, toIndex >= 0 ? toIndex + 1 : 1, targetTempo );
		}

		if( !hasTarget )
		{
			return tr( "Could not parse tempo. Try: set tempo to 140" );
		}

		QJsonObject tempoResult;
		if( !setTempoValue( targetTempo, tempoResult, error ) )
		{
			return error;
		}
		return tr( "Set tempo to %1" ).arg( tempoResult.value( "tempo_after" ).toInt( targetTempo ) );
	}

	if( containsPhrase( "beginner help" ) )
	{
		return beginnerHelpText();
	}

	if( first == "manual" && tokens.size() >= 2 && tokens[1].compare( "map", Qt::CaseInsensitive ) == 0 )
	{
		return manualMapText( joinTokens( tokens, 2 ) );
	}

	if( first == "list" && tokens.size() >= 2 && tokens[1].compare( "windows", Qt::CaseInsensitive ) == 0 )
	{
		return QString(
			"Openable windows:\n"
			"- song editor\n"
			"- piano roll\n"
			"- automation editor\n"
			"- mixer (aliases: fx mixer, fx mixer/effects, effects)\n"
			"- controller rack\n"
			"- project notes\n"
			"- microtuner" );
	}

	if( containsPhrase( "compose from score sheet" ) )
	{
		QJsonObject tempoResult;
		if( !setTempoValue( 140, tempoResult, error ) )
		{
			return error;
		}
		QString trackResult;
		if( !createTrack( Track::Type::Instrument, trackResult, error ) )
		{
			return error;
		}
		QString instrumentResult;
		if( !createInstrumentTrack( "tripleoscillator", instrumentResult, error ) )
		{
			return error;
		}
		QJsonObject patternResult;
		if( !createPatternClip( QJsonObject(), patternResult, error ) )
		{
			return tr( "Started score-sheet workflow: tempo 140, instrument ready. Next: open piano roll and enter notes." );
		}
		return tr( "Started score-sheet workflow: tempo 140, TripleOscillator loaded, first pattern created." );
	}

	if( containsPhrase( "add rhythm" ) )
	{
		QString trackResult;
		if( !createTrack( Track::Type::Sample, trackResult, error ) )
		{
			return error;
		}
		if( !addKickPattern( error ) )
		{
			return tr( "Created sample track. Next: load drum samples and program BB rhythm." );
		}
		return tr( "Created sample track and added starter kick pattern." );
	}

	if( containsPhrase( "sample as melodic instrument" ) )
	{
		QString trackResult;
		if( !createTrack( Track::Type::Instrument, trackResult, error ) )
		{
			return error;
		}
		QString instrumentResult;
		if( !createInstrumentTrack( "audiofileprocessor", instrumentResult, error ) )
		{
			return tr( "Created instrument track. Next: load AudioFileProcessor manually and set root note." );
		}
		return tr( "Created sample-instrument track with AudioFileProcessor. Next: load sample and test pitch range." );
	}

	if( containsPhrase( "sidechain through mixer" ) )
	{
		if( !showWindowCommand( "mixer", result, error ) )
		{
			return error;
		}
		return tr( "Opened mixer. Next: route source/target, bind controller to gain, then tune depth/release." );
	}

	if( containsPhrase( "edit existing song" ) )
	{
		QJsonObject snapshotResult;
		if( !createSnapshot( "before_existing_song_edit", snapshotResult, error ) )
		{
			return error;
		}
		QString songEditorResult;
		showWindowCommand( "song editor", songEditorResult, error );
		QString mixerResult;
		showWindowCommand( "mixer", mixerResult, error );
		return tr( "Snapshot created. Opened Song Editor and Mixer for edit pass." );
	}

	if( containsPhrase( "export song" ) || containsPhrase( "render song" ) )
	{
		return tr( "Use File -> Export (Ctrl+E). Choose format and destination, then render." );
	}

	if( first == "new" )
	{
		if( normalizeName( rawText ).contains( "arpeggiator" ) &&
			normalizeName( rawText ).contains( "lane" ) )
		{
			QString createResult;
			if( !createTrack( Track::Type::Pattern, createResult, error ) )
			{
				return error;
			}
			QString openResult;
			QString openError;
			if( !showWindowCommand( QStringLiteral( "piano roll" ), openResult, openError ) )
			{
				return createResult;
			}
			return tr( "Mapped arpeggiator lane to pattern workflow. %1. %2" ).arg( createResult, openResult );
		}
		if( tokens.size() >= 2 && tokens[1].compare( "project", Qt::CaseInsensitive ) == 0 )
		{
			return newProject( result, error ) ? result : error;
		}
		if( tokens.size() >= 3 && tokens[1].compare( "sample", Qt::CaseInsensitive ) == 0 &&
			tokens[2].compare( "track", Qt::CaseInsensitive ) == 0 )
		{
			return createTrack( Track::Type::Sample, result, error ) ? result : error;
		}
		if( tokens.size() >= 3 && tokens[1].compare( "instrument", Qt::CaseInsensitive ) == 0 &&
			tokens[2].compare( "track", Qt::CaseInsensitive ) == 0 )
		{
			return createTrack( Track::Type::Instrument, result, error ) ? result : error;
		}
		if( tokens.size() >= 3 && tokens[1].compare( "automation", Qt::CaseInsensitive ) == 0 &&
			tokens[2].compare( "track", Qt::CaseInsensitive ) == 0 )
		{
			return createTrack( Track::Type::Automation, result, error ) ? result : error;
		}
		if( tokens.size() >= 3 && tokens[1].compare( "instrument", Qt::CaseInsensitive ) == 0 )
		{
			return createInstrumentTrack( joinTokens( tokens, 2 ), result, error ) ? result : error;
		}
	}
	if( first == "create" )
	{
		if( normalizeName( rawText ).contains( "arpeggiator" ) &&
			normalizeName( rawText ).contains( "lane" ) )
		{
			QString createResult;
			if( !createTrack( Track::Type::Pattern, createResult, error ) )
			{
				return error;
			}
			QString openResult;
			QString openError;
			if( !showWindowCommand( QStringLiteral( "piano roll" ), openResult, openError ) )
			{
				return createResult;
			}
			return tr( "Mapped arpeggiator lane to pattern workflow. %1. %2" ).arg( createResult, openResult );
		}
		if( tokens.size() >= 3 && tokens[1].compare( "sample", Qt::CaseInsensitive ) == 0 &&
			tokens[2].compare( "track", Qt::CaseInsensitive ) == 0 )
		{
			return createTrack( Track::Type::Sample, result, error ) ? result : error;
		}
		if( tokens.size() >= 3 && tokens[1].compare( "instrument", Qt::CaseInsensitive ) == 0 &&
			tokens[2].compare( "track", Qt::CaseInsensitive ) == 0 )
		{
			return createTrack( Track::Type::Instrument, result, error ) ? result : error;
		}
		if( tokens.size() >= 3 && tokens[1].compare( "automation", Qt::CaseInsensitive ) == 0 &&
			tokens[2].compare( "track", Qt::CaseInsensitive ) == 0 )
		{
			return createTrack( Track::Type::Automation, result, error ) ? result : error;
		}
		if( tokens.size() >= 3 && tokens[1].compare( "pattern", Qt::CaseInsensitive ) == 0 &&
			tokens[2].compare( "track", Qt::CaseInsensitive ) == 0 )
		{
			return createTrack( Track::Type::Pattern, result, error ) ? result : error;
		}
	}

	if( first == "show" )
	{
		if( tokens.size() >= 3 && tokens[1].compare( "tool", Qt::CaseInsensitive ) == 0 )
		{
			return showToolCommand( joinTokens( tokens, 2 ), result, error ) ? result : error;
		}
		return showWindowCommand( joinTokens( tokens, 1 ), result, error ) ? result : error;
	}

	if( first == "open" )
	{
		if( tokens.size() >= 2 && tokens[1].compare( "project", Qt::CaseInsensitive ) == 0 )
		{
			return openProject( joinTokens( tokens, 2 ), result, error ) ? result : error;
		}
		if( tokens.size() >= 2 && normalizeName( joinTokens( tokens, 1 ) ) == "slicer" )
		{
			return createInstrumentTrack( "slicert", result, error ) ? result : error;
		}
		if( tokens.size() >= 2 )
		{
			return showWindowCommand( joinTokens( tokens, 1 ), result, error ) ? result : error;
		}
	}

	if( first == "save" )
	{
		if( tokens.size() >= 2 && tokens[1].compare( "project", Qt::CaseInsensitive ) == 0 )
		{
			if( tokens.size() >= 4 && tokens[2].compare( "as", Qt::CaseInsensitive ) == 0 )
			{
				return saveProjectAs( joinTokens( tokens, 3 ), result, error ) ? result : error;
			}
			return saveProject( result, error ) ? result : error;
		}
	}

	if( first == "import" )
	{
		if( tokens.size() >= 3 && tokens[1].compare( "audio", Qt::CaseInsensitive ) == 0 )
		{
			return importAudioFile( joinTokens( tokens, 2 ), error ) ? tr( "Imported %1" ).arg( joinTokens( tokens, 2 ) ) : error;
		}
		if( tokens.size() >= 3 && tokens[1].compare( "midi", Qt::CaseInsensitive ) == 0 )
		{
			return importProjectFile( joinTokens( tokens, 2 ), error ) ? tr( "Imported MIDI %1" ).arg( joinTokens( tokens, 2 ) ) : error;
		}
		if( tokens.size() >= 3 && tokens[1].compare( "hydrogen", Qt::CaseInsensitive ) == 0 )
		{
			return importProjectFile( joinTokens( tokens, 2 ), error ) ? tr( "Imported Hydrogen project %1" ).arg( joinTokens( tokens, 2 ) ) : error;
		}
		if( tokens.size() >= 2 )
		{
			QString fileName = joinTokens( tokens, 1 );
			fileName.remove( QRegularExpression(
				R"(\bfrom\s+(?:my\s+)?downloads?(?:\s+folder)?\b.*$)",
				QRegularExpression::CaseInsensitiveOption ) );
			fileName = fileName.trimmed();

			if( importAudioFile( fileName, error ) )
			{
				return tr( "Imported %1" ).arg( fileName );
			}

			const QString fuzzyDownloadsMatch = resolveDownloadsAudioQuery( fileName );
			if( !fuzzyDownloadsMatch.isEmpty() && importAudioFile( fuzzyDownloadsMatch, error ) )
			{
				return tr( "Imported %1" ).arg( QFileInfo( fuzzyDownloadsMatch ).fileName() );
			}

			if( importFromDownloads( fileName, error ) )
			{
				return tr( "Imported %1" ).arg( fileName );
			}
			return error;
		}
	}

	if( first == "load" || first == "set" )
	{
		if( tokens.size() >= 3 && tokens[1].compare( "instrument", Qt::CaseInsensitive ) == 0 )
		{
			return createInstrumentTrack( joinTokens( tokens, 2 ), result, error ) ? result : error;
		}
	}

	if( first == "add" )
	{
		if( tokens.size() >= 3 && tokens[1].compare( "effect", Qt::CaseInsensitive ) == 0 )
		{
			int toIndex = tokens.indexOf( "to" );
			const QString effectName = toIndex > 2 ? tokens.mid( 2, toIndex - 2 ).join( " " ) : joinTokens( tokens, 2 );
			const QString trackName = toIndex >= 0 ? joinTokens( tokens, toIndex + 1 ) : QString();
			return addEffectToTrack( effectName, trackName, result, error ) ? result : error;
		}
		if( rawText.contains( "piano", Qt::CaseInsensitive ) )
		{
			QString createResult;
			if( createInstrumentTrack( "tripleoscillator", createResult, error ) )
			{
				return tr( "Queued piano instrument (TripleOscillator)" );
			}
			return error;
		}
		if( rawText.contains( "808", Qt::CaseInsensitive ) || rawText.contains( "kick", Qt::CaseInsensitive ) )
		{
			return addKickPattern( error ) ? tr( "Added kick pattern" ) : error;
		}
		if( rawText.contains( "snare", Qt::CaseInsensitive ) )
		{
			return addSnarePattern( error ) ? tr( "Added snare pattern" ) : error;
		}
		if( rawText.contains( "hihat", Qt::CaseInsensitive ) ||
			rawText.contains( "hi-hat", Qt::CaseInsensitive ) ||
			rawText.contains( "hi hat", Qt::CaseInsensitive ) ||
			rawText.contains( "hats", Qt::CaseInsensitive ) )
		{
			return addHiHatPattern( error ) ? tr( "Added hi-hat pattern" ) : error;
		}
		if( rawText.contains( "ride", Qt::CaseInsensitive ) )
		{
			return addRidePattern( error ) ? tr( "Added ride pattern" ) : error;
		}
		if( rawText.contains( "crash", Qt::CaseInsensitive ) ||
			rawText.contains( "cymbal", Qt::CaseInsensitive ) )
		{
			return addCrashPattern( error ) ? tr( "Added crash cymbal pattern" ) : error;
		}
	}

	if( first == "make" )
	{
		if( rawText.contains( "drums", Qt::CaseInsensitive ) &&
			rawText.contains( "hit", Qt::CaseInsensitive ) &&
			rawText.contains( "harder", Qt::CaseInsensitive ) )
		{
			return addKickPattern( error ) ? tr( "Added kick pattern" ) : error;
		}
	}

	if( first == "remove" )
	{
		if( tokens.size() >= 3 && tokens[1].compare( "effect", Qt::CaseInsensitive ) == 0 )
		{
			int fromIndex = tokens.indexOf( "from" );
			const QString effectName = fromIndex > 2 ? tokens.mid( 2, fromIndex - 2 ).join( " " ) : joinTokens( tokens, 2 );
			const QString trackName = fromIndex >= 0 ? joinTokens( tokens, fromIndex + 1 ) : QString();
			return removeEffectFromTrack( effectName, trackName, result, error ) ? result : error;
		}
	}

	if( first == "mute" )
	{
		const QString trackName = ( tokens.size() >= 2 && tokens[1].compare( "track", Qt::CaseInsensitive ) == 0 ) ?
			joinTokens( tokens, 2 ) : joinTokens( tokens, 1 );
		QJsonObject muteResult;
		if( setTrackMute( trackName, true, muteResult, error ) )
		{
			return tr( "Muted track %1" ).arg( muteResult.value( "track_name" ).toString( trackName ) );
		}
		return error;
	}

	if( first == "unmute" )
	{
		const QString trackName = ( tokens.size() >= 2 && tokens[1].compare( "track", Qt::CaseInsensitive ) == 0 ) ?
			joinTokens( tokens, 2 ) : joinTokens( tokens, 1 );
		QJsonObject muteResult;
		if( setTrackMute( trackName, false, muteResult, error ) )
		{
			return tr( "Unmuted track %1" ).arg( muteResult.value( "track_name" ).toString( trackName ) );
		}
		return error;
	}

	if( first == "solo" )
	{
		const QString trackName = ( tokens.size() >= 2 && tokens[1].compare( "track", Qt::CaseInsensitive ) == 0 ) ?
			joinTokens( tokens, 2 ) : joinTokens( tokens, 1 );
		QJsonObject soloResult;
		if( setTrackSolo( trackName, true, soloResult, error ) )
		{
			return tr( "Soloed track %1" ).arg( soloResult.value( "track_name" ).toString( trackName ) );
		}
		return error;
	}

	if( first == "unsolo" )
	{
		const QString trackName = ( tokens.size() >= 2 && tokens[1].compare( "track", Qt::CaseInsensitive ) == 0 ) ?
			joinTokens( tokens, 2 ) : joinTokens( tokens, 1 );
		QJsonObject soloResult;
		if( setTrackSolo( trackName, false, soloResult, error ) )
		{
			return tr( "Unsoloed track %1" ).arg( soloResult.value( "track_name" ).toString( trackName ) );
		}
		return error;
	}

	if( first == "undo" )
	{
		QJsonObject undoResult;
		if( undoLastAction( undoResult, error ) )
		{
			return tr( "Undid last action" );
		}
		return error;
	}

	return tr( "Unknown command: %1. For natural-language tasks use lmmsagent/scripts/run_text_agent.sh" ).arg( rawText );
}

bool AgentControlService::handleSlicerWorkflow(
	const QString& rawText,
	const QStringList& tokens,
	QString& result,
	QString& error )
{
	if( tokens.isEmpty() )
	{
		return false;
	}

	const QString first = tokens[0].toLower();
	const QString normalizedRaw = normalizeName( rawText );
	const bool mentionsSlicer = normalizedRaw.contains( "slicer" ) ||
		normalizedRaw.contains( "splicer" );
	const bool mentionsImport = normalizedRaw.contains( "import" );
	const bool mentionsDownloadFolder = normalizedRaw.contains( "fromdownloads" ) ||
		normalizedRaw.contains( "frommydownloads" );
	const bool mentionsTransientSlices = normalizedRaw.contains( "transient" ) ||
		normalizedRaw.contains( "onset" );
	const bool mentionsSegmentOrSlice = normalizedRaw.contains( "segment" ) ||
		normalizedRaw.contains( "slice" );
	const bool hasExplicitSegmentCount = QRegularExpression( R"(\b\d+\b)" ).match( rawText ).hasMatch();
	const bool mentionsEqualSlices = normalizedRaw.contains( "equalsegment" ) ||
		normalizedRaw.contains( "equalslice" ) ||
		( mentionsSegmentOrSlice && !mentionsTransientSlices &&
			( normalizedRaw.contains( "divideinto" ) || normalizedRaw.contains( "splitinto" ) || hasExplicitSegmentCount ) );

	const bool openSlicerRequest = normalizedRaw.contains( "openslicer" ) ||
		( first == "open" && tokens.size() >= 2 &&
			( normalizeName( joinTokens( tokens, 1 ) ) == "slicer" ||
			  normalizeName( joinTokens( tokens, 1 ) ) == "splicer" ) ) ||
		( first == "slicer" && tokens.size() >= 2 &&
			( tokens[1].compare( "open", Qt::CaseInsensitive ) == 0 ||
			  tokens[1].compare( "show", Qt::CaseInsensitive ) == 0 ) );
	const bool importIntoSlicerRequest = ( mentionsImport || mentionsDownloadFolder ||
		normalizedRaw.contains( "intoslicer" ) ) && ( mentionsSlicer || openSlicerRequest );
	const bool slicerSplitRequest = ( first == "divide" || first == "split" || first == "slice" ||
		first == "slicer" || mentionsSlicer ) && ( mentionsEqualSlices || mentionsTransientSlices );

	if( !openSlicerRequest && !importIntoSlicerRequest && !slicerSplitRequest )
	{
		return false;
	}

	QStringList steps;
	if( openSlicerRequest )
	{
		InstrumentTrack *track = nullptr;
		if( !ensureSlicerTrack( track, true, error ) )
		{
			return true;
		}
		if( !focusInstrumentTrackWindow( track, error ) )
		{
			return true;
		}
		steps << tr( "Opened slicer on track %1" ).arg( track->name() );
	}

	if( importIntoSlicerRequest )
	{
		QString importMessage;
		const QString fileQuery = extractAudioQuery( rawText, tokens );
		if( fileQuery.isEmpty() )
		{
			error = tr( "Could not detect an audio filename for slicer import" );
			return true;
		}
		if( !loadFileIntoSlicer( fileQuery, importMessage, error ) )
		{
			return true;
		}
		steps << importMessage;
	}

	if( mentionsEqualSlices )
	{
		int segmentCount = 16;
		const QRegularExpression beforeEqual( R"((\d+)\s+equal\s+(?:segments?|slices?))",
			QRegularExpression::CaseInsensitiveOption );
		const QRegularExpression afterEqual( R"(equal\s+(?:segments?|slices?)(?:\s+of)?\s+(\d+))",
			QRegularExpression::CaseInsensitiveOption );
		const QRegularExpression genericSegments( R"((\d+)\s+(?:segments?|slices?))",
			QRegularExpression::CaseInsensitiveOption );

		auto match = beforeEqual.match( rawText );
		if( !match.hasMatch() )
		{
			match = afterEqual.match( rawText );
		}
		if( !match.hasMatch() )
		{
			match = genericSegments.match( rawText );
		}
		if( match.hasMatch() )
		{
			bool ok = false;
			const int parsed = match.captured( 1 ).toInt( &ok );
			if( ok )
			{
				segmentCount = parsed;
			}
		}
		segmentCount = qBound( 1, segmentCount, 128 );

		QString sliceMessage;
		if( !sliceSlicerEqual( segmentCount, sliceMessage, error ) )
		{
			return true;
		}
		steps << sliceMessage;
	}

	if( mentionsTransientSlices )
	{
		QString sliceMessage;
		if( !sliceSlicerByTransients( sliceMessage, error ) )
		{
			return true;
		}
		steps << sliceMessage;
	}

	if( steps.isEmpty() )
	{
		return false;
	}

	result = steps.join( ". " );
	return true;
}

QJsonObject AgentControlService::dispatchTool( const QString& toolName, const QJsonObject& args )
{
	const QString tool = normalizeName( toolName );
	const QSet<QString> writeTools =
	{
		"createtrack", "renametrack", "loadinstrument", "loadsample", "createpattern",
		"addnotes", "addsteps", "settempo", "addeffect", "removeeffect", "seteffectparam",
		"opentool", "importaudio", "importmidi", "importhydrogen", "selecttrack", "mutetrack",
		"solotrack", "undoslastaction", "undolastaction", "createsnapshot", "rollbacktosnapshot"
	};
	const QSet<QString> mutatingTools =
	{
		"createtrack", "renametrack", "loadinstrument", "loadsample", "createpattern",
		"addnotes", "addsteps", "settempo", "addeffect", "removeeffect", "seteffectparam",
		"importaudio", "importmidi", "importhydrogen", "mutetrack", "solotrack"
	};

	const bool isWriteTool = writeTools.contains( tool );
	const QJsonObject beforeState = isWriteTool ? projectStateObject() : QJsonObject();
	QJsonObject result;
	QString error;
	bool ok = true;
	QJsonArray warnings;

	auto isKnownWindow = [this]( const QString& name )
	{
		const QString wanted = normalizeName( name );
		for( const auto &value : availableWindows() )
		{
			if( normalizeName( value.toString() ) == wanted )
			{
				return true;
			}
		}
		return false;
	};

	if( tool == "getprojectstate" )
	{
		result = projectStateObject();
	}
	else if( tool == "listtracks" )
	{
		result = QJsonObject{ { "tracks", trackArray() } };
	}
	else if( tool == "gettrackdetails" )
	{
		Track* track = resolveTrackRef( args );
		if( track == nullptr )
		{
			return errorResponse( "track_not_found", tr( "Track not found" ) );
		}
		result = trackObject( track, trackIndex( track ) );
	}
	else if( tool == "listpatterns" )
	{
		result = QJsonObject{ { "patterns", listPatternArray() } };
	}
	else if( tool == "listinstruments" )
	{
		QJsonArray activeTracks;
		Song* song = Engine::getSong();
		if( song != nullptr )
		{
			int idx = 0;
			for( const auto *track : song->tracks() )
			{
				if( dynamic_cast<const InstrumentTrack *>( track ) != nullptr )
				{
					activeTracks.append( trackObject( track, idx ) );
				}
				++idx;
			}
		}
		result = QJsonObject
		{
			{ "installed", availableInstruments() },
			{ "active_tracks", activeTracks }
		};
	}
	else if( tool == "listeffects" )
	{
		Track* track = resolveTrackRef( args );
		result = QJsonObject
		{
			{ "installed", availableEffects() },
			{ "track_effects", track != nullptr ? effectArrayForTrack( track ) : QJsonArray() }
		};
	}
	else if( tool == "listtoolwindows" )
	{
		result = QJsonObject
		{
			{ "windows", availableWindows() },
			{ "tools", availableTools() }
		};
	}
	else if( tool == "getselectionstate" )
	{
		result = QJsonObject
		{
			{ "selected_track", m_selectedTrackName },
			{ "has_selection", !m_selectedTrackName.isEmpty() }
		};
	}
	else if( tool == "findtrackbyname" )
	{
		const QString query = args.value( "query" ).toString().trimmed().isEmpty()
			? args.value( "name" ).toString().trimmed()
			: args.value( "query" ).toString().trimmed();
		if( query.isEmpty() )
		{
			return errorResponse( "invalid_args", tr( "Missing query for find_track_by_name" ) );
		}
		const QString wanted = normalizeName( query );
		Song* song = Engine::getSong();
		if( song == nullptr )
		{
			return errorResponse( "no_song", tr( "No active song" ) );
		}
		const auto &tracks = song->tracks();
		Track* best = nullptr;
		for( auto *track : tracks )
		{
			const QString trackName = normalizeName( track->name() );
			if( trackName == wanted )
			{
				best = track;
				break;
			}
			if( best == nullptr && trackName.contains( wanted ) )
			{
				best = track;
			}
		}
		if( best == nullptr )
		{
			return errorResponse( "track_not_found", tr( "No track matched %1" ).arg( query ) );
		}
		result = QJsonObject{ { "match", trackObject( best, trackIndex( best ) ) } };
	}
	else if( tool == "searchprojectaudio" )
	{
		const QString query = args.value( "query" ).toString().trimmed();
		result = QJsonObject{ { "matches", searchProjectAudio( query ) } };
	}
	else if( tool == "createtrack" )
	{
		const QString typeName = normalizeName( args.value( "type" ).toString().trimmed() );
		Track::Type type = Track::Type::Instrument;
		if( typeName == "sample" || typeName == "sampletrack" )
		{
			type = Track::Type::Sample;
		}
		else if( typeName == "automation" || typeName == "automationtrack" )
		{
			type = Track::Type::Automation;
		}
		else if( typeName == "pattern" || typeName == "patterntrack" )
		{
			type = Track::Type::Pattern;
		}
		Song* song = Engine::getSong();
		if( song == nullptr )
		{
			return errorResponse( "no_song", tr( "No active song" ) );
		}
		Track* track = Track::create( type, song );
		if( track == nullptr )
		{
			return errorResponse( "create_failed", tr( "Failed to create track" ) );
		}
		const QString name = args.value( "name" ).toString().trimmed();
		if( !name.isEmpty() )
		{
			track->setName( name );
		}
		m_selectedTrackName = track->name();
		result = QJsonObject{ { "track", trackObject( track, trackIndex( track ) ) } };
	}
	else if( tool == "renametrack" )
	{
		ok = renameTrack(
			args.value( "track" ).toString().trimmed().isEmpty()
				? args.value( "track_name" ).toString().trimmed()
				: args.value( "track" ).toString().trimmed(),
			args.value( "new_name" ).toString().trimmed(),
			result, error );
	}
	else if( tool == "loadinstrument" )
	{
		const QString pluginQuery = args.value( "plugin" ).toString().trimmed().isEmpty()
			? args.value( "name" ).toString().trimmed()
			: args.value( "plugin" ).toString().trimmed();
		if( pluginQuery.isEmpty() )
		{
			return errorResponse( "invalid_args", tr( "Missing plugin for load_instrument" ) );
		}

		QString displayName;
		const QString pluginName = resolveInstrumentPlugin( pluginQuery, displayName );
		if( pluginName.isEmpty() )
		{
			return errorResponse( "plugin_not_found", tr( "Unknown instrument: %1" ).arg( pluginQuery ) );
		}

		InstrumentTrack* track = resolveInstrumentTrack( args );
		if( track == nullptr )
		{
			Song* song = Engine::getSong();
			if( song == nullptr )
			{
				return errorResponse( "no_song", tr( "No active song" ) );
			}
			track = dynamic_cast<InstrumentTrack *>( Track::create( Track::Type::Instrument, song ) );
		}
		if( track == nullptr )
		{
			return errorResponse( "create_failed", tr( "Could not create instrument track" ) );
		}
		track->loadInstrument( pluginName );
		if( !args.value( "track_name" ).toString().trimmed().isEmpty() )
		{
			track->setName( args.value( "track_name" ).toString().trimmed() );
		}
		m_selectedTrackName = track->name();
		result = QJsonObject
		{
			{ "plugin", pluginName },
			{ "plugin_display_name", displayName },
			{ "track", trackObject( track, trackIndex( track ) ) }
		};
	}
	else if( tool == "loadsample" )
	{
		const QString samplePath = args.value( "sample_path" ).toString().trimmed().isEmpty()
			? args.value( "path" ).toString().trimmed()
			: args.value( "sample_path" ).toString().trimmed();
		const QString trackName = args.value( "track" ).toString().trimmed().isEmpty()
			? args.value( "track_name" ).toString().trimmed()
			: args.value( "track" ).toString().trimmed();
		ok = loadSampleToTrack( samplePath, trackName, result, error );
	}
	else if( tool == "createpattern" )
	{
		ok = createPatternClip( args, result, error );
	}
	else if( tool == "addnotes" )
	{
		ok = addNotesToPattern( args, result, error );
	}
	else if( tool == "addsteps" )
	{
		ok = addStepsToPattern( args, result, error );
	}
	else if( tool == "settempo" )
	{
		const int tempo = args.value( "tempo" ).toInt( 0 );
		ok = setTempoValue( tempo, result, error );
	}
	else if( tool == "addeffect" )
	{
		QString message;
		const QString effectName = args.value( "effect" ).toString().trimmed().isEmpty()
			? args.value( "name" ).toString().trimmed()
			: args.value( "effect" ).toString().trimmed();
		const QString trackName = args.value( "track" ).toString().trimmed();
		ok = addEffectToTrack( effectName, trackName, message, error );
		result = QJsonObject{ { "message", message } };
	}
	else if( tool == "removeeffect" )
	{
		QString message;
		const QString effectName = args.value( "effect" ).toString().trimmed().isEmpty()
			? args.value( "name" ).toString().trimmed()
			: args.value( "effect" ).toString().trimmed();
		const QString trackName = args.value( "track" ).toString().trimmed();
		ok = removeEffectFromTrack( effectName, trackName, message, error );
		result = QJsonObject{ { "message", message } };
	}
	else if( tool == "opentool" )
	{
		QString message;
		const QString name = args.value( "name" ).toString().trimmed();
		if( name.isEmpty() )
		{
			return errorResponse( "invalid_args", tr( "Missing tool or window name" ) );
		}
		const QString kind = normalizeName( args.value( "kind" ).toString().trimmed() );
		if( kind == "window" )
		{
			ok = showWindowCommand( name, message, error );
		}
		else if( kind == "tool" )
		{
			ok = showToolCommand( name, message, error );
		}
		else if( isKnownWindow( name ) )
		{
			ok = showWindowCommand( name, message, error );
		}
		else
		{
			ok = showToolCommand( name, message, error );
			if( !ok )
			{
				ok = showWindowCommand( name, message, error );
			}
		}
		result = QJsonObject{ { "message", message } };
	}
	else if( tool == "importaudio" )
	{
		const QString path = args.value( "path" ).toString().trimmed();
		ok = importAudioFile( path, error );
		result = QJsonObject{ { "path", path } };
	}
	else if( tool == "importmidi" )
	{
		const QString path = args.value( "path" ).toString().trimmed();
		ok = importProjectFile( path, error );
		result = QJsonObject{ { "path", path }, { "format", "midi" } };
	}
	else if( tool == "importhydrogen" )
	{
		const QString path = args.value( "path" ).toString().trimmed();
		ok = importProjectFile( path, error );
		result = QJsonObject{ { "path", path }, { "format", "hydrogen" } };
	}
	else if( tool == "selecttrack" )
	{
		const QString trackName = args.value( "track" ).toString().trimmed().isEmpty()
			? args.value( "track_name" ).toString().trimmed()
			: args.value( "track" ).toString().trimmed();
		ok = selectTrack( trackName, result, error );
	}
	else if( tool == "mutetrack" )
	{
		const QString trackName = args.value( "track" ).toString().trimmed().isEmpty()
			? args.value( "track_name" ).toString().trimmed()
			: args.value( "track" ).toString().trimmed();
		const bool mute = args.value( "mute" ).toBool( true );
		ok = setTrackMute( trackName, mute, result, error );
	}
	else if( tool == "solotrack" )
	{
		const QString trackName = args.value( "track" ).toString().trimmed().isEmpty()
			? args.value( "track_name" ).toString().trimmed()
			: args.value( "track" ).toString().trimmed();
		const bool solo = args.value( "solo" ).toBool( true );
		ok = setTrackSolo( trackName, solo, result, error );
	}
	else if( tool == "createsnapshot" )
	{
		ok = createSnapshot( args.value( "label" ).toString().trimmed(), result, error );
	}
	else if( tool == "undolastaction" || tool == "undoslastaction" )
	{
		ok = undoLastAction( result, error );
	}
	else if( tool == "rollbacktosnapshot" )
	{
		const QString snapshotId = args.value( "snapshot_id" ).toString().trimmed();
		ok = rollbackSnapshot( snapshotId, result, error );
	}
	else if( tool == "diffsincesnapshot" || tool == "diffsincesnapshop" || tool == "diffsince_snapshot" )
	{
		const QString snapshotId = args.value( "snapshot_id" ).toString().trimmed();
		ok = diffSinceSnapshot( snapshotId, result, error );
	}
	else
	{
		// v2 tool surface (see lmmsagent/docs/TOOL_CONTRACT_V2.md)
		return dispatchV2Tool( tool, args );
	}

	if( !ok )
	{
		return errorResponse( "tool_failed", error.isEmpty() ? tr( "Tool execution failed" ) : error, warnings );
	}

	QJsonObject stateDelta;
	if( isWriteTool )
	{
		const QJsonObject afterState = projectStateObject();
		stateDelta = diffState( beforeState, afterState );
		if( mutatingTools.contains( tool ) )
		{
			++m_actionCounter;
		}
	}

	return successResponse( result, stateDelta, warnings );
}

void AgentControlService::onNewConnection()
{
	while( m_server.hasPendingConnections() )
	{
		QTcpSocket *sock = m_server.nextPendingConnection();
		m_clients.insert( sock );
		connect( sock, &QTcpSocket::readyRead, this, &AgentControlService::onSocketReady );
		connect( sock, &QTcpSocket::disconnected, this, &AgentControlService::onSocketClosed );
		emit logMessage( tr( "Client connected" ) );
	}
}

void AgentControlService::onSocketReady()
{
	QPointer<QTcpSocket> sock = qobject_cast<QTcpSocket *>( sender() );
	if( !sock || !m_clients.contains( sock.data() ) )
	{
		return;
	}

	m_readBuffers[sock.data()].append( sock->readAll() );

	while( true )
	{
		if( !sock || !m_clients.contains( sock.data() ) )
		{
			return;
		}

		auto bufferIt = m_readBuffers.find( sock.data() );
		if( bufferIt == m_readBuffers.end() )
		{
			return;
		}

		const int newlineIndex = bufferIt->indexOf( '\n' );
		if( newlineIndex < 0 )
		{
			return;
		}

		const QByteArray line = bufferIt->left( newlineIndex );
		bufferIt->remove( 0, newlineIndex + 1 );

		if( line.trimmed().isEmpty() )
		{
			continue;
		}

		QJsonParseError parseError {};
		const QJsonDocument doc = QJsonDocument::fromJson( line, &parseError );
		QJsonObject reply;
		QString displayResult;
		if( parseError.error == QJsonParseError::NoError && doc.isObject() )
		{
			reply = handleRequest( doc.object() );
			displayResult = toCommandResponseText( reply );
		}
		else
		{
			const QString result = handleCommand( QString::fromUtf8( line ) );
			displayResult = result;
			reply = successResponse( QJsonObject
			{
				{ "message", result }
			} );
		}

		if( !reply.contains( "ok" ) )
		{
			reply = errorResponse( "invalid_response", tr( "AgentControl returned an invalid response" ) );
		}

		const bool ok = reply.value( "ok" ).toBool( false );
		if( !ok && reply.value( "error_message" ).toString().isEmpty() )
		{
			reply["error_message"] = tr( "Request failed" );
		}
		if( !ok )
		{
			displayResult = reply.value( "error_message" ).toString();
		}
		emit commandResult( displayResult );

		if( !sock || !m_clients.contains( sock.data() ) ||
			sock->state() != QAbstractSocket::ConnectedState )
		{
			return;
		}

		const QByteArray payload = QJsonDocument( reply ).toJson( QJsonDocument::Compact ) + "\n";
		if( sock->write( payload ) < 0 )
		{
			emit logMessage( tr( "Failed to write response to client socket" ) );
			return;
		}
		sock->flush();
	}
}

void AgentControlService::onSocketClosed()
{
	auto *sock = qobject_cast<QTcpSocket *>( sender() );
	if( !sock )
	{
		return;
	}
	m_clients.remove( sock );
	m_readBuffers.remove( sock );
	sock->deleteLater();
	emit logMessage( tr( "Client disconnected" ) );
}

bool AgentControlService::newProject( QString& result, QString& error )
{
	Song *song = Engine::getSong();
	auto *guiApp = gui::getGUI();
	if( song == nullptr || guiApp == nullptr || guiApp->mainWindow() == nullptr )
	{
		error = tr( "GUI is not ready" );
		return false;
	}
	if( m_projectTransitionQueued )
	{
		error = tr( "A project open/new operation is already in progress" );
		return false;
	}
	m_projectTransitionQueued = true;
	QTimer::singleShot( 0, guiApp->mainWindow(), [this, song]()
	{
		song->createNewProject();
		m_projectTransitionQueued = false;
		processDeferredOpenProject();
	} );
	result = tr( "Queued new project" );
	return true;
}

bool AgentControlService::openProject( const QString& path, QString& result, QString& error )
{
	const QString fullPath = canonicalPath( path );
	if( fullPath.isEmpty() )
	{
		error = tr( "Project not found: %1" ).arg( path );
		return false;
	}
	Song *song = Engine::getSong();
	auto *guiApp = gui::getGUI();
	if( song == nullptr || guiApp == nullptr || guiApp->mainWindow() == nullptr )
	{
		error = tr( "GUI is not ready" );
		return false;
	}
	if( m_projectTransitionQueued )
	{
		scheduleDeferredOpenProject( fullPath );
		result = tr( "Queued open project retry for %1" ).arg( QFileInfo( fullPath ).fileName() );
		return true;
	}
	m_projectTransitionQueued = true;
	QTimer::singleShot( 0, guiApp->mainWindow(), [this, song, fullPath]()
	{
		song->loadProject( fullPath );
		m_projectTransitionQueued = false;
		processDeferredOpenProject();
	} );
	result = tr( "Queued open project %1" ).arg( QFileInfo( fullPath ).fileName() );
	return true;
}

void AgentControlService::scheduleDeferredOpenProject( const QString& fullPath )
{
	if( fullPath.isEmpty() )
	{
		return;
	}

	m_deferredOpenProjectPath = fullPath;
	m_deferredOpenProjectRetries = 0;
	QTimer::singleShot( 250, this, [this]()
	{
		processDeferredOpenProject();
	} );
}

void AgentControlService::processDeferredOpenProject()
{
	if( m_deferredOpenProjectPath.isEmpty() )
	{
		return;
	}

	if( m_projectTransitionQueued )
	{
		++m_deferredOpenProjectRetries;
		if( m_deferredOpenProjectRetries > 24 )
		{
			emit logMessage( tr( "Deferred open project timed out for %1" )
				.arg( QFileInfo( m_deferredOpenProjectPath ).fileName() ) );
			m_deferredOpenProjectPath.clear();
			m_deferredOpenProjectRetries = 0;
			return;
		}

		QTimer::singleShot( 250, this, [this]()
		{
			processDeferredOpenProject();
		} );
		return;
	}

	Song *song = Engine::getSong();
	auto *guiApp = gui::getGUI();
	if( song == nullptr || guiApp == nullptr || guiApp->mainWindow() == nullptr )
	{
		emit logMessage( tr( "Deferred open project skipped: GUI is not ready" ) );
		m_deferredOpenProjectPath.clear();
		m_deferredOpenProjectRetries = 0;
		return;
	}

	const QString fullPath = m_deferredOpenProjectPath;
	m_deferredOpenProjectPath.clear();
	m_deferredOpenProjectRetries = 0;
	m_projectTransitionQueued = true;

	QTimer::singleShot( 0, guiApp->mainWindow(), [this, song, fullPath]()
	{
		song->loadProject( fullPath );
		m_projectTransitionQueued = false;
		emit logMessage( tr( "Deferred open project executed: %1" )
			.arg( QFileInfo( fullPath ).fileName() ) );
		processDeferredOpenProject();
	} );
}

bool AgentControlService::saveProject( QString& result, QString& error )
{
	Song *song = Engine::getSong();
	if( song == nullptr )
	{
		error = tr( "No active song" );
		return false;
	}
	if( song->projectFileName().isEmpty() )
	{
		error = tr( "Project has no filename. Use save project as <path>" );
		return false;
	}
	if( !song->guiSaveProject() )
	{
		error = tr( "Save failed" );
		return false;
	}
	result = tr( "Saved project %1" ).arg( QFileInfo( song->projectFileName() ).fileName() );
	return true;
}

bool AgentControlService::saveProjectAs( const QString& path, QString& result, QString& error )
{
	Song *song = Engine::getSong();
	if( song == nullptr )
	{
		error = tr( "No active song" );
		return false;
	}
	const QString fullPath = QFileInfo( path ).isAbsolute()
		? QDir::cleanPath( path )
		: QDir::current().absoluteFilePath( path );
	if( !song->guiSaveProjectAs( fullPath ) )
	{
		error = tr( "Save failed for %1" ).arg( fullPath );
		return false;
	}
	result = tr( "Saved project as %1" ).arg( fullPath );
	return true;
}

bool AgentControlService::showWindowCommand( const QString& windowName, QString& result, QString& error )
{
	auto *guiApp = gui::getGUI();
	if( guiApp == nullptr || guiApp->mainWindow() == nullptr )
	{
		error = tr( "GUI is not ready" );
		return false;
	}

	QString normalized = normalizeName( windowName );
	// Accept common feature labels and human aliases used in docs/manual text.
	if( normalized == "fxmixer" || normalized == "fxmixereffects" ||
		normalized == "mixereffects" || normalized == "effectsmixer" ||
		normalized == "effectchain" || normalized == "effects" )
	{
		normalized = "mixer";
	}
	else if( normalized == "songeditortracks" || normalized == "songarrangement" )
	{
		normalized = "songeditor";
	}
	else if( normalized == "pianorolleditor" || normalized == "melodyeditor" )
	{
		normalized = "pianoroll";
	}
	else if( normalized == "automationeditorandcontrollerrack" || normalized == "automationcontrollers" )
	{
		normalized = "automationeditor";
	}

	const char *slot = nullptr;
	if( normalized == "songeditor" )
	{
		slot = "toggleSongEditorWin";
	}
	else if( normalized == "patterneditor" )
	{
		slot = "togglePatternEditorWin";
	}
	else if( normalized == "pianoroll" )
	{
		slot = "togglePianoRollWin";
	}
	else if( normalized == "automationeditor" )
	{
		slot = "toggleAutomationEditorWin";
	}
	else if( normalized == "mixer" )
	{
		slot = "toggleMixerWin";
	}
	else if( normalized == "controllerrack" )
	{
		slot = "toggleControllerRack";
	}
	else if( normalized == "projectnotes" )
	{
		slot = "toggleProjectNotesWin";
	}
	else if( normalized == "microtuner" )
	{
		slot = "toggleMicrotunerWin";
	}
	else
	{
		error = tr( "Unknown window: %1" ).arg( windowName );
		return false;
	}

	bool invoked = false;
	if( normalized == "patterneditor" )
	{
		invoked = QMetaObject::invokeMethod( guiApp->mainWindow(), slot, Qt::DirectConnection,
			Q_ARG( bool, true ) );
	}
	else
	{
		invoked = QMetaObject::invokeMethod( guiApp->mainWindow(), slot, Qt::DirectConnection );
	}

	if( !invoked )
	{
		error = tr( "Failed to show %1" ).arg( windowName );
		return false;
	}
	result = tr( "Toggled %1" ).arg( windowName );
	return true;
}

bool AgentControlService::showToolCommand( const QString& toolName, QString& result, QString& error )
{
	const QString desired = normalizeName( toolName );
	for( const Plugin::Descriptor* desc : getPluginFactory()->descriptors( Plugin::Type::Tool ) )
	{
		const QString byName = normalizeName( QString::fromUtf8( desc->name ) );
		const QString byDisplayName = normalizeName( QString::fromUtf8( desc->displayName ) );
		if( desired != byName && desired != byDisplayName )
		{
			continue;
		}

		auto *tool = ToolPlugin::instantiate( QString::fromUtf8( desc->name ), nullptr );
		if( tool == nullptr )
		{
			error = tr( "Failed to instantiate tool %1" ).arg( toolName );
			return false;
		}
		auto *view = tool->createView( gui::getGUI()->mainWindow() );
		if( view == nullptr )
		{
			delete tool;
			error = tr( "Failed to create view for tool %1" ).arg( toolName );
			return false;
		}
		view->show();
		if( view->parentWidget() )
		{
			view->parentWidget()->show();
		}
		view->setFocus();
		result = tr( "Opened tool %1" ).arg( QString::fromUtf8( desc->displayName ) );
		return true;
	}

	error = tr( "Unknown tool: %1" ).arg( toolName );
	return false;
}

bool AgentControlService::createTrack( Track::Type type, QString& result, QString& error )
{
	Song *song = Engine::getSong();
	if( song == nullptr )
	{
		error = tr( "No active song" );
		return false;
	}

	Track *track = Track::create( type, song );
	if( track == nullptr )
	{
		error = tr( "Failed to create track" );
		return false;
	}

	result = tr( "Created %1" ).arg( track->name() );
	return true;
}

bool AgentControlService::createInstrumentTrack( const QString& pluginName, QString& result, QString& error )
{
	Song *song = Engine::getSong();
	auto *guiApp = gui::getGUI();
	if( song == nullptr || guiApp == nullptr || guiApp->mainWindow() == nullptr )
	{
		error = tr( "GUI is not ready" );
		return false;
	}

	const QString requested = normalizeName( pluginName );
	QString resolvedPlugin;
	QString displayName = pluginName.trimmed();
	for( const Plugin::Descriptor* desc : getPluginFactory()->descriptors( Plugin::Type::Instrument ) )
	{
		const QString byName = normalizeName( QString::fromUtf8( desc->name ) );
		const QString byDisplayName = normalizeName( QString::fromUtf8( desc->displayName ) );
		if( requested == byName || requested == byDisplayName )
		{
			resolvedPlugin = QString::fromUtf8( desc->name );
			displayName = QString::fromUtf8( desc->displayName );
			break;
		}
	}
	if( resolvedPlugin.isEmpty() )
	{
		error = tr( "Unknown instrument: %1" ).arg( pluginName );
		return false;
	}

	QTimer::singleShot( 0, guiApp->mainWindow(), [song, resolvedPlugin]()
	{
		auto *track = dynamic_cast<InstrumentTrack *>( Track::create( Track::Type::Instrument, song ) );
		if( track != nullptr )
		{
			track->loadInstrument( resolvedPlugin );
		}
	} );
	m_lastLoadedInstrument = resolvedPlugin;
	result = tr( "Queued instrument %1" ).arg( displayName );
	return true;
}

bool AgentControlService::importFromDownloads( const QString& fileName, QString& error )
{
	return importAudioFile( resolveDownloadsFile( fileName ), error );
}

bool AgentControlService::importAudioFile( const QString& path, QString& error )
{
	const QString fullPath = canonicalPath( path );
	if( fullPath.isEmpty() )
	{
		error = tr( "File not found: %1" ).arg( path );
		return false;
	}
	auto *track = createSampleTrack( QFileInfo( fullPath ).fileName() );
	if( track == nullptr )
	{
		error = tr( "Failed to create sample track" );
		return false;
	}

	auto *clip = dynamic_cast<SampleClip *>( track->createClip( TimePos( 0 ) ) );
	if( clip == nullptr )
	{
		error = tr( "Failed to create sample clip" );
		return false;
	}

	clip->setSampleFile( fullPath );
	clip->updateLength();
	m_lastImportedAudioPath = fullPath;
	return true;
}

bool AgentControlService::importProjectFile( const QString& path, QString& error )
{
	const QString fullPath = canonicalPath( path );
	if( fullPath.isEmpty() )
	{
		error = tr( "File not found: %1" ).arg( path );
		return false;
	}
	Song *song = Engine::getSong();
	if( song == nullptr )
	{
		error = tr( "No active song" );
		return false;
	}
	ImportFilter::import( fullPath, song );
	return true;
}

bool AgentControlService::addKickPattern( QString& error )
{
	const QString samplePath = defaultKickSample();
	if( samplePath.isEmpty() )
	{
		error = tr( "Kick sample missing" );
		return false;
	}

	auto *track = createSampleTrack( tr( "Agent 808" ) );
	if( track == nullptr )
	{
		error = tr( "Failed to create sample track" );
		return false;
	}

	const int stepTicks = DefaultTicksPerBar / DefaultStepsPerBar;
	const int steps[] = { 0, 4, 8, 12 };
	for( int s : steps )
	{
		addSampleClip( track, samplePath, s * stepTicks );
	}
	return true;
}

bool AgentControlService::addSnarePattern( QString& error )
{
	const QString samplePath = defaultSnareSample();
	if( samplePath.isEmpty() )
	{
		error = tr( "Snare sample missing" );
		return false;
	}

	auto *track = createSampleTrack( tr( "Agent Snare" ) );
	if( track == nullptr )
	{
		error = tr( "Failed to create sample track" );
		return false;
	}

	const int stepTicks = DefaultTicksPerBar / DefaultStepsPerBar;
	const int steps[] = { 4, 12 };
	for( int s : steps )
	{
		addSampleClip( track, samplePath, s * stepTicks );
	}
	return true;
}

bool AgentControlService::addHiHatPattern( QString& error )
{
	const QString samplePath = defaultHiHatSample();
	if( samplePath.isEmpty() )
	{
		error = tr( "Hi-hat sample missing" );
		return false;
	}

	auto *track = createSampleTrack( tr( "Agent Hi-Hat" ) );
	if( track == nullptr )
	{
		error = tr( "Failed to create sample track" );
		return false;
	}

	const int stepTicks = DefaultTicksPerBar / DefaultStepsPerBar;
	for( int step = 0; step < DefaultStepsPerBar; step += 2 )
	{
		addSampleClip( track, samplePath, step * stepTicks );
	}
	return true;
}

bool AgentControlService::addCrashPattern( QString& error )
{
	const QString samplePath = defaultCrashSample();
	if( samplePath.isEmpty() )
	{
		error = tr( "Crash sample missing" );
		return false;
	}

	auto *track = createSampleTrack( tr( "Agent Crash" ) );
	if( track == nullptr )
	{
		error = tr( "Failed to create sample track" );
		return false;
	}

	addSampleClip( track, samplePath, 0 );
	return true;
}

bool AgentControlService::addRidePattern( QString& error )
{
	const QString samplePath = defaultRideSample();
	if( samplePath.isEmpty() )
	{
		error = tr( "Ride sample missing" );
		return false;
	}

	auto *track = createSampleTrack( tr( "Agent Ride" ) );
	if( track == nullptr )
	{
		error = tr( "Failed to create sample track" );
		return false;
	}

	const int stepTicks = DefaultTicksPerBar / DefaultStepsPerBar;
	const int steps[] = { 0, 4, 8, 12 };
	for( int s : steps )
	{
		addSampleClip( track, samplePath, s * stepTicks );
	}
	return true;
}

bool AgentControlService::addEffectToTrack( const QString& effectName, const QString& trackName, QString& result, QString& error )
{
	Track *track = trackName.isEmpty()
		? findLastTrackOfTypes( { Track::Type::Instrument, Track::Type::Sample } )
		: findTrackByName( trackName );
	if( track == nullptr )
	{
		error = tr( "Track not found: %1" ).arg( trackName.isEmpty() ? tr( "last audio track" ) : trackName );
		return false;
	}

	if( effectChainForTrack( track ) == nullptr )
	{
		error = tr( "Track %1 does not support effects" ).arg( track->name() );
		return false;
	}

	QString displayName = effectName.trimmed();
	const QString resolvedEffect = resolveEffectPlugin( effectName, displayName );
	if( resolvedEffect.isEmpty() )
	{
		error = tr( "Unknown effect: %1" ).arg( effectName );
		return false;
	}

	const QString targetTrackName = track->name();
	auto *guiApp = gui::getGUI();
	if( guiApp == nullptr || guiApp->mainWindow() == nullptr )
	{
		error = tr( "GUI is not ready" );
		return false;
	}

	QTimer::singleShot( 0, guiApp->mainWindow(), [this, resolvedEffect, targetTrackName]()
	{
		Track *targetTrack = findTrackByName( targetTrackName );
		if( targetTrack == nullptr )
		{
			return;
		}

		EffectChain *chain = effectChainForTrack( targetTrack );
		if( chain == nullptr )
		{
			return;
		}

		if( auto *effect = Effect::instantiate( resolvedEffect, chain, nullptr ) )
		{
			chain->appendEffect( effect );
		}
	} );
	result = tr( "Queued effect %1 for %2" ).arg( displayName, targetTrackName );
	return true;
}

bool AgentControlService::removeEffectFromTrack( const QString& effectName, const QString& trackName, QString& result, QString& error )
{
	Track *track = trackName.isEmpty()
		? findLastTrackOfTypes( { Track::Type::Instrument, Track::Type::Sample } )
		: findTrackByName( trackName );
	if( track == nullptr )
	{
		error = tr( "Track not found: %1" ).arg( trackName.isEmpty() ? tr( "last audio track" ) : trackName );
		return false;
	}

	EffectChain *chain = effectChainForTrack( track );
	if( chain == nullptr )
	{
		error = tr( "Track %1 does not support effects" ).arg( track->name() );
		return false;
	}

	const QString requested = normalizeName( effectName );
	QString resolvedDisplay;
	const QString resolvedEffect = normalizeName( resolveEffectPlugin( effectName, resolvedDisplay ) );
	for( Effect *effect : chain->effects() )
	{
		const QString byName = normalizeName( QString::fromUtf8( effect->descriptor()->name ) );
		const QString byDisplayName = normalizeName( effect->displayName() );
		const bool fuzzyRequested = requested.size() >= 4 &&
			( byName.contains( requested ) || byDisplayName.contains( requested ) );
		const bool canonicalResolved = !resolvedEffect.isEmpty() && byName == resolvedEffect;
		if( requested == byName || requested == byDisplayName || fuzzyRequested || canonicalResolved )
		{
			const QString removedName = effect->displayName();
			chain->removeEffect( effect );
			delete effect;
			result = tr( "Removed effect %1 from %2" ).arg( removedName, track->name() );
			return true;
		}
	}

	error = tr( "Effect %1 not found on %2" ).arg( effectName, track->name() );
	return false;
}

Track* AgentControlService::findTrackByName( const QString& trackName ) const
{
	Song *song = Engine::getSong();
	if( song == nullptr )
	{
		return nullptr;
	}

	const QString wanted = normalizeName( trackName );
	const auto &tracks = song->tracks();
	for( auto it = tracks.rbegin(); it != tracks.rend(); ++it )
	{
		if( normalizeName( (*it)->name() ) == wanted )
		{
			return *it;
		}
	}
	return nullptr;
}

Track* AgentControlService::findLastTrackOfTypes( const QList<Track::Type>& types ) const
{
	Song *song = Engine::getSong();
	if( song == nullptr )
	{
		return nullptr;
	}

	const auto &tracks = song->tracks();
	for( auto it = tracks.rbegin(); it != tracks.rend(); ++it )
	{
		if( types.contains( (*it)->type() ) )
		{
			return *it;
		}
	}
	return nullptr;
}

InstrumentTrack* AgentControlService::findInstrumentTrack( const QString& trackName ) const
{
	return dynamic_cast<InstrumentTrack *>( findTrackByName( trackName ) );
}

InstrumentTrack* AgentControlService::findLastSlicerTrack() const
{
	Song *song = Engine::getSong();
	if( song == nullptr )
	{
		return nullptr;
	}

	const auto &tracks = song->tracks();
	for( auto it = tracks.rbegin(); it != tracks.rend(); ++it )
	{
		auto *instrumentTrack = dynamic_cast<InstrumentTrack *>( *it );
		if( instrumentTrack == nullptr || instrumentTrack->instrument() == nullptr )
		{
			continue;
		}

		const QString pluginName = normalizeName( instrumentTrack->instrument()->nodeName() );
		const QString displayName = normalizeName( instrumentTrack->instrumentName() );
		if( pluginName == "slicert" || displayName.contains( "slicer" ) )
		{
			return instrumentTrack;
		}
	}

	return nullptr;
}

bool AgentControlService::ensureSlicerTrack( InstrumentTrack*& track, bool createIfMissing, QString& error )
{
	track = nullptr;

	if( !m_selectedTrackName.isEmpty() )
	{
		auto *selectedTrack = dynamic_cast<InstrumentTrack *>( findTrackByName( m_selectedTrackName ) );
		if( selectedTrack != nullptr && selectedTrack->instrument() != nullptr &&
			normalizeName( selectedTrack->instrument()->nodeName() ) == "slicert" )
		{
			track = selectedTrack;
			return true;
		}
	}

	track = findLastSlicerTrack();
	if( track != nullptr )
	{
		m_selectedTrackName = track->name();
		return true;
	}

	if( !createIfMissing )
	{
		error = tr( "No slicer track is available. Say 'open slicer' first." );
		return false;
	}

	Song *song = Engine::getSong();
	if( song == nullptr )
	{
		error = tr( "No active song" );
		return false;
	}

	track = dynamic_cast<InstrumentTrack *>( Track::create( Track::Type::Instrument, song ) );
	if( track == nullptr )
	{
		error = tr( "Failed to create slicer track" );
		return false;
	}

	if( track->loadInstrument( "slicert" ) == nullptr )
	{
		error = tr( "Failed to load slicer instrument" );
		return false;
	}

	m_selectedTrackName = track->name();
	return true;
}

bool AgentControlService::focusInstrumentTrackWindow( InstrumentTrack* track, QString& error )
{
	if( track == nullptr )
	{
		error = tr( "No slicer track is available" );
		return false;
	}

	auto *guiApp = gui::getGUI();
	if( guiApp == nullptr || guiApp->songEditor() == nullptr || guiApp->songEditor()->m_editor == nullptr )
	{
		error = tr( "GUI is not ready" );
		return false;
	}

	auto *songEditorWindow = guiApp->songEditor();
	songEditorWindow->show();
	if( songEditorWindow->parentWidget() )
	{
		songEditorWindow->parentWidget()->show();
		songEditorWindow->parentWidget()->raise();
		songEditorWindow->parentWidget()->activateWindow();
	}

	for( int attempt = 0; attempt < 8; ++attempt )
	{
		for( auto *trackView : songEditorWindow->m_editor->trackViews() )
		{
			if( trackView == nullptr || trackView->getTrack() != track )
			{
				continue;
			}

			auto *instrumentView = dynamic_cast<gui::InstrumentTrackView *>( trackView );
			if( instrumentView == nullptr )
			{
				continue;
			}

			QMetaObject::invokeMethod( instrumentView, "toggleInstrumentWindow",
				Qt::DirectConnection, Q_ARG( bool, true ) );
			auto *trackWindow = instrumentView->getInstrumentTrackWindow();
			if( trackWindow != nullptr )
			{
				trackWindow->show();
				trackWindow->raise();
				trackWindow->setFocus();
				if( trackWindow->parentWidget() )
				{
					trackWindow->parentWidget()->show();
					trackWindow->parentWidget()->raise();
					trackWindow->parentWidget()->activateWindow();
				}
			}
			return true;
		}

		QCoreApplication::processEvents( QEventLoop::AllEvents, 20 );
		QThread::msleep( 60 );
	}

	error = tr( "Could not focus slicer window yet. Try again in a moment." );
	return false;
}

bool AgentControlService::loadFileIntoSlicer( const QString& fileQuery, QString& result, QString& error )
{
	InstrumentTrack *track = nullptr;
	if( !ensureSlicerTrack( track, true, error ) )
	{
		return false;
	}

	QString fullPath = canonicalPath( fileQuery );
	if( fullPath.isEmpty() )
	{
		fullPath = resolveDownloadsAudioQuery( fileQuery );
	}
	if( fullPath.isEmpty() )
	{
		error = tr( "Audio file not found: %1" ).arg( fileQuery );
		return false;
	}
	if( track->instrument() == nullptr )
	{
		error = tr( "Slicer instrument is not loaded" );
		return false;
	}

	track->instrument()->loadFile( fullPath );
	m_selectedTrackName = track->name();
	m_lastImportedAudioPath = fullPath;
	result = tr( "Loaded %1 into slicer" ).arg( QFileInfo( fullPath ).fileName() );
	return true;
}

bool AgentControlService::sliceSlicerEqual( int segments, QString& result, QString& error )
{
	InstrumentTrack *track = nullptr;
	if( !ensureSlicerTrack( track, false, error ) )
	{
		return false;
	}
	if( track->instrument() == nullptr || normalizeName( track->instrument()->nodeName() ) != "slicert" )
	{
		error = tr( "Selected track is not a slicer track" );
		return false;
	}

	const int boundedSegments = qBound( 1, segments, 128 );
	QDomDocument document;
	QDomElement trackState = document.createElement( "track" );
	document.appendChild( trackState );
	track->saveTrackSpecificSettings( document, trackState, false );

	QDomElement instrumentState = trackState.firstChildElement( "instrument" );
	if( instrumentState.isNull() )
	{
		error = tr( "Slicer settings are not available" );
		return false;
	}

	QDomElement pluginState = instrumentState.firstChildElement();
	if( pluginState.isNull() )
	{
		error = tr( "Slicer plugin state is missing" );
		return false;
	}

	pluginState.setAttribute( "totalSlices", boundedSegments + 1 );
	for( int i = 0; i <= boundedSegments; ++i )
	{
		const double ratio = static_cast<double>( i ) / static_cast<double>( boundedSegments );
		pluginState.setAttribute( QString( "slice_%1" ).arg( i ),
			QString::number( ratio, 'f', 8 ) );
	}

	track->loadTrackSpecificSettings( trackState );
	result = tr( "Set slicer to %1 equal segments" ).arg( boundedSegments );
	return true;
}

bool AgentControlService::sliceSlicerByTransients( QString& result, QString& error )
{
	InstrumentTrack *track = nullptr;
	if( !ensureSlicerTrack( track, false, error ) )
	{
		return false;
	}
	if( track->instrument() == nullptr || normalizeName( track->instrument()->nodeName() ) != "slicert" )
	{
		error = tr( "Selected track is not a slicer track" );
		return false;
	}

	if( !QMetaObject::invokeMethod( track->instrument(), "updateSlices", Qt::DirectConnection ) )
	{
		error = tr( "Failed to trigger transient slice detection" );
		return false;
	}

	result = tr( "Updated slicer using transient detection" );
	return true;
}

SampleTrack* AgentControlService::createSampleTrack( const QString& name ) const
{
	Song *song = Engine::getSong();
	if( song == nullptr )
	{
		return nullptr;
	}
	auto *track = dynamic_cast<SampleTrack *>( Track::create( Track::Type::Sample, song ) );
	if( track != nullptr && !name.isEmpty() )
	{
		track->setName( name );
	}
	return track;
}

EffectChain* AgentControlService::effectChainForTrack( Track* track ) const
{
	if( auto *instrumentTrack = dynamic_cast<InstrumentTrack *>( track ) )
	{
		return instrumentTrack->audioBusHandle()->effects();
	}
	if( auto *sampleTrack = dynamic_cast<SampleTrack *>( track ) )
	{
		return sampleTrack->audioBusHandle()->effects();
	}
	return nullptr;
}

bool AgentControlService::addSampleClip( SampleTrack *track, const QString& samplePath, int tickPos )
{
	auto *clip = dynamic_cast<SampleClip *>( track->createClip( TimePos( tickPos ) ) );
	if( clip == nullptr )
	{
		return false;
	}
	clip->setSampleFile( samplePath );
	clip->updateLength();
	return true;
}

QString AgentControlService::extractAudioQuery( const QString& rawText, const QStringList& tokens ) const
{
	const QString normalized = normalizeName( rawText );
	if( ( normalized.contains( "thissample" ) || normalized.contains( "thisaudio" ) ||
		normalized.contains( "thatsample" ) || normalized.contains( "thataudio" ) ) &&
		!m_lastImportedAudioPath.isEmpty() && QFileInfo::exists( m_lastImportedAudioPath ) )
	{
		return m_lastImportedAudioPath;
	}

	const QRegularExpression quotedPathPattern(
		R"(["']([^"']+\.(?:wav|wave|aif|aiff|flac|ogg|mp3|m4a))["'])",
		QRegularExpression::CaseInsensitiveOption );
	auto match = quotedPathPattern.match( rawText );
	if( match.hasMatch() )
	{
		return match.captured( 1 ).trimmed();
	}

	const QRegularExpression fileWithExtPattern(
		R"(([^\s,]+?\.(?:wav|wave|aif|aiff|flac|ogg|mp3|m4a)))",
		QRegularExpression::CaseInsensitiveOption );
	match = fileWithExtPattern.match( rawText );
	if( match.hasMatch() )
	{
		return match.captured( 1 ).trimmed();
	}

	if( !tokens.isEmpty() && ( tokens[0].compare( "import", Qt::CaseInsensitive ) == 0 ||
		tokens[0].compare( "load", Qt::CaseInsensitive ) == 0 ) )
	{
		QString query = joinTokens( tokens, 1 );
		query.remove( QRegularExpression( R"(\b(?:into|to)\s+slicer\b)",
			QRegularExpression::CaseInsensitiveOption ) );
		query.remove( QRegularExpression( R"(\bfrom\s+(?:my\s+)?downloads?(?:\s+folder)?\b.*$)",
			QRegularExpression::CaseInsensitiveOption ) );
		query = query.trimmed();
		if( !query.isEmpty() && query.compare( "audio", Qt::CaseInsensitive ) != 0 &&
			query.compare( "file", Qt::CaseInsensitive ) != 0 )
		{
			return query;
		}
	}

	return {};
}

QString AgentControlService::resolveDownloadsFile( const QString& fileName ) const
{
	if( fileName.isEmpty() )
	{
		return {};
	}

	QString downloads = QStandardPaths::writableLocation( QStandardPaths::DownloadLocation );
	if( downloads.isEmpty() )
	{
		downloads = QDir::homePath() + "/Downloads";
	}
	const QString absPath = QDir( downloads ).filePath( fileName );
	return QFile::exists( absPath ) ? absPath : QString{};
}

QString AgentControlService::resolveDownloadsAudioQuery( const QString& query ) const
{
	QString cleaned = query.trimmed();
	if( cleaned.isEmpty() )
	{
		return {};
	}
	cleaned.remove( '"' );
	cleaned.remove( '\'' );
	cleaned = cleaned.trimmed();

	const QString direct = resolveDownloadsFile( cleaned );
	if( !direct.isEmpty() )
	{
		return direct;
	}

	QString downloads = QStandardPaths::writableLocation( QStandardPaths::DownloadLocation );
	if( downloads.isEmpty() )
	{
		downloads = QDir::homePath() + "/Downloads";
	}
	QDir downloadsDir( downloads );
	if( !downloadsDir.exists() )
	{
		return {};
	}

	const QString wanted = normalizeName( cleaned );
	QString partialMatch;
	const QFileInfoList files = downloadsDir.entryInfoList( QDir::Files | QDir::Readable );
	for( const QFileInfo& info : files )
	{
		const QString suffix = info.suffix().toLower();
		if( suffix != "wav" && suffix != "wave" && suffix != "aif" && suffix != "aiff" &&
			suffix != "flac" && suffix != "ogg" && suffix != "mp3" && suffix != "m4a" )
		{
			continue;
		}

		const QString fileName = normalizeName( info.fileName() );
		const QString baseName = normalizeName( info.completeBaseName() );
		if( !wanted.isEmpty() && ( fileName == wanted || baseName == wanted ) )
		{
			return info.absoluteFilePath();
		}
		if( partialMatch.isEmpty() && !wanted.isEmpty() &&
			( fileName.contains( wanted ) || baseName.contains( wanted ) || wanted.contains( baseName ) ) )
		{
			partialMatch = info.absoluteFilePath();
		}
	}

	return partialMatch;
}

QString AgentControlService::defaultKickSample() const
{
	const QString base = ConfigManager::inst()->factorySamplesDir();
	const QString candidate = QDir( base ).filePath( "drums/bassdrum04.ogg" );
	return QFile::exists( candidate ) ? candidate : QString{};
}

QString AgentControlService::defaultSnareSample() const
{
	const QString base = ConfigManager::inst()->factorySamplesDir();
	const QString preferred = QDir( base ).filePath( "drums/snare01.ogg" );
	if( QFile::exists( preferred ) )
	{
		return preferred;
	}
	const QString fallback = QDir( base ).filePath( "drums/snare03.ogg" );
	return QFile::exists( fallback ) ? fallback : QString{};
}

QString AgentControlService::defaultHiHatSample() const
{
	const QString base = ConfigManager::inst()->factorySamplesDir();
	const QString preferred = QDir( base ).filePath( "drums/hihat_closed01.ogg" );
	if( QFile::exists( preferred ) )
	{
		return preferred;
	}
	const QString fallback = QDir( base ).filePath( "drums/hihat_closed03.ogg" );
	return QFile::exists( fallback ) ? fallback : QString{};
}

QString AgentControlService::defaultCrashSample() const
{
	const QString base = ConfigManager::inst()->factorySamplesDir();
	const QString preferred = QDir( base ).filePath( "drums/crash01.ogg" );
	if( QFile::exists( preferred ) )
	{
		return preferred;
	}
	const QString fallback = QDir( base ).filePath( "drums/crash02.ogg" );
	return QFile::exists( fallback ) ? fallback : QString{};
}

QString AgentControlService::defaultRideSample() const
{
	const QString base = ConfigManager::inst()->factorySamplesDir();
	const QString preferred = QDir( base ).filePath( "drums/ride01.ogg" );
	if( QFile::exists( preferred ) )
	{
		return preferred;
	}
	const QString fallback = QDir( base ).filePath( "drums/ride02.ogg" );
	return QFile::exists( fallback ) ? fallback : QString{};
}

QString AgentControlService::canonicalPath( const QString& path ) const
{
	if( path.isEmpty() )
	{
		return {};
	}

	const QFileInfo absoluteInfo( path );
	if( absoluteInfo.isAbsolute() && absoluteInfo.exists() )
	{
		return absoluteInfo.canonicalFilePath();
	}

	const QString fromDownloads = resolveDownloadsFile( path );
	if( !fromDownloads.isEmpty() )
	{
		return fromDownloads;
	}

	QFileInfo relativeInfo( QDir::current().absoluteFilePath( path ) );
	if( relativeInfo.exists() )
	{
		return relativeInfo.canonicalFilePath();
	}

	return {};
}

QString AgentControlService::joinTokens( const QStringList& tokens, int startIndex ) const
{
	return startIndex < tokens.size() ? tokens.mid( startIndex ).join( " " ).trimmed() : QString{};
}

QString AgentControlService::normalizeName( const QString& text ) const
{
	QString out;
	out.reserve( text.size() );
	for( const QChar ch : text.toLower() )
	{
		if( ch.isLetterOrNumber() )
		{
			out.append( ch );
		}
	}
	return out;
}

QJsonObject AgentControlService::successResponse(
	const QJsonObject& result,
	const QJsonObject& stateDelta,
	const QJsonArray& warnings ) const
{
	return QJsonObject
	{
		{ "ok", true },
		{ "result", result },
		{ "state_delta", stateDelta },
		{ "warnings", warnings },
		{ "error_code", QJsonValue::Null },
		{ "error_message", QJsonValue::Null }
	};
}

QJsonObject AgentControlService::errorResponse(
	const QString& errorCode,
	const QString& errorMessage,
	const QJsonArray& warnings ) const
{
	return QJsonObject
	{
		{ "ok", false },
		{ "result", QJsonObject() },
		{ "state_delta", QJsonObject() },
		{ "warnings", warnings },
		{ "error_code", errorCode },
		{ "error_message", errorMessage }
	};
}

QString AgentControlService::toCommandResponseText( const QJsonObject& response ) const
{
	if( !response.value( "ok" ).toBool( false ) )
	{
		return response.value( "error_message" ).toString( tr( "Request failed" ) );
	}

	const QJsonObject result = response.value( "result" ).toObject();
	const QString message = result.value( "message" ).toString();
	if( !message.isEmpty() )
	{
		return message;
	}
	return QString::fromUtf8( QJsonDocument( result ).toJson( QJsonDocument::Compact ) );
}

QString AgentControlService::trackTypeName( Track::Type type ) const
{
	switch( type )
	{
	case Track::Type::Instrument:
		return "instrument";
	case Track::Type::Pattern:
		return "pattern";
	case Track::Type::Sample:
		return "sample";
	case Track::Type::Event:
		return "event";
	case Track::Type::Video:
		return "video";
	case Track::Type::Automation:
		return "automation";
	case Track::Type::HiddenAutomation:
		return "hidden_automation";
	case Track::Type::Count:
	default:
		return "unknown";
	}
}

int AgentControlService::trackIndex( const Track* track ) const
{
	Song* song = Engine::getSong();
	if( song == nullptr )
	{
		return -1;
	}
	const auto &tracks = song->tracks();
	for( std::size_t i = 0; i < tracks.size(); ++i )
	{
		if( tracks[i] == track )
		{
			return static_cast<int>( i );
		}
	}
	return -1;
}

QJsonArray AgentControlService::effectArrayForTrack( Track* track ) const
{
	QJsonArray effects;
	EffectChain* chain = effectChainForTrack( track );
	if( chain == nullptr )
	{
		return effects;
	}

	for( Effect* effect : chain->effects() )
	{
		QJsonObject item
		{
			{ "display_name", effect->displayName() }
		};
		if( effect->descriptor() != nullptr )
		{
			item.insert( "plugin_name", QString::fromUtf8( effect->descriptor()->name ) );
		}
		effects.append( item );
	}
	return effects;
}

QJsonObject AgentControlService::trackObject( const Track* track, int index ) const
{
	QJsonObject obj
	{
		{ "id", QString( "t_%1" ).arg( index ) },
		{ "index", index },
		{ "name", track->name() },
		{ "type", trackTypeName( track->type() ) },
		{ "muted", track->isMuted() },
		{ "solo", track->isSolo() },
		{ "clip_count", static_cast<int>( track->getClips().size() ) },
		{ "effects", effectArrayForTrack( const_cast<Track *>( track ) ) }
	};

	if( const auto *instrumentTrack = dynamic_cast<const InstrumentTrack *>( track ) )
	{
		obj.insert( "instrument_name", instrumentTrack->instrumentName() );
	}
	if( const auto *sampleTrack = dynamic_cast<const SampleTrack *>( track ) )
	{
		QJsonArray sampleFiles;
		for( auto *clip : sampleTrack->getClips() )
		{
			if( auto *sampleClip = dynamic_cast<SampleClip *>( clip ) )
			{
				sampleFiles.append( sampleClip->sampleFile() );
			}
		}
		obj.insert( "sample_files", sampleFiles );
	}
	return obj;
}

QJsonArray AgentControlService::trackArray() const
{
	QJsonArray tracksJson;
	Song* song = Engine::getSong();
	if( song == nullptr )
	{
		return tracksJson;
	}

	int index = 0;
	for( const auto *track : song->tracks() )
	{
		tracksJson.append( trackObject( track, index ) );
		++index;
	}
	return tracksJson;
}

QJsonArray AgentControlService::listPatternArray() const
{
	QJsonArray patterns;
	Song* song = Engine::getSong();
	if( song == nullptr )
	{
		return patterns;
	}

	int trackIdx = 0;
	for( auto *track : song->tracks() )
	{
		int clipIdx = 0;
		for( auto *clip : track->getClips() )
		{
			if( auto *midiClip = dynamic_cast<MidiClip *>( clip ) )
			{
				patterns.append( QJsonObject
				{
					{ "track_id", QString( "t_%1" ).arg( trackIdx ) },
					{ "track_name", track->name() },
					{ "clip_index", clipIdx },
					{ "clip_name", midiClip->name() },
					{ "start_tick", static_cast<int>( midiClip->startPosition() ) },
					{ "length_ticks", static_cast<int>( midiClip->length() ) },
					{ "note_count", static_cast<int>( midiClip->notes().size() ) }
				} );
			}
			++clipIdx;
		}
		++trackIdx;
	}
	return patterns;
}

QJsonObject AgentControlService::projectStateObject() const
{
	QJsonObject state;
	Song* song = Engine::getSong();
	if( song == nullptr )
	{
		return state;
	}

	state.insert( "project_file", song->projectFileName() );
	state.insert( "tempo", song->getTempo() );
	state.insert( "track_count", static_cast<int>( song->tracks().size() ) );
	state.insert( "selected_track", m_selectedTrackName );
	state.insert( "tracks", trackArray() );
	return state;
}

QJsonObject AgentControlService::diffState( const QJsonObject& before, const QJsonObject& after ) const
{
	QJsonObject diff;
	diff.insert( "track_count_before", before.value( "track_count" ).toInt() );
	diff.insert( "track_count_after", after.value( "track_count" ).toInt() );
	diff.insert( "tempo_before", before.value( "tempo" ).toInt() );
	diff.insert( "tempo_after", after.value( "tempo" ).toInt() );

	QSet<QString> beforeTracks;
	QSet<QString> afterTracks;
	for( const auto &value : before.value( "tracks" ).toArray() )
	{
		beforeTracks.insert( value.toObject().value( "name" ).toString() );
	}
	for( const auto &value : after.value( "tracks" ).toArray() )
	{
		afterTracks.insert( value.toObject().value( "name" ).toString() );
	}

	int added = 0;
	for( const auto &name : afterTracks )
	{
		if( !beforeTracks.contains( name ) )
		{
			++added;
		}
	}

	int removed = 0;
	for( const auto &name : beforeTracks )
	{
		if( !afterTracks.contains( name ) )
		{
			++removed;
		}
	}

	diff.insert( "tracks_added", added );
	diff.insert( "tracks_removed", removed );
	diff.insert( "selected_track_before", before.value( "selected_track" ).toString() );
	diff.insert( "selected_track_after", after.value( "selected_track" ).toString() );
	return diff;
}

Track* AgentControlService::resolveTrackRef( const QJsonObject& args ) const
{
	Song* song = Engine::getSong();
	if( song == nullptr )
	{
		return nullptr;
	}

	const QString trackId = args.value( "track_id" ).toString().trimmed();
	if( !trackId.isEmpty() )
	{
		const QRegularExpression re( "^t_(\\d+)$" );
		const auto match = re.match( trackId );
		if( match.hasMatch() )
		{
			const int idx = match.captured( 1 ).toInt();
			if( idx >= 0 && idx < static_cast<int>( song->tracks().size() ) )
			{
				return song->tracks()[idx];
			}
		}
	}

	QString trackName = args.value( "track" ).toString().trimmed();
	if( trackName.isEmpty() )
	{
		trackName = args.value( "track_name" ).toString().trimmed();
	}
	if( trackName.isEmpty() )
	{
		trackName = m_selectedTrackName;
	}
	if( trackName.isEmpty() )
	{
		return nullptr;
	}

	if( auto *exact = findTrackByName( trackName ) )
	{
		return exact;
	}

	const QString wanted = normalizeName( trackName );
	for( auto *track : song->tracks() )
	{
		if( normalizeName( track->name() ).contains( wanted ) )
		{
			return track;
		}
	}
	return nullptr;
}

InstrumentTrack* AgentControlService::resolveInstrumentTrack( const QJsonObject& args ) const
{
	return dynamic_cast<InstrumentTrack *>( resolveTrackRef( args ) );
}

SampleTrack* AgentControlService::resolveSampleTrack( const QJsonObject& args ) const
{
	return dynamic_cast<SampleTrack *>( resolveTrackRef( args ) );
}

QString AgentControlService::resolveInstrumentPlugin( const QString& pluginName, QString& displayName ) const
{
	const QString wanted = normalizeName( pluginName );
	QString fuzzyMatch;
	QString fuzzyDisplay;

	for( const Plugin::Descriptor* desc : getPluginFactory()->descriptors( Plugin::Type::Instrument ) )
	{
		const QString byName = normalizeName( QString::fromUtf8( desc->name ) );
		const QString byDisplay = normalizeName( QString::fromUtf8( desc->displayName ) );
		if( byName == wanted || byDisplay == wanted )
		{
			displayName = QString::fromUtf8( desc->displayName );
			return QString::fromUtf8( desc->name );
		}
		if( fuzzyMatch.isEmpty() && ( byName.contains( wanted ) || byDisplay.contains( wanted ) ) )
		{
			fuzzyMatch = QString::fromUtf8( desc->name );
			fuzzyDisplay = QString::fromUtf8( desc->displayName );
		}
	}

	displayName = fuzzyDisplay;
	return fuzzyMatch;
}

QString AgentControlService::resolveEffectPlugin( const QString& effectName, QString& displayName ) const
{
	const QString wanted = normalizeName( effectName );
	QString aliasWanted;
	if( wanted == QStringLiteral( "reverb" ) )
	{
		aliasWanted = QStringLiteral( "reverbsc" );
	}
	else if( wanted == QStringLiteral( "equalizer" ) || wanted == QStringLiteral( "eq" ) )
	{
		aliasWanted = QStringLiteral( "eq" );
	}
	else if( wanted == QStringLiteral( "comp" ) || wanted == QStringLiteral( "compressor" ) )
	{
		aliasWanted = QStringLiteral( "compressor" );
	}
	else if( wanted == QStringLiteral( "distortion" ) )
	{
		aliasWanted = QStringLiteral( "slewdistortion" );
	}

	QString fuzzyMatch;
	QString fuzzyDisplay;

	for( const Plugin::Descriptor* desc : getPluginFactory()->descriptors( Plugin::Type::Effect ) )
	{
		const QString byName = normalizeName( QString::fromUtf8( desc->name ) );
		const QString byDisplay = normalizeName( QString::fromUtf8( desc->displayName ) );
		if( byName == wanted || byDisplay == wanted || ( !aliasWanted.isEmpty() && byName == aliasWanted ) )
		{
			displayName = QString::fromUtf8( desc->displayName );
			return QString::fromUtf8( desc->name );
		}
		if( fuzzyMatch.isEmpty() &&
			( byName.contains( wanted ) || byDisplay.contains( wanted ) ||
			  ( !aliasWanted.isEmpty() && ( byName.contains( aliasWanted ) || byDisplay.contains( aliasWanted ) ) ) ) )
		{
			fuzzyMatch = QString::fromUtf8( desc->name );
			fuzzyDisplay = QString::fromUtf8( desc->displayName );
		}
	}

	displayName = fuzzyDisplay;
	return fuzzyMatch;
}

QJsonArray AgentControlService::availableWindows() const
{
	return QJsonArray
	{
		"song editor",
		"pattern editor",
		"piano roll",
		"automation editor",
		"mixer",
		"controller rack",
		"project notes",
		"microtuner"
	};
}

QJsonArray AgentControlService::availableTools() const
{
	QJsonArray tools;
	for( const Plugin::Descriptor* desc : getPluginFactory()->descriptors( Plugin::Type::Tool ) )
	{
		tools.append( QJsonObject
		{
			{ "name", QString::fromUtf8( desc->name ) },
			{ "display_name", QString::fromUtf8( desc->displayName ) }
		} );
	}
	return tools;
}

QJsonArray AgentControlService::availableInstruments() const
{
	QJsonArray instruments;
	for( const Plugin::Descriptor* desc : getPluginFactory()->descriptors( Plugin::Type::Instrument ) )
	{
		instruments.append( QJsonObject
		{
			{ "name", QString::fromUtf8( desc->name ) },
			{ "display_name", QString::fromUtf8( desc->displayName ) }
		} );
	}
	return instruments;
}

QJsonArray AgentControlService::availableEffects() const
{
	QJsonArray effects;
	for( const Plugin::Descriptor* desc : getPluginFactory()->descriptors( Plugin::Type::Effect ) )
	{
		effects.append( QJsonObject
		{
			{ "name", QString::fromUtf8( desc->name ) },
			{ "display_name", QString::fromUtf8( desc->displayName ) }
		} );
	}
	return effects;
}

QJsonArray AgentControlService::searchProjectAudio( const QString& query ) const
{
	QJsonArray matches;
	Song* song = Engine::getSong();
	if( song == nullptr )
	{
		return matches;
	}

	const QString wanted = query.trimmed().toLower();
	for( auto *track : song->tracks() )
	{
		auto *sampleTrack = dynamic_cast<SampleTrack *>( track );
		if( sampleTrack == nullptr )
		{
			continue;
		}
		int clipIdx = 0;
		for( auto *clip : sampleTrack->getClips() )
		{
			auto *sampleClip = dynamic_cast<SampleClip *>( clip );
			if( sampleClip == nullptr )
			{
				++clipIdx;
				continue;
			}
			const QString path = sampleClip->sampleFile();
			if( wanted.isEmpty() || path.toLower().contains( wanted ) || QFileInfo( path ).fileName().toLower().contains( wanted ) )
			{
				matches.append( QJsonObject
				{
					{ "track_name", track->name() },
					{ "clip_index", clipIdx },
					{ "sample_path", path }
				} );
			}
			++clipIdx;
		}
	}
	return matches;
}

bool AgentControlService::createSnapshot( const QString& label, QJsonObject& result, QString& error )
{
	if( Engine::getSong() == nullptr )
	{
		error = tr( "No active song" );
		return false;
	}

	Snapshot snapshot;
	snapshot.id = QString( "snap_%1" ).arg( ++m_snapshotCounter );
	snapshot.label = label.isEmpty() ? snapshot.id : label;
	snapshot.state = projectStateObject();
	snapshot.actionCounter = m_actionCounter;
	m_snapshots.insert( snapshot.id, snapshot );

	result = QJsonObject
	{
		{ "snapshot_id", snapshot.id },
		{ "label", snapshot.label },
		{ "action_counter", snapshot.actionCounter }
	};
	return true;
}

bool AgentControlService::undoLastAction( QJsonObject& result, QString& error )
{
	ProjectJournal* journal = Engine::projectJournal();
	if( journal == nullptr || !journal->canUndo() )
	{
		error = tr( "Nothing to undo" );
		return false;
	}

	journal->undo();
	if( m_actionCounter > 0 )
	{
		--m_actionCounter;
	}
	result = QJsonObject
	{
		{ "undone", true },
		{ "action_counter", m_actionCounter }
	};
	return true;
}

bool AgentControlService::rollbackSnapshot( const QString& snapshotId, QJsonObject& result, QString& error )
{
	if( snapshotId.isEmpty() || !m_snapshots.contains( snapshotId ) )
	{
		error = tr( "Snapshot not found: %1" ).arg( snapshotId );
		return false;
	}
	ProjectJournal* journal = Engine::projectJournal();
	if( journal == nullptr )
	{
		error = tr( "Project journal is unavailable" );
		return false;
	}

	const Snapshot snapshot = m_snapshots.value( snapshotId );
	int undosRequired = m_actionCounter - snapshot.actionCounter;
	if( undosRequired < 0 )
	{
		undosRequired = 0;
	}

	int applied = 0;
	while( undosRequired > 0 && journal->canUndo() )
	{
		journal->undo();
		--undosRequired;
		++applied;
	}

	m_actionCounter = snapshot.actionCounter + undosRequired;
	result = QJsonObject
	{
		{ "snapshot_id", snapshotId },
		{ "undos_applied", applied },
		{ "action_counter", m_actionCounter }
	};
	return true;
}

bool AgentControlService::diffSinceSnapshot( const QString& snapshotId, QJsonObject& result, QString& error ) const
{
	if( snapshotId.isEmpty() || !m_snapshots.contains( snapshotId ) )
	{
		error = tr( "Snapshot not found: %1" ).arg( snapshotId );
		return false;
	}

	const Snapshot snapshot = m_snapshots.value( snapshotId );
	result = QJsonObject
	{
		{ "snapshot_id", snapshotId },
		{ "diff", diffState( snapshot.state, projectStateObject() ) }
	};
	return true;
}

bool AgentControlService::loadSampleToTrack(
	const QString& samplePath,
	const QString& trackName,
	QJsonObject& result,
	QString& error )
{
	const QString fullPath = canonicalPath( samplePath );
	if( fullPath.isEmpty() )
	{
		error = tr( "File not found: %1" ).arg( samplePath );
		return false;
	}

	SampleTrack* track = nullptr;
	if( !trackName.isEmpty() )
	{
		track = dynamic_cast<SampleTrack *>( findTrackByName( trackName ) );
		if( track == nullptr )
		{
			track = createSampleTrack( trackName );
		}
	}
	else if( !m_selectedTrackName.isEmpty() )
	{
		track = dynamic_cast<SampleTrack *>( findTrackByName( m_selectedTrackName ) );
	}
	if( track == nullptr )
	{
		track = createSampleTrack( QFileInfo( fullPath ).baseName() );
	}
	if( track == nullptr )
	{
		error = tr( "Failed to create sample track" );
		return false;
	}

	if( !addSampleClip( track, fullPath, 0 ) )
	{
		error = tr( "Failed to add sample clip" );
		return false;
	}

	m_selectedTrackName = track->name();
	result = QJsonObject
	{
		{ "sample_path", fullPath },
		{ "track", trackObject( track, trackIndex( track ) ) }
	};
	return true;
}

bool AgentControlService::setTempoValue( int tempo, QJsonObject& result, QString& error )
{
	Song* song = Engine::getSong();
	if( song == nullptr )
	{
		error = tr( "No active song" );
		return false;
	}
	if( tempo < MinTempo || tempo > MaxTempo )
	{
		error = tr( "Tempo must be between %1 and %2" ).arg( MinTempo ).arg( MaxTempo );
		return false;
	}

	const int oldTempo = song->getTempo();
	song->setTempo( tempo );
	result = QJsonObject
	{
		{ "tempo_before", oldTempo },
		{ "tempo_after", song->getTempo() }
	};
	return true;
}

bool AgentControlService::renameTrack(
	const QString& trackName,
	const QString& newName,
	QJsonObject& result,
	QString& error )
{
	if( newName.isEmpty() )
	{
		error = tr( "new_name must not be empty" );
		return false;
	}

	Track* track = findTrackByName( trackName.isEmpty() ? m_selectedTrackName : trackName );
	if( track == nullptr )
	{
		error = tr( "Track not found: %1" ).arg( trackName );
		return false;
	}

	const QString oldName = track->name();
	track->setName( newName );
	m_selectedTrackName = track->name();
	result = QJsonObject
	{
		{ "old_name", oldName },
		{ "new_name", track->name() },
		{ "track", trackObject( track, trackIndex( track ) ) }
	};
	return true;
}

bool AgentControlService::selectTrack( const QString& trackName, QJsonObject& result, QString& error )
{
	Track* track = findTrackByName( trackName );
	if( track == nullptr )
	{
		error = tr( "Track not found: %1" ).arg( trackName );
		return false;
	}
	m_selectedTrackName = track->name();
	result = QJsonObject
	{
		{ "selected_track", track->name() },
		{ "track", trackObject( track, trackIndex( track ) ) }
	};
	return true;
}

bool AgentControlService::setTrackMute( const QString& trackName, bool mute, QJsonObject& result, QString& error )
{
	Track* track = findTrackByName( trackName.isEmpty() ? m_selectedTrackName : trackName );
	if( track == nullptr )
	{
		error = tr( "Track not found: %1" ).arg( trackName );
		return false;
	}
	track->setMuted( mute );
	result = QJsonObject
	{
		{ "track", track->name() },
		{ "muted", track->isMuted() }
	};
	return true;
}

bool AgentControlService::setTrackSolo( const QString& trackName, bool solo, QJsonObject& result, QString& error )
{
	Track* track = findTrackByName( trackName.isEmpty() ? m_selectedTrackName : trackName );
	if( track == nullptr )
	{
		error = tr( "Track not found: %1" ).arg( trackName );
		return false;
	}
	track->setSolo( solo );
	result = QJsonObject
	{
		{ "track", track->name() },
		{ "solo", track->isSolo() }
	};
	return true;
}

bool AgentControlService::createPatternClip( const QJsonObject& args, QJsonObject& result, QString& error )
{
	InstrumentTrack* track = resolveInstrumentTrack( args );
	if( track == nullptr )
	{
		track = dynamic_cast<InstrumentTrack *>( findLastTrackOfTypes( { Track::Type::Instrument } ) );
	}
	if( track == nullptr )
	{
		error = tr( "No instrument track available for create_pattern" );
		return false;
	}

	const int tick = args.value( "tick" ).toInt( 0 );
	Clip* clip = track->createClip( TimePos( tick ) );
	if( clip == nullptr )
	{
		error = tr( "Failed to create pattern clip" );
		return false;
	}
	const QString clipName = args.value( "name" ).toString().trimmed();
	if( !clipName.isEmpty() )
	{
		clip->setName( clipName );
	}

	m_selectedTrackName = track->name();
	result = QJsonObject
	{
		{ "track", track->name() },
		{ "clip_index", track->getClipNum( clip ) },
		{ "clip_name", clip->name() },
		{ "start_tick", tick }
	};
	return true;
}

bool AgentControlService::addNotesToPattern( const QJsonObject& args, QJsonObject& result, QString& error )
{
	InstrumentTrack* track = resolveInstrumentTrack( args );
	if( track == nullptr )
	{
		track = dynamic_cast<InstrumentTrack *>( findLastTrackOfTypes( { Track::Type::Instrument } ) );
	}
	if( track == nullptr )
	{
		error = tr( "No instrument track available for add_notes" );
		return false;
	}

	MidiClip* targetClip = nullptr;
	const int requestedClipIndex = args.value( "clip_index" ).toInt( -1 );
	if( requestedClipIndex >= 0 && requestedClipIndex < static_cast<int>( track->getClips().size() ) )
	{
		targetClip = dynamic_cast<MidiClip *>( track->getClip( static_cast<std::size_t>( requestedClipIndex ) ) );
	}
	if( targetClip == nullptr )
	{
		for( auto it = track->getClips().rbegin(); it != track->getClips().rend(); ++it )
		{
			targetClip = dynamic_cast<MidiClip *>( *it );
			if( targetClip != nullptr )
			{
				break;
			}
		}
	}
	if( targetClip == nullptr )
	{
		Clip* clip = track->createClip( TimePos( 0 ) );
		targetClip = dynamic_cast<MidiClip *>( clip );
	}
	if( targetClip == nullptr )
	{
		error = tr( "Failed to resolve target pattern clip" );
		return false;
	}

	QJsonArray notes = args.value( "notes" ).toArray();
	if( notes.isEmpty() && args.contains( "key" ) )
	{
		notes.append( QJsonObject
		{
			{ "key", args.value( "key" ).toInt( DefaultKey ) },
			{ "pos", args.value( "pos" ).toInt( 0 ) },
			{ "length", args.value( "length" ).toInt( DefaultTicksPerBar / 4 ) },
			{ "velocity", args.value( "velocity" ).toInt( 100 ) }
		} );
	}
	if( notes.isEmpty() )
	{
		error = tr( "add_notes requires notes array or key/pos/length fields" );
		return false;
	}

	int added = 0;
	for( const auto &entry : notes )
	{
		const QJsonObject noteObj = entry.toObject();
		const int key = noteObj.value( "key" ).toInt( DefaultKey );
		const int pos = noteObj.value( "pos" ).toInt( 0 );
		const int length = noteObj.value( "length" ).toInt( DefaultTicksPerBar / 4 );
		const int velocity = qBound( 1, noteObj.value( "velocity" ).toInt( 100 ), 200 );
		Note note( TimePos( length ), TimePos( pos ), key, velocity );
		targetClip->addNote( note, false );
		++added;
	}
	targetClip->updateLength();

	result = QJsonObject
	{
		{ "track", track->name() },
		{ "clip_index", track->getClipNum( targetClip ) },
		{ "notes_added", added }
	};
	return true;
}

bool AgentControlService::addStepsToPattern( const QJsonObject& args, QJsonObject& result, QString& error )
{
	InstrumentTrack* track = resolveInstrumentTrack( args );
	if( track == nullptr )
	{
		track = dynamic_cast<InstrumentTrack *>( findLastTrackOfTypes( { Track::Type::Instrument } ) );
	}
	if( track == nullptr )
	{
		error = tr( "No instrument track available for add_steps" );
		return false;
	}

	MidiClip* targetClip = nullptr;
	const int requestedClipIndex = args.value( "clip_index" ).toInt( -1 );
	if( requestedClipIndex >= 0 && requestedClipIndex < static_cast<int>( track->getClips().size() ) )
	{
		targetClip = dynamic_cast<MidiClip *>( track->getClip( static_cast<std::size_t>( requestedClipIndex ) ) );
	}
	if( targetClip == nullptr )
	{
		for( auto it = track->getClips().rbegin(); it != track->getClips().rend(); ++it )
		{
			targetClip = dynamic_cast<MidiClip *>( *it );
			if( targetClip != nullptr )
			{
				break;
			}
		}
	}
	if( targetClip == nullptr )
	{
		Clip* clip = track->createClip( TimePos( 0 ) );
		targetClip = dynamic_cast<MidiClip *>( clip );
	}
	if( targetClip == nullptr )
	{
		error = tr( "Failed to resolve target pattern clip" );
		return false;
	}

	if( args.value( "clear_existing" ).toBool( false ) )
	{
		targetClip->clearNotes();
	}

	QJsonArray steps = args.value( "steps" ).toArray();
	if( steps.isEmpty() )
	{
		steps = QJsonArray{ 0, 4, 8, 12 };
	}

	int added = 0;
	for( const auto &stepValue : steps )
	{
		targetClip->setStep( stepValue.toInt(), true );
		++added;
	}

	result = QJsonObject
	{
		{ "track", track->name() },
		{ "clip_index", track->getClipNum( targetClip ) },
		{ "steps_enabled", added }
	};
	return true;
}

} // namespace lmms
