/*
 * agent_arrangement.cpp - track & clip (arrangement) domain tools (v2)
 */

#include "AgentControl.h"

#include <QColor>
#include <QJsonArray>

#include "Clip.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "SampleTrack.h"
#include "Song.h"
#include "Track.h"
#include "TrackContainer.h"

namespace lmms
{

std::optional<QJsonObject> AgentControlService::dispatchArrangementTool(
	const QString& tool, const QJsonObject& args )
{
	Song* song = Engine::getSong();
	if( song == nullptr ) { return errorResponse( "tool_failed", tr( "no song loaded" ) ); }
	QJsonObject result;
	QString error;

	// ---- clip ops -----------------------------------------------------------

	if( tool == "createclip" )
	{
		QJsonObject trackArgs = args;
		trackArgs.remove( "tick" );
		trackArgs.remove( "name" );
		InstrumentTrack* track = resolveInstrumentTrackOrLast( trackArgs, error );
		if( track == nullptr ) { return errorResponse( "tool_failed", error ); }
		bool ok = false;
		const TimePos pos = timePosFromArgs( args, ok );
		if( !ok ) { return errorResponse( "bad_args", tr( "create_clip needs tick or bar" ) ); }
		Clip* clip = track->createClip( pos );
		if( clip == nullptr ) { return errorResponse( "tool_failed", tr( "failed to create clip" ) ); }
		const QString name = args.value( "name" ).toString().trimmed();
		if( !name.isEmpty() ) { clip->setName( name ); }
		result["track"] = track->name();
		result["clip_index"] = track->getClipNum( clip );
		result["start_tick"] = clip->startPosition().getTicks();
		return successResponse( result );
	}

	Clip* clip = nullptr;
	Track* track = nullptr;
	int clipIndex = -1;
	if( tool == "moveclip" || tool == "resizeclip" || tool == "splitclip"
		|| tool == "cloneclip" || tool == "deleteclip" || tool == "setclipmute"
		|| tool == "setclipname" || tool == "setclipcolor" )
	{
		QString resolveError;
		track = resolveTrackRef( args );
		if( track == nullptr ) { return errorResponse( "tool_failed", tr( "track not found" ) ); }
		clipIndex = args.value( "clip_index" ).toInt( -1 );
		const auto& clips = track->getClips();
		if( clipIndex < 0 || clipIndex >= static_cast<int>( clips.size() ) )
		{
			return errorResponse( "bad_args", tr( "clip_index out of range" ) );
		}
		clip = clips[static_cast<std::size_t>( clipIndex )];
	}

	if( tool == "moveclip" )
	{
		bool ok = false;
		const TimePos pos = timePosFromArgs( args, ok );
		if( !ok ) { return errorResponse( "bad_args", tr( "move_clip needs tick or bar" ) ); }
		clip->movePosition( pos );
		result["track"] = track->name();
		result["clip_index"] = clipIndex;
		result["start_tick"] = clip->startPosition().getTicks();
		return successResponse( result );
	}
	if( tool == "resizeclip" )
	{
		if( !args.contains( "new_length" ) )
		{
			return errorResponse( "bad_args", tr( "resize_clip needs new_length" ) );
		}
		const int newLength = qMax( 1, args.value( "new_length" ).toInt() );
		clip->changeLength( TimePos( newLength ) );
		clip->setAutoResize( false );
		result["track"] = track->name();
		result["clip_index"] = clipIndex;
		result["length"] = clip->length().getTicks();
		return successResponse( result );
	}
	if( tool == "splitclip" )
	{
		bool ok = false;
		const TimePos at = timePosFromArgs( args, ok );
		if( !ok ) { return errorResponse( "bad_args", tr( "split_clip needs tick or bar" ) ); }
		const tick_t splitPos = at.getTicks();
		const tick_t start = clip->startPosition().getTicks();
		const tick_t end = clip->endPosition().getTicks();
		if( splitPos <= start || splitPos >= end )
		{
			return errorResponse( "bad_args", tr( "split point must be inside the clip" ) );
		}
		Clip* rightClip = clip->clone();
		clip->changeLength( TimePos( splitPos - start ) );
		clip->setAutoResize( false );
		rightClip->movePosition( TimePos( splitPos ) );
		rightClip->changeLength( TimePos( end - splitPos ) );
		rightClip->setStartTimeOffset( clip->startTimeOffset() - clip->length() );
		rightClip->setAutoResize( false );
		result["track"] = track->name();
		result["left_index"] = track->getClipNum( clip );
		result["right_index"] = track->getClipNum( rightClip );
		result["split_tick"] = splitPos;
		return successResponse( result );
	}
	if( tool == "cloneclip" )
	{
		Clip* copy = clip->clone();
		if( args.contains( "to_tick" ) )
		{
			copy->movePosition( TimePos( args.value( "to_tick" ).toInt() ) );
		}
		result["track"] = track->name();
		result["clip_index"] = track->getClipNum( copy );
		result["start_tick"] = copy->startPosition().getTicks();
		return successResponse( result );
	}
	if( tool == "deleteclip" )
	{
		track->removeClip( clip );
		result["track"] = track->name();
		result["clip_index"] = clipIndex;
		return successResponse( result );
	}
	if( tool == "setclipmute" )
	{
		if( !args.contains( "mute" ) ) { return errorResponse( "bad_args", tr( "set_clip_mute needs mute" ) ); }
		clip->setMuted( args.value( "mute" ).toBool() );
		result["track"] = track->name();
		result["clip_index"] = clipIndex;
		result["muted"] = clip->isMuted();
		return successResponse( result );
	}
	if( tool == "setclipname" )
	{
		const QString name = args.value( "name" ).toString().trimmed();
		if( name.isEmpty() ) { return errorResponse( "bad_args", tr( "set_clip_name needs name" ) ); }
		clip->setName( name );
		result["track"] = track->name();
		result["clip_index"] = clipIndex;
		result["name"] = clip->name();
		return successResponse( result );
	}
	if( tool == "setclipcolor" )
	{
		const QString colorHex = args.value( "color" ).toString().trimmed();
		const QColor color( colorHex );
		if( !color.isValid() ) { return errorResponse( "bad_args", tr( "color must be #rrggbb" ) ); }
		clip->setColor( color );
		result["track"] = track->name();
		result["clip_index"] = clipIndex;
		return successResponse( result );
	}

	// ---- track ops ------------------------------------------------------------

	if( tool == "clonetrack" )
	{
		Track* source = resolveTrackRef( args );
		if( source == nullptr ) { return errorResponse( "tool_failed", tr( "track not found" ) ); }
		Track* copy = source->clone();
		if( copy == nullptr ) { return errorResponse( "tool_failed", tr( "track clone failed" ) ); }
		if( args.contains( "name" ) )
		{
			copy->setName( args.value( "name" ).toString() );
		}
		result["track"] = copy->name();
		result["type"] = trackTypeName( copy->type() );
		return successResponse( result );
	}
	if( tool == "deletetrack" )
	{
		Track* target = resolveTrackRef( args );
		if( target == nullptr ) { return errorResponse( "tool_failed", tr( "track not found" ) ); }
		const QString name = target->name();
		TrackContainer* container = target->trackContainer();
		if( container == nullptr ) { return errorResponse( "tool_failed", tr( "track has no container" ) ); }
		container->removeTrack( target );
		result["track"] = name;
		return successResponse( result );
	}
	if( tool == "movetrack" )
	{
		Track* target = resolveTrackRef( args );
		if( target == nullptr ) { return errorResponse( "tool_failed", tr( "track not found" ) ); }
		const int index = args.value( "index" ).toInt( -1 );
		if( index < 0 ) { return errorResponse( "bad_args", tr( "move_track needs index" ) ); }
		TrackContainer* container = target->trackContainer();
		if( container == nullptr ) { return errorResponse( "tool_failed", tr( "track has no container" ) ); }
		container->moveTrack( target, index );
		result["track"] = target->name();
		result["index"] = index;
		return successResponse( result );
	}

	if( tool == "settrackvolume" || tool == "settrackpan" || tool == "settrackpitch"
		|| tool == "settrackkeyrange" || tool == "settrackbasenote" )
	{
		Track* target = resolveTrackRef( args );
		if( target == nullptr ) { return errorResponse( "tool_failed", tr( "track not found" ) ); }
		if( auto* it = dynamic_cast<InstrumentTrack*>( target ) )
		{
			if( tool == "settrackvolume" )
			{
				const double v = args.value( "value" ).toDouble( -1.0 );
				if( v < 0.0 || v > 1.0 ) { return errorResponse( "bad_args", tr( "volume must be in 0..1" ) ); }
				it->volumeModel()->setValue( static_cast<float>( v ) );
				result["value"] = it->volumeModel()->value();
			}
			else if( tool == "settrackpan" )
			{
				const double v = args.value( "value" ).toDouble( -1.0 );
				if( v < 0.0 || v > 1.0 ) { return errorResponse( "bad_args", tr( "pan must be in 0..1" ) ); }
				it->panningModel()->setValue( static_cast<float>( v ) );
				result["value"] = it->panningModel()->value();
			}
			else if( tool == "settrackpitch" )
			{
				it->pitchModel()->setValue( static_cast<float>( args.value( "value" ).toDouble() ) );
				result["value"] = it->pitchModel()->value();
			}
			else if( tool == "settrackkeyrange" )
			{
				const int first = args.value( "first_key" ).toInt( -1 );
				const int last = args.value( "last_key" ).toInt( -1 );
				if( first < 0 || last < 0 || first > last )
				{
					return errorResponse( "bad_args", tr( "key range needs 0 <= first_key <= last_key" ) );
				}
				it->firstKeyModel()->setValue( first );
				it->lastKeyModel()->setValue( last );
				result["first_key"] = first;
				result["last_key"] = last;
			}
			else if( tool == "settrackbasenote" )
			{
				const int key = args.value( "key" ).toInt( -1 );
				if( key < 0 || key > 127 ) { return errorResponse( "bad_args", tr( "base note must be 0..127" ) ); }
				it->baseNoteModel()->setValue( key );
				result["key"] = key;
			}
			result["track"] = it->name();
			return successResponse( result );
		}
		if( auto* st = dynamic_cast<SampleTrack*>( target ) )
		{
			if( tool == "settrackvolume" )
			{
				const double v = args.value( "value" ).toDouble( -1.0 );
				if( v < 0.0 || v > 1.0 ) { return errorResponse( "bad_args", tr( "volume must be in 0..1" ) ); }
				st->volumeModel()->setValue( static_cast<float>( v ) );
				result["value"] = st->volumeModel()->value();
				result["track"] = st->name();
				return successResponse( result );
			}
			if( tool == "settrackpan" )
			{
				const double v = args.value( "value" ).toDouble( -1.0 );
				if( v < 0.0 || v > 1.0 ) { return errorResponse( "bad_args", tr( "pan must be in 0..1" ) ); }
				st->panningModel()->setValue( static_cast<float>( v ) );
				result["value"] = st->panningModel()->value();
				result["track"] = st->name();
				return successResponse( result );
			}
			return errorResponse( "tool_failed",
				tr( "sample tracks only support volume/pan" ) );
		}
		return errorResponse( "tool_failed", tr( "track type not supported" ) );
	}

	return std::nullopt;
}

} // namespace lmms
