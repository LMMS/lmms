/*
 * agent_misc.cpp - misc domain tools (v2): describe_song/describe_track,
 * project notes, microtuner.
 */

#include "AgentControl.h"

#include <QJsonArray>

#include "AutomationTrack.h"
#include "EffectChain.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "Microtuner.h"
#include "MidiClip.h"
#include "Mixer.h"
#include "PatternStore.h"
#include "ProjectNotes.h"
#include "SampleTrack.h"
#include "Song.h"
#include "Track.h"

namespace lmms
{

std::optional<QJsonObject> AgentControlService::dispatchMiscTool(
	const QString& tool, const QJsonObject& args )
{
	Song* song = Engine::getSong();
	if( song == nullptr ) { return errorResponse( "tool_failed", tr( "no song" ) ); }
	QJsonObject result;
	QString error;

	if( tool == "describetrack" )
	{
		QString resolveError;
		InstrumentTrack* track = resolveInstrumentTrackOrLast( args, resolveError );
		if( track == nullptr )
		{
			// fall back to any track type for a basic description
			Track* any = resolveTrackRef( args );
			if( any == nullptr ) { return errorResponse( "tool_failed", resolveError ); }
			result["track"] = any->name();
			result["type"] = trackTypeName( any->type() );
			QJsonArray clips;
			for( Clip* c : any->getClips() )
			{
				QJsonObject o;
				o["index"] = any->getClipNum( c );
				o["name"] = c->name();
				o["start"] = c->startPosition().getTicks();
				o["length"] = c->length().getTicks();
				clips.append( o );
			}
			result["clips"] = clips;
			return successResponse( result );
		}
		QJsonObject describe = describeModelsForTrack( track, track->name() );
		QJsonArray clips;
		for( Clip* c : track->getClips() )
		{
			auto* mc = dynamic_cast<MidiClip*>( c );
			QJsonObject o;
			o["index"] = track->getClipNum( c );
			o["name"] = c->name();
			o["start"] = c->startPosition().getTicks();
			o["length"] = c->length().getTicks();
			if( mc != nullptr )
			{
				o["notes"] = static_cast<int>( mc->notes().size() );
			}
			clips.append( o );
		}
		describe["clips"] = clips;
		if( track->instrument() != nullptr )
		{
			describe["instrument"] = track->instrument()->displayName();
		}
		return successResponse( describe );
	}

	if( tool == "describesong" )
	{
		result["project_file"] = song->projectFileName();
		result["tempo"] = song->tempoModel().value();
		result["time_signature"] = QJsonArray{ song->getTimeSigModel().getNumerator(),
			song->getTimeSigModel().getDenominator() };
		result["master_volume"] = song->masterVolumeModel().value();
		result["master_pitch"] = song->masterPitchModel().value();
		result["length"] = song->length();

		QJsonArray tracks;
		for( Track* t : song->tracks() )
		{
			QJsonObject o;
			o["name"] = t->name();
			o["type"] = trackTypeName( t->type() );
			o["clips"] = static_cast<int>( t->getClips().size() );
			tracks.append( o );
		}
		result["tracks"] = tracks;

		result["patterns"] = Engine::patternStore()->numOfPatterns();

		QJsonArray mixer;
		Mixer* m = Engine::mixer();
		if( m != nullptr )
		{
			for( mix_ch_t i = 0; i < m->numChannels(); ++i )
			{
				QJsonObject o;
				o["index"] = i;
				o["name"] = m->mixerChannel( i )->m_name;
				mixer.append( o );
			}
		}
		result["mixer_channels"] = mixer;

		QJsonArray controllers;
		for( Controller* c : song->controllers() )
		{
			controllers.append( c->displayName() );
		}
		result["controllers"] = controllers;

		QJsonArray automation;
		for( Track* t : song->tracks() )
		{
			if( dynamic_cast<AutomationTrack*>( t ) == nullptr ) { continue; }
			automation.append( t->name() );
		}
		result["automation_tracks"] = automation;

		if( gui::getGUI() != nullptr && gui::getGUI()->getProjectNotes() != nullptr )
		{
			result["project_notes"] = gui::getGUI()->getProjectNotes()->text();
		}
		return successResponse( result );
	}

	if( tool == "setprojectnotes" )
	{
		const QString text = args.value( "text" ).toString();
		if( gui::getGUI() == nullptr || gui::getGUI()->getProjectNotes() == nullptr )
		{
			return errorResponse( "not_available", tr( "project notes window not available" ) );
		}
		gui::getGUI()->getProjectNotes()->setText( text );
		result["message"] = tr( "project notes updated" );
		return successResponse( result );
	}

	if( tool == "getprojectnotes" )
	{
		if( gui::getGUI() == nullptr || gui::getGUI()->getProjectNotes() == nullptr )
		{
			return errorResponse( "not_available", tr( "project notes window not available" ) );
		}
		result["text"] = gui::getGUI()->getProjectNotes()->text();
		return successResponse( result );
	}

	if( tool == "setmicrotuner" )
	{
		QString resolveError;
		InstrumentTrack* track = resolveInstrumentTrackOrLast( args, resolveError );
		if( track == nullptr ) { return errorResponse( "tool_failed", resolveError ); }
		Microtuner* microtuner = track->microtuner();
		if( microtuner == nullptr ) { return errorResponse( "tool_failed", tr( "no microtuner" ) ); }
		if( args.contains( "enabled" ) )
		{
			microtuner->enabledModel()->setValue( args.value( "enabled" ).toBool() ? 1.0f : 0.0f );
			result["enabled"] = args.value( "enabled" );
		}
		if( args.contains( "scale" ) )
		{
			const QString scale = args.value( "scale" ).toString().trimmed();
			ComboBoxModel& model = *microtuner->scaleModel();
			int index = -1;
			for( int i = model.minValue(); i <= model.maxValue(); ++i )
			{
				if( normalizeName( model.itemText( i ) ) == normalizeName( scale ) )
				{
					index = i;
					break;
				}
			}
			if( index < 0 ) { return errorResponse( "bad_args", tr( "unknown scale: %1" ).arg( scale ) ); }
			model.setValue( index );
			result["scale"] = scale;
		}
		if( args.contains( "keymap" ) )
		{
			const QString keymap = args.value( "keymap" ).toString().trimmed();
			ComboBoxModel& model = *microtuner->keymapModel();
			int index = -1;
			for( int i = model.minValue(); i <= model.maxValue(); ++i )
			{
				if( normalizeName( model.itemText( i ) ) == normalizeName( keymap ) )
				{
					index = i;
					break;
				}
			}
			if( index < 0 ) { return errorResponse( "bad_args", tr( "unknown keymap: %1" ).arg( keymap ) ); }
			model.setValue( index );
			result["keymap"] = keymap;
		}
		if( !args.contains( "enabled" ) && !args.contains( "scale" ) && !args.contains( "keymap" ) )
		{
			return errorResponse( "bad_args", tr( "set_microtuner needs enabled/scale/keymap" ) );
		}
		result["track"] = track->name();
		return successResponse( result );
	}

	return std::nullopt;
}

} // namespace lmms
