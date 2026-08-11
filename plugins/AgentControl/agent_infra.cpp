/*
 * agent_infra.cpp - v2 tool surface infrastructure for AgentControl
 *
 * Dispatch routing, model-address resolution, introspection (describe_*),
 * generic param get/set, MIDI capture plumbing, and render lifecycle.
 * See lmmsagent/docs/TOOL_CONTRACT_V2.md for the surface spec.
 */

#include "AgentControl.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>

#include <chrono>

#include "AutomationClip.h"
#include "AutomationTrack.h"
#include "ComboBoxModel.h"
#include "ControllerConnection.h"
#include "Effect.h"
#include "EffectChain.h"
#include "EffectControls.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "MidiClip.h"
#include "MidiController.h"
#include "OutputSettings.h"
#include "ProjectNotes.h"
#include "ProjectRenderer.h"
#include "SampleTrack.h"
#include "Song.h"

namespace lmms
{

namespace
{

qint64 steadyMs()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch() ).count();
}

QString modelTypeName( AutomatableModel* model )
{
	if( model == nullptr ) { return "unknown"; }
	if( model->dynamicCast<TempoSyncKnobModel>() ) { return "temposync"; }
	if( model->dynamicCast<ComboBoxModel>() ) { return "combo"; }
	if( model->dynamicCast<FloatModel>() ) { return "float"; }
	if( model->dynamicCast<IntModel>() ) { return "int"; }
	if( model->dynamicCast<BoolModel>() ) { return "bool"; }
	return "model";
}

} // namespace


// ---------------------------------------------------------------------------
// dispatch routing
// ---------------------------------------------------------------------------

QJsonObject AgentControlService::dispatchV2Tool( const QString& tool, const QJsonObject& args )
{
	const bool isWrite = !( tool.startsWith( "describe" ) || tool.startsWith( "list" )
		|| tool.startsWith( "get" ) || tool == "readautomation"
		|| tool == "getselectionstate" || tool == "getprojectstate" );
	const QJsonObject beforeState = isWrite ? projectStateObject() : QJsonObject();

	std::optional<QJsonObject> result;

	if( ( result = dispatchProjectTool( tool, args ) ) ||
		( result = dispatchArrangementTool( tool, args ) ) ||
		( result = dispatchNoteTool( tool, args ) ) ||
		( result = dispatchPatternTool( tool, args ) ) ||
		( result = dispatchSampleTool( tool, args ) ) ||
		( result = dispatchInstrumentTool( tool, args ) ) ||
		( result = dispatchSoundTool( tool, args ) ) ||
		( result = dispatchEffectTool( tool, args ) ) ||
		( result = dispatchMixerTool( tool, args ) ) ||
		( result = dispatchAutomationTool( tool, args ) ) ||
		( result = dispatchControllerTool( tool, args ) ) ||
		( result = dispatchRenderTool( tool, args ) ) ||
		( result = dispatchRecordTool( tool, args ) ) ||
		( result = dispatchMiscTool( tool, args ) ) )
	{
		QJsonObject response = *result;
		if( isWrite && response.value( "ok" ).toBool( false ) )
		{
			const QJsonObject afterState = projectStateObject();
			response["state_delta"] = diffState( beforeState, afterState );
			++m_actionCounter;
		}
		return response;
	}

	return errorResponse( "unknown_tool", tr( "Unknown tool: %1" ).arg( tool ) );
}


// ---------------------------------------------------------------------------
// MIDI capture (record domain)
// ---------------------------------------------------------------------------

