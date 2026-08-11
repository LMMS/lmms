/*
 * agent_pattern.cpp - pattern (Beat+Bassline) domain tools (v2)
 */

#include "AgentControl.h"

#include <QJsonArray>

#include "Engine.h"
#include "InstrumentTrack.h"
#include "MidiClip.h"
#include "PatternStore.h"
#include "PatternTrack.h"
#include "SampleTrack.h"
#include "Song.h"
#include "Track.h"

namespace lmms
{

std::optional<QJsonObject> AgentControlService::dispatchPatternTool(
	const QString& tool, const QJsonObject& args )
{
	Song* song = Engine::getSong();
	PatternStore* store = Engine::patternStore();
	if( song == nullptr || store == nullptr ) { return errorResponse( "tool_failed", tr( "no song" ) ); }
	QJsonObject result;
	QString error;

	if( tool == "createpattern" )
	{
		song->addPatternTrack();
		const int pattern = store->currentPattern();
		const QString name = args.value( "name" ).toString().trimmed();
		if( !name.isEmpty() && store->numOfPatterns() > 0 )
		{
			if( PatternTrack* pt = PatternTrack::findPatternTrack( pattern ) )
			{
				pt->setName( name );
			}
		}
		result["pattern"] = pattern;
		result["count"] = store->numOfPatterns();
		return successResponse( result );
	}
	if( tool == "selectpattern" )
	{
		const int pattern = args.value( "pattern" ).toInt( -1 );
		if( pattern < 0 || pattern >= store->numOfPatterns() )
		{
			return errorResponse( "bad_args", tr( "pattern out of range (0..%1)" ).arg( store->numOfPatterns() - 1 ) );
		}
		store->setCurrentPattern( pattern );
		result["pattern"] = pattern;
		return successResponse( result );
	}
	if( tool == "clonepattern" )
	{
		const int pattern = args.value( "pattern" ).toInt( -1 );
		if( pattern < 0 || pattern >= store->numOfPatterns() )
		{
			return errorResponse( "bad_args", tr( "pattern out of range (0..%1)" ).arg( store->numOfPatterns() - 1 ) );
		}
		PatternTrack* source = PatternTrack::findPatternTrack( pattern );
		if( source == nullptr ) { return errorResponse( "tool_failed", tr( "pattern track not found" ) ); }
		Track* copy = source->clone();
		if( copy == nullptr ) { return errorResponse( "tool_failed", tr( "pattern clone failed" ) ); }
		if( args.contains( "name" ) ) { copy->setName( args.value( "name" ).toString() ); }
		result["pattern"] = dynamic_cast<PatternTrack*>( copy )->patternIndex();
		result["count"] = store->numOfPatterns();
		return successResponse( result );
	}

	MidiClip* clip = resolveMidiClip( args, error );
	if( clip == nullptr ) { return errorResponse( "tool_failed", error ); }

	if( tool == "setsteps" )
	{
		const QJsonArray steps = args.value( "steps" ).toArray();
		if( steps.isEmpty() ) { return errorResponse( "bad_args", tr( "set_steps needs steps" ) ); }
		if( args.value( "clear_existing" ).toBool( false ) ) { clip->clear(); }
		int enabled = 0;
		for( const auto& s : steps )
		{
			const int step = s.toInt( -1 );
			if( step >= 0 )
			{
				clip->setStep( step, true );
				++enabled;
			}
		}
		result["enabled"] = enabled;
		result["track"] = clip->instrumentTrack()->name();
		result["clip_index"] = clip->instrumentTrack()->getClipNum( clip );
		return successResponse( result );
	}
	if( tool == "setstepvelocity" )
	{
		const int step = args.value( "step" ).toInt( -1 );
		const int velocity = args.value( "velocity" ).toInt( -1 );
		if( step < 0 || velocity < 1 || velocity > 127 )
		{
			return errorResponse( "bad_args", tr( "set_step_velocity needs step (>=0) and velocity (1..127)" ) );
		}
		Note* note = clip->noteAtStep( step );
		if( note == nullptr ) { return errorResponse( "tool_failed", tr( "no note at step %1" ).arg( step ) ); }
		note->setVolume( static_cast<volume_t>( velocity ) * DefaultVolume / 127 );
		result["step"] = step;
		result["velocity"] = velocity;
		result["track"] = clip->instrumentTrack()->name();
		return successResponse( result );
	}
	if( tool == "setstepsperbar" )
	{
		const int steps = args.value( "steps" ).toInt( -1 );
		if( steps < 1 || steps > 64 )
		{
			return errorResponse( "bad_args", tr( "steps must be 1..64" ) );
		}
		clip->setStepsPerBar( steps );
		result["steps"] = steps;
		result["track"] = clip->instrumentTrack()->name();
		return successResponse( result );
	}
	if( tool == "addrhythm" )
	{
		const QString drum = args.value( "drum" ).toString().trimmed();
		const QJsonArray pattern = args.value( "pattern" ).toArray();
		if( pattern.isEmpty() ) { return errorResponse( "bad_args", tr( "add_rhythm needs pattern [0..15]" ) ); }
		QString sampleFile;
		if( drum == "kick" ) { sampleFile = defaultKickSample(); }
		else if( drum == "snare" ) { sampleFile = defaultSnareSample(); }
		else if( drum == "hihat" ) { sampleFile = defaultHiHatSample(); }
		else if( drum == "crash" ) { sampleFile = defaultCrashSample(); }
		else if( drum == "ride" ) { sampleFile = defaultRideSample(); }
		else
		{
			return errorResponse( "bad_args", tr( "drum must be kick|snare|hihat|crash|ride" ) );
		}
		if( sampleFile.isEmpty() ) { return errorResponse( "not_available", tr( "no default %1 sample found" ).arg( drum ) ); }

		// create a sample track and place one sample clip per enabled step
		SampleTrack* track = createSampleTrack( tr( "Agent %1" ).arg( drum ) );
		if( track == nullptr ) { return errorResponse( "tool_failed", tr( "could not create sample track" ) ); }
		const int stepTicks = qMax( 1, TimePos::ticksPerBar() / 16 );
		int placed = 0;
		for( const auto& s : pattern )
		{
			const int step = s.toInt( -1 );
			if( step >= 0 && step < 16 )
			{
				if( !addSampleClip( track, sampleFile, step * stepTicks ) )
				{
					return errorResponse( "tool_failed", tr( "failed to place sample at step %1" ).arg( step ) );
				}
				++placed;
			}
		}
		result["drum"] = drum;
		result["placed"] = placed;
		result["track"] = track->name();
		return successResponse( result );
	}

	return std::nullopt;
}

} // namespace lmms
