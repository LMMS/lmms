/*
 * agent_sample.cpp - sample manipulation domain tools (v2)
 */

#include "AgentControl.h"

#include <QJsonArray>

#include "Sample.h"
#include "SampleClip.h"
#include "SampleTrack.h"

namespace lmms
{

std::optional<QJsonObject> AgentControlService::dispatchSampleTool(
	const QString& tool, const QJsonObject& args )
{
	QJsonObject result;
	QString error;

	if( tool != "setsampleloop" && tool != "setsamplepitch"
		&& tool != "setsampleamp" && tool != "setsamplerange" )
	{
		return std::nullopt;
	}

	SampleTrack* track = resolveSampleTrackOrLast( args, error );
	if( track == nullptr ) { return errorResponse( "tool_failed", error ); }

	SampleClip* clip = nullptr;
	const int requestedIndex = args.value( "clip_index" ).toInt( -1 );
	const auto& clips = track->getClips();
	if( requestedIndex >= 0 && requestedIndex < static_cast<int>( clips.size() ) )
	{
		clip = dynamic_cast<SampleClip*>( clips[static_cast<std::size_t>( requestedIndex )] );
	}
	if( clip == nullptr )
	{
		for( auto it = clips.rbegin(); it != clips.rend(); ++it )
		{
			clip = dynamic_cast<SampleClip*>( *it );
			if( clip != nullptr ) { break; }
		}
	}
	if( clip == nullptr ) { return errorResponse( "tool_failed", tr( "no sample clip on track %1" ).arg( track->name() ) ); }

	Sample& sample = clip->sample();

	if( tool == "setsampleloop" )
	{
		const QString mode = args.value( "mode" ).toString().trimmed();
		if( mode != "off" && mode != "on" && mode != "pingpong" )
		{
			return errorResponse( "bad_args", tr( "mode must be off|on|pingpong" ) );
		}
		const int endFrame = sample.endFrame() > 0 ? sample.endFrame() : 1;
		const int loopStart = args.contains( "loop_start" ) ? args.value( "loop_start" ).toInt() : sample.startFrame();
		const int loopEnd = args.contains( "loop_end" ) ? args.value( "loop_end" ).toInt() : endFrame;
		sample.setLoopStartFrame( qBound( 0, loopStart, qMax( 0, endFrame - 1 ) ) );
		sample.setLoopEndFrame( qBound( loopStart + 1, loopEnd, qMax( loopStart + 1, endFrame ) ) );
		QJsonArray warnings;
		warnings.append( tr( "loop mode is not persisted per clip in this LMMS build; start/end frames are set" ) );
		result["mode"] = mode;
		result["loop_start"] = sample.loopStartFrame();
		result["loop_end"] = sample.loopEndFrame();
		return successResponse( result, QJsonObject(), warnings );
	}
	if( tool == "setsamplepitch" )
	{
		const double semitones = args.value( "semitones" ).toDouble( 0.0 );
		const double ratio = std::pow( 2.0, semitones / 12.0 );
		sample.setFrequency( static_cast<float>( DefaultBaseFreq * ratio ) );
		result["semitones"] = semitones;
		result["frequency"] = sample.frequency();
		return successResponse( result );
	}
	if( tool == "setsampleamp" )
	{
		const double value = args.value( "value" ).toDouble( -1.0 );
		if( value < 0.0 || value > 1.0 )
		{
			return errorResponse( "bad_args", tr( "amplification must be in 0..1" ) );
		}
		sample.setAmplification( static_cast<float>( value * 2.0f ) );
		result["value"] = sample.amplification();
		return successResponse( result );
	}
	if( tool == "setsamplerange" )
	{
		if( !args.contains( "start_frame" ) || !args.contains( "length_frames" ) )
		{
			return errorResponse( "bad_args", tr( "set_sample_range needs start_frame and length_frames" ) );
		}
		const int start = qMax( 0, args.value( "start_frame" ).toInt() );
		const int length = qMax( 1, args.value( "length_frames" ).toInt() );
		clip->setSampleStartFrame( start );
		clip->setSamplePlayLength( length );
		result["start_frame"] = start;
		result["length_frames"] = length;
		return successResponse( result );
	}

	return std::nullopt;
}

} // namespace lmms
