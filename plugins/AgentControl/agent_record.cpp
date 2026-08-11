/*
 * agent_record.cpp - MIDI note capture domain tools (v2)
 *
 * Captures MIDI input through a plugin-owned MidiPort (MidiEventProcessor
 * pattern); events arrive on the MIDI driver thread and are drained into the
 * armed MidiClip on the GUI thread via a timer.
 */

#include "AgentControl.h"

#include <QJsonArray>

#include <chrono>

#include "Engine.h"
#include "InstrumentTrack.h"
#include "MidiClient.h"
#include "MidiClip.h"
#include "MidiPort.h"
#include "Song.h"

namespace lmms
{

namespace
{

qint64 recordSteadyMs()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch() ).count();
}

} // namespace

std::optional<QJsonObject> AgentControlService::dispatchRecordTool(
	const QString& tool, const QJsonObject& args )
{
	Song* song = Engine::getSong();
	if( song == nullptr ) { return errorResponse( "tool_failed", tr( "no song" ) ); }
	QJsonObject result;
	QString error;

	if( tool == "recordarm" )
	{
		QJsonObject trackArgs = args;
		trackArgs.remove( "clip_index" );
		InstrumentTrack* track = resolveInstrumentTrackOrLast( trackArgs, error );
		if( track == nullptr ) { return errorResponse( "tool_failed", error ); }

		MidiClip* clip = nullptr;
		const int requestedIndex = args.value( "clip_index" ).toInt( -1 );
		const auto& clips = track->getClips();
		if( requestedIndex >= 0 && requestedIndex < static_cast<int>( clips.size() ) )
		{
			clip = dynamic_cast<MidiClip*>( clips[static_cast<std::size_t>( requestedIndex )] );
		}
		if( clip == nullptr )
		{
			for( auto it = clips.rbegin(); it != clips.rend(); ++it )
			{
				clip = dynamic_cast<MidiClip*>( *it );
				if( clip != nullptr ) { break; }
			}
		}
		if( clip == nullptr )
		{
			clip = dynamic_cast<MidiClip*>( track->createClip( TimePos( 0 ) ) );
		}
		if( clip == nullptr ) { return errorResponse( "tool_failed", tr( "no pattern clip available" ) ); }

		if( !m_capturePort )
		{
			MidiClient* client = Engine::audioEngine()->midiClient();
			if( client == nullptr ) { return errorResponse( "not_available", tr( "no MIDI client" ) ); }
			m_capturePort = std::make_unique<MidiPort>(
				tr( "Agent Capture" ), client, &m_captureProcessor,
				song, MidiPort::Mode::Input );
			const QStringList ports = client->readablePorts();
			for( const QString& port : ports )
			{
				m_capturePort->subscribeReadablePort( port );
			}
		}
		m_capturePort->setReadable( true );
		{
			std::lock_guard<std::mutex> lock( m_captureMutex );
			m_captureQueue.clear();
		}
		m_capturePendingNotes.clear();
		m_captureTrack = track;
		m_captureClipIndex = track->getClipNum( clip );
		m_captureQuantize = 0;

		result["track"] = track->name();
		result["clip_index"] = m_captureClipIndex;
		result["message"] = tr( "recording armed; call record_start to begin" );
		return successResponse( result );
	}

	if( tool == "recorddisarm" )
	{
		m_captureActive = false;
		m_captureDrainTimer.stop();
		if( m_capturePort ) { m_capturePort->setReadable( false ); }
		{
			std::lock_guard<std::mutex> lock( m_captureMutex );
			m_captureQueue.clear();
		}
		m_capturePendingNotes.clear();
		m_captureTrack = nullptr;
		result["message"] = tr( "recording disarmed" );
		return successResponse( result );
	}

	if( tool == "recordstart" )
	{
		if( m_captureTrack == nullptr ) { return errorResponse( "tool_failed", tr( "arm recording first" ) ); }
		m_captureStartMs = recordSteadyMs();
		m_captureStartPos = song->getPlayPos( Song::PlayMode::Song );
		m_captureActive = true;
		m_captureDrainTimer.start( 15 );
		song->playAndRecord();
		result["message"] = tr( "recording started" );
		return successResponse( result );
	}

	if( tool == "recordstop" )
	{
		if( !m_captureActive && m_captureTrack == nullptr )
		{
			return errorResponse( "tool_failed", tr( "no active recording" ) );
		}
		song->stop();
		m_captureQuantize = qBound( 0, args.value( "quantize" ).toInt( 0 ), 192 );
		drainCaptureQueue();
		m_captureActive = false;
		m_captureDrainTimer.stop();
		{
			std::lock_guard<std::mutex> lock( m_captureMutex );
			m_captureQueue.clear();
		}
		const int dropped = m_capturePendingNotes.size();
		m_capturePendingNotes.clear();
		QJsonArray warnings;
		if( dropped > 0 )
		{
			warnings.append( tr( "%1 note(s) had no note-off and were dropped" ).arg( dropped ) );
		}
		result["message"] = tr( "recording stopped" );
		result["track"] = m_captureTrack != nullptr ? m_captureTrack->name() : QString();
		result["quantize"] = m_captureQuantize;
		m_captureTrack = nullptr;
		m_captureClipIndex = -1;
		return successResponse( result, QJsonObject(), warnings );
	}

	return std::nullopt;
}

} // namespace lmms