void AgentControlService::CaptureProcessor::processInEvent(
	const MidiEvent& event, const TimePos&, f_cnt_t )
{
	if( m_service == nullptr || !m_service->m_captureActive )
	{
		return;
	}
	if( event.type() != MidiNoteOn && event.type() != MidiNoteOff )
	{
		return;
	}
	if( event.type() == MidiNoteOn && event.velocity() == 0 )
	{
		return; // treated as note-off below
	}

	CapturedEvent captured;
	captured.type = static_cast<quint8>( event.type() );
	captured.key = static_cast<quint8>( event.key() );
	captured.velocity = event.velocity();
	captured.ms = steadyMs() - m_service->m_captureStartMs;

	std::lock_guard<std::mutex> lock( m_service->m_captureMutex );
	if( m_service->m_captureQueue.size() >= 8192 )
	{
		m_service->m_captureQueue.removeFirst();
	}
	m_service->m_captureQueue.append( captured );
}

void AgentControlService::drainCaptureQueue()
{
	if( !m_captureActive )
	{
		return;
	}
	QVector<CapturedEvent> events;
	{
		std::lock_guard<std::mutex> lock( m_captureMutex );
		events.swap( m_captureQueue );
	}
	if( events.isEmpty() )
	{
		return;
	}

	Song* song = Engine::getSong();
	if( song == nullptr || m_captureTrack == nullptr )
	{
		m_captureActive = false;
		return;
	}

	MidiClip* clip = nullptr;
	if( m_captureClipIndex >= 0 )
	{
		const auto& clips = m_captureTrack->getClips();
		if( m_captureClipIndex < static_cast<int>( clips.size() ) )
		{
			clip = dynamic_cast<MidiClip*>( clips[static_cast<std::size_t>( m_captureClipIndex )] );
		}
	}
	if( clip == nullptr )
	{
		for( auto it = m_captureTrack->getClips().rbegin(); it != m_captureTrack->getClips().rend(); ++it )
		{
			clip = dynamic_cast<MidiClip*>( *it );
			if( clip != nullptr ) { break; }
		}
	}
	if( clip == nullptr )
	{
		clip = dynamic_cast<MidiClip*>( m_captureTrack->createClip( TimePos( 0 ) ) );
	}
	if( clip == nullptr )
	{
		m_captureActive = false;
		return;
	}

	const int bpm = qMax( 1, song->tempoModel().value() );
	const int ticksPerBar = TimePos::ticksPerBar();
	const double msPerTick = 60000.0 / ( bpm * ( ticksPerBar / 4.0 ) );

	int added = 0;
	for( const CapturedEvent& ev : events )
	{
		const tick_t tick = m_captureStartPos.getTicks()
			+ static_cast<tick_t>( ev.ms / msPerTick );
		if( ev.type == MidiNoteOn )
		{
			m_capturePendingNotes.insert( ev.key, tick );
		}
		else
		{
			auto it = m_capturePendingNotes.find( ev.key );
			if( it == m_capturePendingNotes.end() ) { continue; }
			const tick_t start = it.value();
			m_capturePendingNotes.erase( it );
			tick_t length = qMax( static_cast<tick_t>( 1 ), tick - start );
			Note note( TimePos( length ), TimePos( start ), ev.key,
				static_cast<volume_t>( qBound( 0, static_cast<int>( ev.velocity ), 127 ) ) * DefaultVolume / 127 );
			if( m_captureQuantize > 0 )
			{
				const int tpn = qMax( 1, ticksPerBar / m_captureQuantize );
				note.quantizePos( tpn );
				note.setLength( qMax( 1, note.length().getTicks() / tpn * tpn ) );
			}
			clip->addNote( note, false );
			++added;
		}
	}
	clip->updateLength();

	QJsonArray warnings;
	if( added > 0 )
	{
		warnings.append( tr( "captured %1 note(s) into clip %2" ).arg( added ).arg( clip->name() ) );
	}
	emitTrace( "record_capture", QJsonObject{ { "notes", added }, { "clip", clip->name() } } );
}


// ---------------------------------------------------------------------------
// model addressing
// ---------------------------------------------------------------------------

