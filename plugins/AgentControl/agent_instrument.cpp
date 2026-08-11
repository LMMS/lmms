/*
 * agent_instrument.cpp - instrument domain tools (v2): presets, VST, SF2,
 * generic param get/set, instrument introspection.
 */

#include "AgentControl.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QMetaObject>

#include "Effect.h"
#include "EffectChain.h"
#include "EffectControls.h"
#include "Engine.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "SampleTrack.h"

namespace lmms
{

std::optional<QJsonObject> AgentControlService::dispatchInstrumentTool(
	const QString& tool, const QJsonObject& args )
{
	QJsonObject result;
	QString error;

	// ---- generic param get/set ---------------------------------------------

	if( tool == "getparam" || tool == "setparam" )
	{
		const QString address = args.value( "address" ).toString().trimmed();
		if( address.isEmpty() )
		{
			return errorResponse( "bad_args", tr( "needs address" ) );
		}
		QString canonical;
		QString resolveError;
		AutomatableModel* model = resolveModelAddress( address, canonical, resolveError );
		if( model == nullptr )
		{
			return errorResponse( "bad_address", resolveError );
		}
		if( tool == "getparam" )
		{
			return successResponse( describeModel( model, canonical ) );
		}
		if( !args.contains( "value" ) )
		{
			return errorResponse( "bad_args", tr( "set_param needs value" ) );
		}
		const double value = args.value( "value" ).toDouble();
		const double min = model->minValue<double>();
		const double max = model->maxValue<double>();
		if( value < min || value > max )
		{
			return errorResponse( "bad_args",
				tr( "value %1 out of range [%2, %3]" ).arg( value ).arg( min ).arg( max ) );
		}
		model->setValue( static_cast<float>( value ) );
		result["address"] = canonical;
		result["value"] = model->value<double>();
		return successResponse( result );
	}

	InstrumentTrack* track = nullptr;
	if( tool == "loadinstrumentpreset" || tool == "setvstprogram"
		|| tool == "setvstparam" || tool == "setsf2patch"
		|| tool == "describeinstrument" )
	{
		track = resolveInstrumentTrackOrLast( args, error );
		if( track == nullptr ) { return errorResponse( "tool_failed", error ); }
	}

	if( tool == "loadinstrumentpreset" )
	{
		const QString path = args.value( "path" ).toString().trimmed();
		if( path.isEmpty() ) { return errorResponse( "bad_args", tr( "load_instrument_preset needs path" ) ); }
		if( !QFileInfo::exists( path ) ) { return errorResponse( "bad_args", tr( "file not found: %1" ).arg( path ) ); }
		Instrument* instrument = track->instrument();
		if( instrument == nullptr )
		{
			const QString plugin = args.value( "plugin" ).toString().trimmed();
			if( plugin.isEmpty() ) { return errorResponse( "tool_failed", tr( "no instrument loaded; pass plugin" ) ); }
			QString displayName;
			const QString resolved = resolveInstrumentPlugin( plugin, displayName );
			if( resolved.isEmpty() ) { return errorResponse( "tool_failed", tr( "unknown instrument plugin: %1" ).arg( plugin ) ); }
			track->loadInstrument( resolved, nullptr );
			instrument = track->instrument();
		}
		if( instrument == nullptr ) { return errorResponse( "tool_failed", tr( "could not instantiate instrument" ) ); }
		instrument->loadFile( path );
		result["track"] = track->name();
		result["preset"] = path;
		result["instrument"] = instrument->displayName();
		return successResponse( result );
	}

	if( tool == "setvstprogram" )
	{
		if( !args.contains( "program" ) ) { return errorResponse( "bad_args", tr( "set_vst_program needs program" ) ); }
		const int program = args.value( "program" ).toInt();
		Instrument* instrument = track->instrument();
		if( instrument == nullptr ) { return errorResponse( "tool_failed", tr( "no instrument loaded" ) ); }
		// VST hosts expose nextProgram/previousProgram slots; step relative to
		// the current program (index 0 = no change, positive = forward).
		const int hasNext = instrument->metaObject()->indexOfSlot( "nextProgram()" );
		const int hasPrev = instrument->metaObject()->indexOfSlot( "previousProgram()" );
		if( hasNext < 0 || hasPrev < 0 )
		{
			return errorResponse( "not_implemented", tr( "instrument is not a VST host" ) );
		}
		int steps = qBound( -128, program, 128 );
		const bool forward = steps >= 0;
		steps = qAbs( steps );
		for( int i = 0; i < steps; ++i )
		{
			QMetaObject::invokeMethod( instrument,
				forward ? "nextProgram" : "previousProgram", Qt::DirectConnection );
		}
		result["track"] = track->name();
		result["program"] = program;
		return successResponse( result );
	}

	if( tool == "setvstparam" )
	{
		if( !args.contains( "param" ) || !args.contains( "value" ) )
		{
			return errorResponse( "bad_args", tr( "set_vst_param needs param and value" ) );
		}
		Instrument* instrument = track->instrument();
		if( instrument == nullptr ) { return errorResponse( "tool_failed", tr( "no instrument loaded" ) ); }
		// VST knobs are FloatModels parented to the instrument; match by
		// display name or numeric index.
		const QString paramRef = args.value( "param" ).toString();
		const float value = static_cast<float>( args.value( "value" ).toDouble() );
		bool numeric = false;
		const int index = paramRef.toInt( &numeric );
		const auto models = instrument->findChildren<AutomatableModel*>();
		AutomatableModel* target = nullptr;
		int seen = 0;
		for( auto* m : models )
		{
			if( m == nullptr ) { continue; }
			if( !numeric && normalizeName( m->displayName() ) == normalizeName( paramRef ) )
			{
				target = m;
				break;
			}
			if( numeric && seen == index ) { target = m; break; }
			++seen;
		}
		if( target == nullptr ) { return errorResponse( "tool_failed", tr( "VST param not found: %1" ).arg( paramRef ) ); }
		target->setValue( value );
		result["param"] = paramRef;
		result["value"] = target->value<double>();
		return successResponse( result );
	}

	if( tool == "setsf2patch" )
	{
		Instrument* instrument = track->instrument();
		if( instrument == nullptr ) { return errorResponse( "tool_failed", tr( "no instrument loaded" ) ); }
		if( args.contains( "bank" ) )
		{
			AutomatableModel* bank = instrument->childModel( "bank" );
			if( bank == nullptr ) { return errorResponse( "tool_failed", tr( "instrument has no bank selector" ) ); }
			bank->setValue( static_cast<float>( args.value( "bank" ).toInt() ) );
		}
		if( args.contains( "patch" ) )
		{
			AutomatableModel* patch = instrument->childModel( "patch" );
			if( patch == nullptr ) { return errorResponse( "tool_failed", tr( "instrument has no patch selector" ) ); }
			patch->setValue( static_cast<float>( args.value( "patch" ).toInt() ) );
		}
		result["track"] = track->name();
		result["instrument"] = instrument->displayName();
		return successResponse( result );
	}

	if( tool == "describeinstrument" )
	{
		Instrument* instrument = track->instrument();
		if( instrument == nullptr )
		{
			result["track"] = track->name();
			result["instrument"] = QJsonValue::Null;
			result["params"] = QJsonArray();
			return successResponse( result );
		}
		QJsonArray params;
		const auto models = instrument->findChildren<AutomatableModel*>();
		int seen = 0;
		for( auto* m : models )
		{
			if( m == nullptr ) { continue; }
			QJsonObject o = describeModel( m, QString( "inst:%1" ).arg( m->displayName() ) );
			o["index"] = seen;
			params.append( o );
			++seen;
		}
		result["track"] = track->name();
		result["instrument"] = instrument->displayName();
		result["params"] = params;
		return successResponse( result );
	}

	return std::nullopt;
}

} // namespace lmms
