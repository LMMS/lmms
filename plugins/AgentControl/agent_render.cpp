/*
 * agent_render.cpp - render & export domain tools (v2)
 */

#include "AgentControl.h"

#include <QFileInfo>
#include <QJsonArray>

#include "Engine.h"
#include "OutputSettings.h"
#include "ProjectRenderer.h"
#include "Song.h"

namespace lmms
{

namespace
{

OutputSettings outputSettingsFromArgs( const QJsonObject& args )
{
	const int sampleRate = qBound( 44100, args.value( "sample_rate" ).toInt( 44100 ), 192000 );
	const int bitrate = qBound( 64, args.value( "bitrate" ).toInt( 192 ), 320 );
	OutputSettings::BitDepth depth = OutputSettings::BitDepth::Depth16Bit;
	const QString depthName = args.value( "bit_depth" ).toString();
	if( depthName == "24" ) { depth = OutputSettings::BitDepth::Depth24Bit; }
	else if( depthName == "32" ) { depth = OutputSettings::BitDepth::Depth32Bit; }
	OutputSettings::StereoMode stereo = OutputSettings::StereoMode::Stereo;
	const QString stereoName = args.value( "stereo_mode" ).toString();
	if( stereoName == "mono" ) { stereo = OutputSettings::StereoMode::Mono; }
	else if( stereoName == "joint_stereo" ) { stereo = OutputSettings::StereoMode::JointStereo; }
	return OutputSettings( sampleRate, bitrate, depth, stereo );
}

} // namespace

std::optional<QJsonObject> AgentControlService::dispatchRenderTool(
	const QString& tool, const QJsonObject& args )
{
	QJsonObject result;
	QString error;

	if( tool == "rendersong" || tool == "rendertracks" || tool == "renderpreview" )
	{
		const QString path = args.value( "path" ).toString().trimmed();
		if( path.isEmpty() ) { return errorResponse( "bad_args", tr( "needs path" ) ); }
		if( QFileInfo::exists( path ) && !args.value( "overwrite" ).toBool( false ) )
		{
			return errorResponse( "confirm", tr( "file exists: %1 (pass overwrite=true to replace)" ).arg( path ) );
		}
		const OutputSettings settings = outputSettingsFromArgs( args );
		const bool tracksMode = tool == "rendertracks";
		if( !startRender( settings, path, tracksMode, m_renderJob, error ) )
		{
			return errorResponse( "tool_failed", error );
		}
		if( tool == "renderpreview" )
		{
			Song* song = Engine::getSong();
			if( song != nullptr && song->getTimeline( Song::PlayMode::Song ).loopEnabled() )
			{
				song->setRenderBetweenMarkers( true );
				m_renderJob.restoreExportSettings = true;
			}
		}
		result["render_id"] = m_renderJob.id;
		result["path"] = m_renderJob.outputPath;
		result["mode"] = tracksMode ? "tracks" : "song";
		return successResponse( result );
	}

	if( tool == "getrenderprogress" )
	{
		const QString id = args.value( "render_id" ).toString();
		if( !id.isEmpty() && id != m_renderJob.id )
		{
			return errorResponse( "not_found", tr( "unknown render_id: %1" ).arg( id ) );
		}
		result["render_id"] = m_renderJob.id;
		result["progress"] = m_renderJob.progress;
		result["done"] = m_renderJob.done;
		result["path"] = m_renderJob.outputPath;
		if( m_renderJob.error.isEmpty() ) { result["error"] = QJsonValue::Null; }
		else { result["error"] = m_renderJob.error; }
		return successResponse( result );
	}

	if( tool == "cancelrender" )
	{
		if( m_renderJob.manager == nullptr || m_renderJob.done )
		{
			return successResponse( QJsonObject{ { "message", tr( "no active render" ) } } );
		}
		m_renderJob.cancelled = true;
		m_renderJob.manager->abortProcessing();
		result["render_id"] = m_renderJob.id;
		result["cancelled"] = true;
		return successResponse( result );
	}

	if( tool == "exportmidi" )
	{
		const QString path = args.value( "path" ).toString().trimmed();
		if( path.isEmpty() ) { return errorResponse( "bad_args", tr( "export_midi needs path" ) ); }
		Engine::getSong()->exportProjectMidi( path );
		result["path"] = path;
		return successResponse( result );
	}

	return std::nullopt;
}

} // namespace lmms