AutomatableModel* AgentControlService::modelForTrackPath(
	InstrumentTrack* track, const QStringList& path, QString& error ) const
{
	if( path.isEmpty() )
	{
		error = tr( "empty model path" );
		return nullptr;
	}
	const QString p0 = path[0];

	if( p0 == "volume" ) { return track->volumeModel(); }
	if( p0 == "pan" ) { return track->panningModel(); }
	if( p0 == "pitch" ) { return track->pitchModel(); }
	if( p0 == "pitch_range" ) { return track->pitchRangeModel(); }
	if( p0 == "base_note" ) { return track->baseNoteModel(); }
	if( p0 == "first_key" ) { return track->firstKeyModel(); }
	if( p0 == "last_key" ) { return track->lastKeyModel(); }
	if( p0 == "mixer_channel" ) { return track->mixerChannelModel(); }
	if( p0 == "midi" && path.size() == 3 && path[1] == "cc" )
	{
		bool ok = false;
		const int i = path[2].toInt( &ok );
		if( !ok || i < 0 || i >= MidiControllerCount )
		{
			error = tr( "midi cc index out of range: %1" ).arg( path[2] );
			return nullptr;
		}
		return track->midiCCModel( i );
	}

	InstrumentSoundShaping* shaping = track->soundShaping();
	if( p0 == "filter" && path.size() == 2 )
	{
		if( path[1] == "enabled" ) { return &shaping->getFilterEnabledModel(); }
		if( path[1] == "type" ) { return &shaping->getFilterModel(); }
		if( path[1] == "cutoff" ) { return &shaping->getFilterCutModel(); }
		if( path[1] == "reso" ) { return &shaping->getFilterResModel(); }
	}

	if( p0 == "env" || p0 == "lfo" )
	{
		EnvelopeAndLfoParameters* params = nullptr;
		if( path.size() == 2 )
		{
			params = &shaping->getVolumeParameters();
		}
		else if( path.size() == 3 )
		{
			if( path[1] == "cutoff" ) { params = &shaping->getCutoffParameters(); }
			else if( path[1] == "resonance" ) { params = &shaping->getResonanceParameters(); }
		}
		if( params != nullptr )
		{
			const QString p = path[path.size() - 1];
			if( p0 == "env" )
			{
				if( p == "predelay" ) { return &params->getPredelayModel(); }
				if( p == "attack" ) { return &params->getAttackModel(); }
				if( p == "hold" ) { return &params->getHoldModel(); }
				if( p == "decay" ) { return &params->getDecayModel(); }
				if( p == "sustain" ) { return &params->getSustainModel(); }
				if( p == "release" ) { return &params->getReleaseModel(); }
				if( p == "amount" ) { return &params->getAmountModel(); }
			}
			else
			{
				if( p == "amount" ) { return &params->getLfoAmountModel(); }
				if( p == "speed" ) { return &params->getLfoSpeedModel(); }
				if( p == "wave" ) { return &params->getLfoWaveModel(); }
				if( p == "delay" ) { return &params->getLfoPredelayModel(); }
				if( p == "attack" ) { return &params->getLfoAttackModel(); }
				if( p == "x100" ) { return &params->getX100Model(); }
			}
		}
	}

	if( p0 == "arp" && path.size() == 2 )
	{
		InstrumentFunctionArpeggio* arp = track->arpeggio();
		const QString p = path[1];
		if( p == "enabled" ) { return &arp->arpEnabledModel(); }
		if( p == "chord" ) { return &arp->arpModel(); }
		if( p == "range" ) { return &arp->arpRangeModel(); }
		if( p == "repeats" ) { return &arp->arpRepeatsModel(); }
		if( p == "cycle" ) { return &arp->arpCycleModel(); }
		if( p == "skip" ) { return &arp->arpSkipModel(); }
		if( p == "miss" ) { return &arp->arpMissModel(); }
		if( p == "time" ) { return &arp->arpTimeModel(); }
		if( p == "gate" ) { return &arp->arpGateModel(); }
		if( p == "direction" ) { return &arp->arpDirectionModel(); }
		if( p == "mode" ) { return &arp->arpModeModel(); }
	}

	if( p0 == "ns" && path.size() == 2 )
	{
		InstrumentFunctionNoteStacking* ns = track->noteStacking();
		if( path[1] == "enabled" ) { return &ns->chordsEnabledModel(); }
		if( path[1] == "chord" ) { return &ns->chordsModel(); }
		if( path[1] == "range" ) { return &ns->chordRangeModel(); }
	}

	error = tr( "unknown model path: %1" ).arg( path.join( '.' ) );
	return nullptr;
}

