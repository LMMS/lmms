/*
 * agent_sound.cpp - sound shaping domain tools (v2): envelope, filter, LFO,
 * arpeggio, note stacking.
 */

#include "AgentControl.h"

#include <QJsonArray>

#include "ComboBoxModel.h"
#include "Engine.h"
#include "InstrumentTrack.h"

namespace lmms
{

std::optional<QJsonObject> AgentControlService::dispatchSoundTool(
	const QString& tool, const QJsonObject& args )
{
	QString error;
	InstrumentTrack* track = resolveInstrumentTrackOrLast( args, error );
	if( track == nullptr ) { return errorResponse( "tool_failed", error ); }

	const QString trackName = track->name();
	auto setPath = [&]( const QString& path, double value ) -> std::optional<QJsonObject>
	{
		QString resolveError;
		AutomatableModel* model = modelForTrackPath( track, path.split( '.' ), resolveError );
		if( model == nullptr )
		{
			return errorResponse( "bad_address", resolveError );
		}
		const double min = model->minValue<double>();
		const double max = model->maxValue<double>();
		if( value < min || value > max )
		{
			return errorResponse( "bad_args",
				tr( "%1 out of range [%2, %3]" ).arg( path ).arg( min ).arg( max ) );
		}
		model->setValue( static_cast<float>( value ) );
		return std::nullopt;
	};

	QJsonObject result;
	result["track"] = trackName;

	if( tool == "setenvelope" )
	{
		QString target = args.value( "target" ).toString( "volume" );
		if( target != "volume" && target != "cutoff" && target != "resonance" )
		{
			return errorResponse( "bad_args", tr( "target must be volume|cutoff|resonance" ) );
		}
		const QString prefix = target == "volume" ? "env" : QString( "env.%1" ).arg( target );
		struct EnvParam { const char* name; const char* field; };
		const EnvParam params[] =
		{
			{ "predelay", "predelay" }, { "attack", "attack" }, { "hold", "hold" },
			{ "decay", "decay" }, { "sustain", "sustain" }, { "release", "release" },
			{ "amount", "amount" }
		};
		QJsonObject applied;
		for( const auto& p : params )
		{
			if( args.contains( p.field ) )
			{
				if( auto bad = setPath( QString( "%1.%2" ).arg( prefix, p.name ),
					args.value( p.field ).toDouble() ) )
				{
					return *bad;
				}
				applied[p.field] = args.value( p.field );
			}
		}
		if( applied.isEmpty() )
		{
			return errorResponse( "bad_args", tr( "set_envelope needs at least one envelope parameter" ) );
		}
		result["target"] = target;
		result["applied"] = applied;
		return successResponse( result );
	}

	if( tool == "setfilter" )
	{
		if( args.contains( "enabled" ) )
		{
			if( auto bad = setPath( "filter.enabled", args.value( "enabled" ).toBool() ? 1.0 : 0.0 ) ) { return *bad; }
		}
		if( args.contains( "type" ) )
		{
			const QString type = args.value( "type" ).toString().trimmed();
			ComboBoxModel& filterModel = track->soundShaping()->getFilterModel();
			int index = -1;
			for( int i = filterModel.minValue(); i <= filterModel.maxValue(); ++i )
			{
				if( normalizeName( filterModel.itemText( i ) ) == normalizeName( type ) )
				{
					index = i;
					break;
				}
			}
			if( index < 0 ) { return errorResponse( "bad_args", tr( "unknown filter type: %1" ).arg( type ) ); }
			filterModel.setValue( index );
			result["type"] = type;
		}
		if( args.contains( "cutoff" ) )
		{
			if( auto bad = setPath( "filter.cutoff", args.value( "cutoff" ).toDouble() ) ) { return *bad; }
		}
		if( args.contains( "resonance" ) )
		{
			if( auto bad = setPath( "filter.reso", args.value( "resonance" ).toDouble() ) ) { return *bad; }
		}
		if( !args.contains( "enabled" ) && !args.contains( "type" )
			&& !args.contains( "cutoff" ) && !args.contains( "resonance" ) )
		{
			return errorResponse( "bad_args", tr( "set_filter needs enabled/type/cutoff/resonance" ) );
		}
		return successResponse( result );
	}

	if( tool == "setlfo" )
	{
		QString target = args.value( "target" ).toString( "volume" );
		if( target != "volume" && target != "cutoff" && target != "resonance" )
		{
			return errorResponse( "bad_args", tr( "target must be volume|cutoff|resonance" ) );
		}
		const QString prefix = target == "volume" ? "lfo" : QString( "lfo.%1" ).arg( target );
		struct LfoParam { const char* name; const char* field; };
		const LfoParam params[] =
		{
			{ "amount", "amount" }, { "speed", "speed" }, { "wave", "wave" },
			{ "delay", "delay" }, { "attack", "attack" }
		};
		QJsonObject applied;
		for( const auto& p : params )
		{
			if( args.contains( p.field ) )
			{
				if( auto bad = setPath( QString( "%1.%2" ).arg( prefix, p.name ),
					args.value( p.field ).toDouble() ) )
				{
					return *bad;
				}
				applied[p.field] = args.value( p.field );
			}
		}
		if( applied.isEmpty() )
		{
			return errorResponse( "bad_args", tr( "set_lfo needs at least one LFO parameter" ) );
		}
		result["target"] = target;
		result["applied"] = applied;
		return successResponse( result );
	}

	if( tool == "setarp" )
	{
		InstrumentFunctionArpeggio* arp = track->arpeggio();
		QJsonObject applied;
		if( args.contains( "enabled" ) )
		{
			arp->arpEnabledModel().setValue( args.value( "enabled" ).toBool() ? 1.0f : 0.0f );
			applied["enabled"] = args.value( "enabled" );
		}
		if( args.contains( "chord" ) )
		{
			const QString chord = args.value( "chord" ).toString().trimmed();
			ComboBoxModel& model = arp->arpModel();
			int index = -1;
			for( int i = model.minValue(); i <= model.maxValue(); ++i )
			{
				if( normalizeName( model.itemText( i ) ) == normalizeName( chord ) )
				{
					index = i;
					break;
				}
			}
			if( index < 0 ) { return errorResponse( "bad_args", tr( "unknown arp chord: %1" ).arg( chord ) ); }
			model.setValue( index );
			applied["chord"] = chord;
		}
		if( args.contains( "range" ) ) { arp->arpRangeModel().setValue( static_cast<float>( args.value( "range" ).toDouble() ) ); applied["range"] = args.value( "range" ); }
		if( args.contains( "direction" ) )
		{
			const QString dir = args.value( "direction" ).toString().trimmed();
			ComboBoxModel& model = arp->arpDirectionModel();
			int index = -1;
			for( int i = model.minValue(); i <= model.maxValue(); ++i )
			{
				if( normalizeName( model.itemText( i ) ) == normalizeName( dir ) )
				{
					index = i;
					break;
				}
			}
			if( index < 0 ) { return errorResponse( "bad_args", tr( "unknown arp direction: %1" ).arg( dir ) ); }
			model.setValue( index );
			applied["direction"] = dir;
		}
		if( args.contains( "time" ) ) { arp->arpTimeModel().setValue( static_cast<float>( args.value( "time" ).toDouble() ) ); applied["time"] = args.value( "time" ); }
		if( args.contains( "gate" ) ) { arp->arpGateModel().setValue( static_cast<float>( args.value( "gate" ).toDouble() ) ); applied["gate"] = args.value( "gate" ); }
		if( args.contains( "mode" ) )
		{
			const QString mode = args.value( "mode" ).toString().trimmed();
			ComboBoxModel& model = arp->arpModeModel();
			int index = -1;
			for( int i = model.minValue(); i <= model.maxValue(); ++i )
			{
				if( normalizeName( model.itemText( i ) ) == normalizeName( mode ) )
				{
					index = i;
					break;
				}
			}
			if( index < 0 ) { return errorResponse( "bad_args", tr( "unknown arp mode: %1" ).arg( mode ) ); }
			model.setValue( index );
			applied["mode"] = mode;
		}
		if( applied.isEmpty() ) { return errorResponse( "bad_args", tr( "set_arp needs at least one parameter" ) ); }
		result["applied"] = applied;
		return successResponse( result );
	}

	if( tool == "setnotestacking" )
	{
		InstrumentFunctionNoteStacking* ns = track->noteStacking();
		QJsonObject applied;
		if( args.contains( "enabled" ) )
		{
			ns->chordsEnabledModel().setValue( args.value( "enabled" ).toBool() ? 1.0f : 0.0f );
			applied["enabled"] = args.value( "enabled" );
		}
		if( args.contains( "chord" ) )
		{
			const QString chord = args.value( "chord" ).toString().trimmed();
			ComboBoxModel& model = ns->chordsModel();
			int index = -1;
			for( int i = model.minValue(); i <= model.maxValue(); ++i )
			{
				if( normalizeName( model.itemText( i ) ) == normalizeName( chord ) )
				{
					index = i;
					break;
				}
			}
			if( index < 0 ) { return errorResponse( "bad_args", tr( "unknown chord: %1" ).arg( chord ) ); }
			model.setValue( index );
			applied["chord"] = chord;
		}
		if( args.contains( "range" ) ) { ns->chordRangeModel().setValue( static_cast<float>( args.value( "range" ).toDouble() ) ); applied["range"] = args.value( "range" ); }
		if( applied.isEmpty() ) { return errorResponse( "bad_args", tr( "set_note_stacking needs at least one parameter" ) ); }
		result["applied"] = applied;
		return successResponse( result );
	}

	return std::nullopt;
}

} // namespace lmms
