/*
 * agent_project.cpp - project & transport domain tools (v2)
 */

#include "AgentControl.h"

#include <QJsonArray>
#include <QMetaObject>

#include "Engine.h"
#include "Song.h"
#include "Timeline.h"

namespace lmms
{

std::optional<QJsonObject> AgentControlService::dispatchProjectTool(
	const QString& tool, const QJsonObject& args )
{
	Song* song = Engine::getSong();
	QJsonObject result;
	QString error;

	if( tool == "newproject" )
	{
		QString message;
		if( !newProject( message, error ) ) { return errorResponse( "tool_failed", error ); }
		result["message"] = message;
		return successResponse( result );
	}
	if( tool == "openproject" )
	{
		const QString path = args.value( "path" ).toString().trimmed();
		if( path.isEmpty() ) { return errorResponse( "bad_args", tr( "open_project requires path" ) ); }
		QString message;
		if( !openProject( path, message, error ) ) { return errorResponse( "tool_failed", error ); }
		result["message"] = message;
		return successResponse( result );
	}
	if( tool == "saveproject" )
	{
		QString message;
		if( !saveProject( message, error ) ) { return errorResponse( "tool_failed", error ); }
		result["message"] = message;
		return successResponse( result );
	}
	if( tool == "saveprojectas" )
	{
		const QString path = args.value( "path" ).toString().trimmed();
		if( path.isEmpty() ) { return errorResponse( "bad_args", tr( "save_project_as requires path" ) ); }
		QString message;
		if( !saveProjectAs( path, message, error ) ) { return errorResponse( "tool_failed", error ); }
		result["message"] = message;
		return successResponse( result );
	}

	if( tool == "settimesignature" )
	{
		const int numerator = args.value( "numerator" ).toInt( 0 );
		const int denominator = args.value( "denominator" ).toInt( 0 );
		if( numerator < 1 || denominator < 1 )
		{
			return errorResponse( "bad_args", tr( "set_time_signature needs numerator/denominator >= 1" ) );
		}
		song->getTimeSigModel().setNumerator( numerator );
		song->getTimeSigModel().setDenominator( denominator );
		result["numerator"] = numerator;
		result["denominator"] = denominator;
		return successResponse( result );
	}
	if( tool == "setmetronome" )
	{
		if( !args.contains( "enabled" ) )
		{
			return errorResponse( "bad_args", tr( "set_metronome needs enabled" ) );
		}
		song->metronome().setActive( args.value( "enabled" ).toBool() );
		result["enabled"] = song->metronome().active();
		return successResponse( result );
	}
	if( tool == "setmastervolume" )
	{
		const double value = args.value( "value" ).toDouble( -1.0 );
		if( value < 0.0 || value > 1.0 )
		{
			return errorResponse( "bad_args", tr( "master volume must be in 0..1" ) );
		}
		song->masterVolumeModel().setValue( static_cast<float>( value * 100.0 ) );
		result["value"] = song->masterVolumeModel().value();
		return successResponse( result );
	}
	if( tool == "setmasterpitch" )
	{
		if( !args.contains( "value" ) )
		{
			return errorResponse( "bad_args", tr( "set_master_pitch needs value" ) );
		}
		const double value = args.value( "value" ).toDouble();
		song->masterPitchModel().setValue( static_cast<float>( value ) );
		result["value"] = song->masterPitchModel().value();
		return successResponse( result );
	}
	if( tool == "setplaypos" )
	{
		bool ok = false;
		const TimePos pos = timePosFromArgs( args, ok );
		if( !ok )
		{
			return errorResponse( "bad_args", tr( "set_play_pos needs tick or bar" ) );
		}
		song->setPlayPos( pos.getTicks(), Song::PlayMode::Song );
		result["tick"] = pos.getTicks();
		return successResponse( result );
	}
	if( tool == "setloop" )
	{
		if( !args.contains( "begin_tick" ) || !args.contains( "end_tick" ) )
		{
			return errorResponse( "bad_args", tr( "set_loop needs begin_tick and end_tick" ) );
		}
		const int begin = args.value( "begin_tick" ).toInt( -1 );
		const int end = args.value( "end_tick" ).toInt( -1 );
		if( begin < 0 || end <= begin )
		{
			return errorResponse( "bad_args", tr( "end_tick must be greater than begin_tick (>= 0)" ) );
		}
		Timeline& tl = song->getTimeline( Song::PlayMode::Song );
		tl.setLoopPoints( TimePos( begin ), TimePos( end ) );
		tl.setLoopEnabled( true );
		result["begin_tick"] = begin;
		result["end_tick"] = end;
		return successResponse( result );
	}
	if( tool == "clearloop" )
	{
		song->getTimeline( Song::PlayMode::Song ).setLoopEnabled( false );
		return successResponse( result );
	}
	if( tool == "setstopbehaviour" )
	{
		const QString mode = args.value( "mode" ).toString().trimmed();
		Timeline::StopBehaviour behaviour = Timeline::StopBehaviour::BackToZero;
		if( mode == "back_to_start" ) { behaviour = Timeline::StopBehaviour::BackToStart; }
		else if( mode == "continue" ) { behaviour = Timeline::StopBehaviour::KeepPosition; }
		else if( mode != "back_to_zero" )
		{
			return errorResponse( "bad_args",
				tr( "stop behaviour must be back_to_zero|back_to_start|continue" ) );
		}
		song->getTimeline( Song::PlayMode::Song ).setStopBehaviour( behaviour );
		result["mode"] = mode;
		return successResponse( result );
	}
	if( tool == "playpattern" )
	{
		song->playPattern();
		result["message"] = tr( "Playing pattern" );
		return successResponse( result );
	}
	if( tool == "playclip" )
	{
		QString resolveError;
		MidiClip* clip = resolveMidiClip( args, resolveError );
		if( clip == nullptr ) { return errorResponse( "tool_failed", resolveError ); }
		song->playMidiClip( clip );
		result["message"] = tr( "Playing clip" );
		return successResponse( result );
	}
	if( tool == "insertbar" )
	{
		bool ok = false;
		const TimePos pos = timePosFromArgs( args, ok );
		if( !ok ) { return errorResponse( "bad_args", tr( "insert_bar needs tick or bar" ) ); }
		song->setPlayPos( pos.getTicks(), Song::PlayMode::Song );
		QMetaObject::invokeMethod( song, "insertBar", Qt::DirectConnection );
		result["at_tick"] = pos.getTicks();
		return successResponse( result );
	}
	if( tool == "removebar" )
	{
		bool ok = false;
		const TimePos pos = timePosFromArgs( args, ok );
		if( !ok ) { return errorResponse( "bad_args", tr( "remove_bar needs tick or bar" ) ); }
		song->setPlayPos( pos.getTicks(), Song::PlayMode::Song );
		QMetaObject::invokeMethod( song, "removeBar", Qt::DirectConnection );
		result["at_tick"] = pos.getTicks();
		return successResponse( result );
	}

	return std::nullopt;
}

} // namespace lmms