AutomatableModel* AgentControlService::resolveModelAddress(
	const QString& address, QString& canonicalAddress, QString& error ) const
{
	canonicalAddress.clear();
	Song* song = Engine::getSong();
	if( song == nullptr )
	{
		error = tr( "no song loaded" );
		return nullptr;
	}

	if( address.startsWith( "song." ) )
	{
		const QString p = address.mid( 5 );
		if( p == "tempo" ) { canonicalAddress = QStringLiteral( "song.tempo" ); return &song->tempoModel(); }
		if( p == "master.volume" ) { canonicalAddress = QStringLiteral( "song.master.volume" ); return &song->masterVolumeModel(); }
		if( p == "master.pitch" ) { canonicalAddress = QStringLiteral( "song.master.pitch" ); return &song->masterPitchModel(); }
		error = tr( "unknown song model: %1" ).arg( p );
		return nullptr;
	}

	if( address.startsWith( "track:" ) )
	{
		const int dot = address.indexOf( '.' );
		if( dot < 0 )
		{
			error = tr( "track address needs a model path: %1" ).arg( address );
			return nullptr;
		}
		const QString trackName = address.mid( 6, dot - 6 );
		const QString pathStr = address.mid( dot + 1 );
		QJsonObject args{ { "track", trackName } };
		Track* t = resolveTrackRef( args );
		if( t == nullptr )
		{
			error = tr( "track not found: %1" ).arg( trackName );
			return nullptr;
		}
		if( auto* it = dynamic_cast<InstrumentTrack*>( t ) )
		{
			AutomatableModel* m = modelForTrackPath( it, pathStr.split( '.' ), error );
			if( m != nullptr )
			{
				canonicalAddress = QString( "track:%1.%2" ).arg( it->name(), pathStr );
			}
			return m;
		}
		if( auto* st = dynamic_cast<SampleTrack*>( t ) )
		{
			if( pathStr == "volume" )
			{
				canonicalAddress = QString( "track:%1.volume" ).arg( st->name() );
				return st->volumeModel();
			}
			if( pathStr == "pan" )
			{
				canonicalAddress = QString( "track:%1.pan" ).arg( st->name() );
				return st->panningModel();
			}
			error = tr( "sample tracks expose only volume/pan" );
			return nullptr;
		}
		error = tr( "track is not an instrument/sample track: %1" ).arg( trackName );
		return nullptr;
	}

	if( address.startsWith( "fx:" ) )
	{
		const QStringList parts = address.mid( 3 ).split( '.' );
		if( parts.size() < 3 )
		{
			error = tr( "fx address needs channel.effect.param" );
			return nullptr;
		}
		MixerChannel* channel = resolveMixerChannelRef( parts[0], error );
		if( channel == nullptr ) { return nullptr; }
		Effect* fx = findEffectInChain( &channel->m_fxChain, parts[1] );
		if( fx == nullptr )
		{
			error = tr( "effect not found on channel %1: %2" ).arg( parts[0], parts[1] );
			return nullptr;
		}
		AutomatableModel* m = findParamByName( fx->controls(), parts[2] );
		if( m == nullptr )
		{
			error = tr( "effect param not found: %1" ).arg( parts[2] );
			return nullptr;
		}
		canonicalAddress = address;
		return m;
	}

	if( address.startsWith( "inst:" ) )
	{
		const QString p = address.mid( 5 );
		QJsonObject args;
		QString resolveError;
		InstrumentTrack* it = resolveInstrumentTrackOrLast( args, resolveError );
		if( it == nullptr || it->instrument() == nullptr )
		{
			error = tr( "no instrument loaded on the target track" );
			return nullptr;
		}
		Instrument* inst = it->instrument();
		const QString wanted = normalizeName( p );
		const auto models = inst->findChildren<AutomatableModel*>();
		for( auto* m : models )
		{
			if( m != nullptr && normalizeName( m->displayName() ) == wanted )
			{
				canonicalAddress = address;
				return m;
			}
		}
		if( AutomatableModel* child = inst->childModel( p ) )
		{
			canonicalAddress = address;
			return child;
		}
		error = tr( "instrument param not found: %1" ).arg( p );
		return nullptr;
	}

	error = tr( "invalid address: %1" ).arg( address );
	return nullptr;
}

