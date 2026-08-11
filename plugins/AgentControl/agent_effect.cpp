/*
 * agent_effect.cpp - effect domain tools (v2): params, order, enable, wet/dry.
 */

#include "AgentControl.h"

#include <QJsonArray>

#include "Effect.h"
#include "EffectChain.h"
#include "EffectControls.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "SampleTrack.h"
#include "Track.h"

namespace lmms
{

std::optional<QJsonObject> AgentControlService::dispatchEffectTool(
	const QString& tool, const QJsonObject& args )
{
	QString error;

	EffectChain* chain = nullptr;
	QString chainLabel;
	Effect* effect = nullptr;

	if( tool == "seteffectparam" || tool == "geteffectparams"
		|| tool == "moveeffect" || tool == "seteffectenabled"
		|| tool == "seteffectwetdry" || tool == "describeeffect" )
	{
		const QString effectName = args.value( "effect" ).toString().trimmed();
		if( effectName.isEmpty() )
		{
			return errorResponse( "bad_args", tr( "needs effect name" ) );
		}
		if( args.contains( "channel" ) )
		{
			MixerChannel* channel = resolveMixerChannelRef( args.value( "channel" ).toString(), error );
			if( channel == nullptr ) { return errorResponse( "tool_failed", error ); }
			chain = &channel->m_fxChain;
			chainLabel = tr( "channel %1" ).arg( channel->m_name );
		}
		else
		{
			QJsonObject trackArgs = args;
			trackArgs.remove( "effect" );
			Track* track = resolveTrackRef( trackArgs );
			if( track == nullptr ) { return errorResponse( "tool_failed", tr( "track not found" ) ); }
			chain = effectChainForTrack( track );
			if( chain == nullptr ) { return errorResponse( "tool_failed", tr( "track has no effect chain" ) ); }
			chainLabel = track->name();
		}
		effect = findEffectInChain( chain, effectName );
		if( effect == nullptr )
		{
			return errorResponse( "tool_failed", tr( "effect not found on %1: %2" ).arg( chainLabel, effectName ) );
		}
	}

	QJsonObject result;
	result["chain"] = chainLabel;

	if( tool == "seteffectparam" )
	{
		const QString paramName = args.value( "param" ).toString().trimmed();
		if( paramName.isEmpty() || !args.contains( "value" ) )
		{
			return errorResponse( "bad_args", tr( "set_effect_param needs param and value" ) );
		}
		EffectControls* controls = effect->controls();
		if( controls == nullptr ) { return errorResponse( "tool_failed", tr( "effect exposes no controls" ) ); }
		AutomatableModel* model = findParamByName( controls, paramName );
		if( model == nullptr )
		{
			return errorResponse( "tool_failed", tr( "param not found on %1: %2" ).arg( effect->displayName(), paramName ) );
		}
		const double value = args.value( "value" ).toDouble();
		const double min = model->minValue<double>();
		const double max = model->maxValue<double>();
		if( value < min || value > max )
		{
			return errorResponse( "bad_args", tr( "value %1 out of range [%2, %3]" ).arg( value ).arg( min ).arg( max ) );
		}
		model->setValue( static_cast<float>( value ) );
		result["effect"] = effect->displayName();
		result["param"] = model->displayName();
		result["value"] = model->value<double>();
		return successResponse( result );
	}

	if( tool == "geteffectparams" )
	{
		EffectControls* controls = effect->controls();
		QJsonArray params;
		if( controls != nullptr )
		{
			const auto models = controls->findChildren<AutomatableModel*>(
				QString(), Qt::FindDirectChildrenOnly );
			for( auto* m : models )
			{
				if( m == nullptr ) { continue; }
				params.append( describeModel( m,
					QString( "fx:%1.%2" ).arg( chainLabel, m->displayName() ) ) );
			}
		}
		result["effect"] = effect->displayName();
		result["params"] = params;
		return successResponse( result );
	}

	if( tool == "moveeffect" )
	{
		const QString direction = args.value( "direction" ).toString().trimmed();
		if( direction == "up" ) { chain->moveUp( effect ); }
		else if( direction == "down" ) { chain->moveDown( effect ); }
		else { return errorResponse( "bad_args", tr( "direction must be up|down" ) ); }
		result["effect"] = effect->displayName();
		result["direction"] = direction;
		return successResponse( result );
	}

	if( tool == "seteffectenabled" )
	{
		if( !args.contains( "enabled" ) ) { return errorResponse( "bad_args", tr( "set_effect_enabled needs enabled" ) ); }
		effect->setEnabled( args.value( "enabled" ).toBool() );
		result["effect"] = effect->displayName();
		result["enabled"] = effect->isEnabled();
		return successResponse( result );
	}

	if( tool == "seteffectwetdry" )
	{
		const double value = args.value( "value" ).toDouble( -1.0 );
		if( value < 0.0 || value > 1.0 )
		{
			return errorResponse( "bad_args", tr( "wet/dry must be in 0..1" ) );
		}
		effect->setWetDryLevel( static_cast<float>( value ) );
		result["effect"] = effect->displayName();
		result["value"] = value;
		return successResponse( result );
	}

	if( tool == "describeeffect" )
	{
		EffectControls* controls = effect->controls();
		QJsonArray params;
		if( controls != nullptr )
		{
			const auto models = controls->findChildren<AutomatableModel*>(
				QString(), Qt::FindDirectChildrenOnly );
			for( auto* m : models )
			{
				if( m == nullptr ) { continue; }
				params.append( describeModel( m,
					QString( "fx:%1.%2" ).arg( chainLabel, m->displayName() ) ) );
			}
		}
		result["effect"] = effect->displayName();
		result["enabled"] = effect->isEnabled();
		result["wetdry"] = effect->wetLevel();
		result["params"] = params;
		return successResponse( result );
	}

	return std::nullopt;
}

} // namespace lmms