QJsonObject AgentControlService::describeModel( AutomatableModel* model, const QString& address )
{
	QJsonObject o;
	if( model == nullptr )
	{
		o["address"] = address;
		o["error"] = "unresolvable";
		return o;
	}
	o["address"] = address;
	o["display"] = model->displayName();
	o["type"] = modelTypeName( model );
	o["min"] = model->minValue<double>();
	o["max"] = model->maxValue<double>();
	o["step"] = model->step<double>();
	o["value"] = model->value<double>();
	o["automated"] = model->isAutomated();
	if( auto* cc = model->controllerConnection() )
	{
		o["controller"] = cc->getController()->displayName();
	}
	return o;
}

QJsonObject AgentControlService::describeModelsForTrack(
	InstrumentTrack* track, const QString& trackName )
{
	QJsonArray params;
	const QStringList paths =
	{
		"volume", "pan", "pitch", "pitch_range", "base_note", "first_key", "last_key", "mixer_channel",
		"filter.enabled", "filter.type", "filter.cutoff", "filter.reso",
		"env.predelay", "env.attack", "env.hold", "env.decay", "env.sustain", "env.release", "env.amount",
		"env.cutoff.attack", "env.cutoff.decay", "env.cutoff.release", "env.cutoff.amount",
		"env.resonance.attack", "env.resonance.decay", "env.resonance.amount",
		"lfo.amount", "lfo.speed", "lfo.wave", "lfo.delay", "lfo.attack",
		"lfo.cutoff.amount", "lfo.cutoff.speed", "lfo.cutoff.wave",
		"arp.enabled", "arp.chord", "arp.range", "arp.direction", "arp.time", "arp.gate", "arp.mode",
		"ns.enabled", "ns.chord", "ns.range"
	};
	QString error;
	for( const QString& path : paths )
	{
		AutomatableModel* m = modelForTrackPath( track, path.split( '.' ), error );
		if( m != nullptr )
		{
			params.append( describeModel( m, QString( "track:%1.%2" ).arg( trackName, path ) ) );
		}
	}
	QJsonObject result;
	result["track"] = trackName;
	result["params"] = params;
	return result;
}

MixerChannel* AgentControlService::resolveMixerChannelRef( const QString& ref, QString& error ) const
{
	Mixer* mixer = Engine::mixer();
	if( mixer == nullptr )
	{
		error = tr( "mixer not available" );
		return nullptr;
	}
	bool ok = false;
	const int idx = ref.toInt( &ok );
	if( ok && idx >= 0 && idx < static_cast<int>( mixer->numChannels() ) )
	{
		return mixer->mixerChannel( idx );
	}
	const QString wanted = normalizeName( ref );
	for( mix_ch_t i = 0; i < mixer->numChannels(); ++i )
	{
		MixerChannel* ch = mixer->mixerChannel( i );
		if( normalizeName( ch->m_name ).contains( wanted ) )
		{
			return ch;
		}
	}
	error = tr( "mixer channel not found: %1" ).arg( ref );
	return nullptr;
}

Effect* AgentControlService::findEffectInChain( EffectChain* chain, const QString& effectName ) const
{
	if( chain == nullptr ) { return nullptr; }
	const QString wanted = normalizeName( effectName );
	Effect* fallback = nullptr;
	for( Effect* fx : chain->effects() )
	{
		if( fx == nullptr ) { continue; }
		const QString candidate = normalizeName( fx->displayName() );
		if( candidate == wanted ) { return fx; }
		if( fallback == nullptr && candidate.contains( wanted ) ) { fallback = fx; }
	}
	return fallback;
}

AutomatableModel* AgentControlService::findParamByName(
	EffectControls* controls, const QString& paramName ) const
{
	if( controls == nullptr ) { return nullptr; }
	bool numeric = false;
	const int index = paramName.toInt( &numeric );
	const QString wanted = normalizeName( paramName );
	const auto params = controls->findChildren<AutomatableModel*>(
		QString(), Qt::FindDirectChildrenOnly );
	AutomatableModel* fallback = nullptr;
	int seen = 0;
	for( auto* m : params )
	{
		if( m == nullptr ) { continue; }
		const QString candidate = normalizeName( m->displayName() );
		if( candidate == wanted ) { return m; }
		if( numeric && seen == index ) { fallback = m; }
		if( fallback == nullptr && candidate.contains( wanted ) ) { fallback = m; }
		++seen;
	}
	return fallback;
}

InstrumentTrack* AgentControlService::resolveInstrumentTrackOrLast(
	const QJsonObject& args, QString& error ) const
{
	InstrumentTrack* track = resolveInstrumentTrack( args );
	if( track == nullptr )
	{
		track = dynamic_cast<InstrumentTrack*>(
			findLastTrackOfTypes( { Track::Type::Instrument } ) );
	}
	if( track == nullptr )
	{
		error = tr( "no instrument track available" );
		return nullptr;
	}
	return track;
}

SampleTrack* AgentControlService::resolveSampleTrackOrLast(
	const QJsonObject& args, QString& error ) const
{
	SampleTrack* track = resolveSampleTrack( args );
	if( track == nullptr )
	{
		track = dynamic_cast<SampleTrack*>(
			findLastTrackOfTypes( { Track::Type::Sample } ) );
	}
	if( track == nullptr )
	{
		error = tr( "no sample track available" );
		return nullptr;
	}
	return track;
}

MidiClip* AgentControlService::resolveMidiClip( const QJsonObject& args, QString& error ) const
{
	QString resolveError;
	InstrumentTrack* track = resolveInstrumentTrackOrLast( args, resolveError );
	if( track == nullptr )
	{
		error = resolveError;
		return nullptr;
	}
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
		error = tr( "no pattern clip on track %1" ).arg( track->name() );
		return nullptr;
	}
	return clip;
}

AutomationClip* AgentControlService::resolveAutomationClipRef(
	const QString& clipRef, QString& error ) const
{
	if( clipRef.startsWith( "global:" ) )
	{
		const QString address = clipRef.mid( 7 );
		QString canonical;
		QString resolveError;
		AutomatableModel* model = resolveModelAddress( address, canonical, resolveError );
		if( model == nullptr )
		{
			error = resolveError;
			return nullptr;
		}
		return AutomationClip::globalAutomationClip( model );
	}
	if( clipRef.startsWith( "auto:" ) )
	{
		const QStringList parts = clipRef.mid( 5 ).split( ':' );
		if( parts.size() != 2 )
		{
			error = tr( "invalid automation clip ref: %1" ).arg( clipRef );
			return nullptr;
		}
		QJsonObject args{ { "track", parts[0] } };
		Track* t = resolveTrackRef( args );
		if( t == nullptr )
		{
			error = tr( "automation track not found: %1" ).arg( parts[0] );
			return nullptr;
		}
		bool ok = false;
		const int index = parts[1].toInt( &ok );
		if( !ok || index < 0 || index >= static_cast<int>( t->getClips().size() ) )
		{
			error = tr( "automation clip index out of range: %1" ).arg( parts[1] );
			return nullptr;
		}
		return dynamic_cast<AutomationClip*>( t->getClip( static_cast<std::size_t>( index ) ) );
	}
	error = tr( "invalid automation clip ref: %1" ).arg( clipRef );
	return nullptr;
}

QString AgentControlService::automationClipRef( AutomationClip* clip ) const
{
	if( clip == nullptr ) { return QString(); }
	Song* song = Engine::getSong();
	if( song != nullptr && clip->getTrack() == static_cast<Track*>( song->globalAutomationTrack() ) )
	{
		if( const AutomatableModel* obj = clip->firstObject() )
		{
			// best-effort: report as global:<model display name>; clients may
			// re-resolve by address through describe.
			return QString( "global:%1" ).arg( obj->displayName() );
		}
		return QStringLiteral( "global" );
	}
	Track* t = clip->getTrack();
	if( t == nullptr ) { return QString(); }
	return QString( "auto:%1:%2" ).arg( t->name() )
		.arg( t->getClipNum( clip ) );
}

bool AgentControlService::setModelValue( AutomatableModel* model, double value, QString& error )
{
	if( model == nullptr )
	{
		error = tr( "null model" );
		return false;
	}
	model->setValue( static_cast<float>( value ) );
	return true;
}

TimePos AgentControlService::timePosFromArgs( const QJsonObject& args, bool& ok ) const
{
	ok = false;
	if( args.contains( "bar" ) )
	{
		const int bar = qMax( 0, args.value( "bar" ).toInt( 1 ) - 1 );
		const int beat = qMax( 0, args.value( "beat" ).toInt( 0 ) );
		const int tick = qMax( 0, args.value( "tick" ).toInt( 0 ) );
		const int tpb = TimePos::ticksPerBar();
		const int ticksPerBeat = qMax( 1, tpb / 4 );
		ok = true;
		return TimePos( bar * tpb + beat * ticksPerBeat + tick );
	}
	if( args.contains( "tick" ) )
	{
		ok = true;
		return TimePos( static_cast<tick_t>( qMax( 0, args.value( "tick" ).toInt( 0 ) ) ) );
	}
	if( args.contains( "ticks" ) )
	{
		ok = true;
		return TimePos( static_cast<tick_t>( qMax( 0, args.value( "ticks" ).toInt( 0 ) ) ) );
	}
	return TimePos();
}

bool AgentControlService::startRender( const OutputSettings& settings,
	const QString& outputPath, bool tracksMode, RenderJob& job, QString& error )
{
	if( m_renderJob.manager != nullptr && !m_renderJob.done )
	{
		error = tr( "another render is in progress (%1)" ).arg( m_renderJob.id );
		return false;
	}

	const ProjectRenderer::ExportFileFormat fmt =
		ProjectRenderer::getFileFormatFromExtension( QFileInfo( outputPath ).suffix() );
	if( fmt == ProjectRenderer::ExportFileFormat::Count )
	{
		error = tr( "unsupported output format: %1" ).arg( outputPath );
		return false;
	}

	auto* manager = new RenderManager( settings, fmt, outputPath );

	job.id = QUuid::createUuid().toString( QUuid::WithoutBraces ).left( 8 );
	job.manager = manager;
	job.outputPath = outputPath;
	job.progress = 0;
	job.done = false;
	job.cancelled = false;
	job.error.clear();
	m_renderJob = job;

	const QString jobId = job.id;
	connect( manager, &RenderManager::progressChanged, this,
		[this, jobId]( int p )
		{
			if( m_renderJob.id == jobId ) { m_renderJob.progress = p; }
		} );
	connect( manager, &RenderManager::finished, this,
		[this, manager, jobId]()
		{
			if( m_renderJob.id == jobId ) { m_renderJob.done = true; }
			manager->deleteLater();
		} );

	QTimer::singleShot( 0, manager,
		[manager, tracksMode]()
		{
			if( tracksMode ) { manager->renderTracks(); }
			else { manager->renderProject(); }
		} );
	return true;
}

} // namespace lmms
