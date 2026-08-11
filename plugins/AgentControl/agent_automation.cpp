/*
 * agent_automation.cpp - automation & controller domain tools (v2)
 */

#include "AgentControl.h"

#include <QJsonArray>

#include "AutomationClip.h"
#include "AutomationTrack.h"
#include "Controller.h"
#include "ControllerConnection.h"
#include "Engine.h"
#include "LfoController.h"
#include "MidiController.h"
#include "Song.h"
#include "Track.h"

namespace lmms
{

namespace
{

AutomationClip::ProgressionType progressionFromString( const QString& name )
{
	if( name == "discrete" ) { return AutomationClip::ProgressionType::Discrete; }
	if( name == "cubic" ) { return AutomationClip::ProgressionType::CubicHermite; }
	return AutomationClip::ProgressionType::Linear;
}

} // namespace

std::optional<QJsonObject> AgentControlService::dispatchAutomationTool(
	const QString& tool, const QJsonObject& args )
{
	Song* song = Engine::getSong();
	if( song == nullptr ) { return errorResponse( "tool_failed", tr( "no song" ) ); }
	QJsonObject result;
	QString error;

	if( tool == "createautomation" )
	{
		const QString address = args.value( "address" ).toString().trimmed();
		if( address.isEmpty() ) { return errorResponse( "bad_args", tr( "create_automation needs address" ) ); }
		QString canonical;
		QString resolveError;
		AutomatableModel* model = resolveModelAddress( address, canonical, resolveError );
		if( model == nullptr ) { return errorResponse( "bad_address", resolveError ); }
		Track* track = Track::create( Track::Type::Automation, song );
		if( track == nullptr ) { return errorResponse( "tool_failed", tr( "could not create automation track" ) ); }
		auto* automationTrack = dynamic_cast<AutomationTrack*>( track );
		AutomationClip* clip = automationTrack != nullptr
			? dynamic_cast<AutomationClip*>( automationTrack->createClip( TimePos( 0 ) ) )
			: nullptr;
		if( clip == nullptr ) { return errorResponse( "tool_failed", tr( "could not create automation clip" ) ); }
		clip->addObject( model );
		const float normalized = model->inverseScaledValue( model->value<double>() );
		clip->putValue( TimePos( 0 ), qBound( 0.0f, normalized, 1.0f ) );
		const QString name = args.value( "name" ).toString().trimmed();
		if( !name.isEmpty() ) { track->setName( name ); }
		result["clip"] = automationClipRef( clip );
		result["address"] = canonical;
		result["track"] = track->name();
		return successResponse( result );
	}

	if( tool == "automate" || tool == "globalautomate" )
	{
		const QString address = args.value( "address" ).toString().trimmed();
		const QJsonArray ticks = args.value( "ticks" ).toArray();
		const QJsonArray values = args.value( "values" ).toArray();
		if( address.isEmpty() || ticks.isEmpty() || values.isEmpty() || ticks.size() != values.size() )
		{
			return errorResponse( "bad_args", tr( "automate needs address, ticks[], values[] (same length)" ) );
		}
		QString canonical;
		QString resolveError;
		AutomatableModel* model = resolveModelAddress( address, canonical, resolveError );
		if( model == nullptr ) { return errorResponse( "bad_address", resolveError ); }

		AutomationClip* clip = nullptr;
		if( args.contains( "clip" ) )
		{
			clip = resolveAutomationClipRef( args.value( "clip" ).toString(), error );
			if( clip == nullptr ) { return errorResponse( "tool_failed", error ); }
			clip->addObject( model );
		}
		else
		{
			clip = AutomationClip::globalAutomationClip( model );
		}
		if( clip == nullptr ) { return errorResponse( "tool_failed", tr( "could not resolve automation clip" ) ); }

		int added = 0;
		for( int i = 0; i < ticks.size(); ++i )
		{
			const int tick = ticks[i].toInt( -1 );
			const double value = values[i].toDouble( -1.0 );
			if( tick < 0 || value < 0.0 || value > 1.0 ) { continue; }
			clip->putValue( TimePos( tick ), static_cast<float>( value ) );
			++added;
		}
		result["clip"] = automationClipRef( clip );
		result["address"] = canonical;
		result["nodes_added"] = added;
		return successResponse( result );
	}

	if( tool == "setautomationnode" || tool == "removeautomationnode"
		|| tool == "setautomationtension" || tool == "setautomationprogression" )
	{
		const QString clipRef = args.value( "clip" ).toString().trimmed();
		if( clipRef.isEmpty() ) { return errorResponse( "bad_args", tr( "needs clip" ) ); }
		AutomationClip* clip = resolveAutomationClipRef( clipRef, error );
		if( clip == nullptr ) { return errorResponse( "tool_failed", error ); }

		if( tool == "setautomationnode" )
		{
			const int tick = args.value( "tick" ).toInt( -1 );
			const double value = args.value( "value" ).toDouble( -1.0 );
			if( tick < 0 || value < 0.0 || value > 1.0 )
			{
				return errorResponse( "bad_args", tr( "needs tick (>=0) and value (0..1)" ) );
			}
			clip->putValue( TimePos( tick ), static_cast<float>( value ) );
			if( args.contains( "tension" ) )
			{
				clip->setTension( QString::number( qBound( -1.0, args.value( "tension" ).toDouble(), 1.0 ) ) );
			}
			result["clip"] = clipRef;
			result["tick"] = tick;
			result["value"] = value;
			return successResponse( result );
		}
		if( tool == "removeautomationnode" )
		{
			const int tick = args.value( "tick" ).toInt( -1 );
			if( tick < 0 ) { return errorResponse( "bad_args", tr( "needs tick" ) ); }
			clip->removeNode( TimePos( tick ) );
			result["clip"] = clipRef;
			result["tick"] = tick;
			return successResponse( result );
		}
		if( tool == "setautomationtension" )
		{
			if( !args.contains( "tension" ) ) { return errorResponse( "bad_args", tr( "needs tension" ) ); }
			const double tension = qBound( -1.0, args.value( "tension" ).toDouble(), 1.0 );
			clip->setTension( QString::number( tension ) );
			result["clip"] = clipRef;
			result["tension"] = tension;
			return successResponse( result );
		}
		if( tool == "setautomationprogression" )
		{
			const QString progression = args.value( "progression" ).toString().trimmed();
			if( progression != "discrete" && progression != "linear" && progression != "cubic" )
			{
				return errorResponse( "bad_args", tr( "progression must be discrete|linear|cubic" ) );
			}
			clip->setProgressionType( progressionFromString( progression ) );
			result["clip"] = clipRef;
			result["progression"] = progression;
			return successResponse( result );
		}
	}

	if( tool == "readautomation" )
	{
		AutomationClip* clip = nullptr;
		const QString clipRef = args.value( "clip" ).toString().trimmed();
		if( !clipRef.isEmpty() )
		{
			clip = resolveAutomationClipRef( clipRef, error );
			if( clip == nullptr ) { return errorResponse( "tool_failed", error ); }
		}
		else if( args.contains( "address" ) )
		{
			QString canonical;
			QString resolveError;
			AutomatableModel* model = resolveModelAddress(
				args.value( "address" ).toString(), canonical, resolveError );
			if( model == nullptr ) { return errorResponse( "bad_address", resolveError ); }
			const auto clips = AutomationClip::clipsForModel( model );
			if( clips.empty() ) { return errorResponse( "not_found", tr( "no automation for address" ) ); }
			clip = clips.front();
		}
		else { return errorResponse( "bad_args", tr( "needs clip or address" ) ); }

		QJsonArray nodes;
		const auto& timeMap = clip->getTimeMap();
		for( auto it = timeMap.constBegin(); it != timeMap.constEnd(); ++it )
		{
			QJsonObject node;
			node["tick"] = it.key();
			node["value"] = it.value().getInValue();
			nodes.append( node );
		}
		result["clip"] = automationClipRef( clip );
		result["progression"] = clip->progressionType() == AutomationClip::ProgressionType::Discrete
			? "discrete"
			: ( clip->progressionType() == AutomationClip::ProgressionType::CubicHermite ? "cubic" : "linear" );
		result["nodes"] = nodes;
		return successResponse( result );
	}

	if( tool == "listautomation" )
	{
		QJsonArray clips;
		for( Track* track : song->tracks() )
		{
			auto* at = dynamic_cast<AutomationTrack*>( track );
			if( at == nullptr ) { continue; }
			for( Clip* c : track->getClips() )
			{
				auto* clip = dynamic_cast<AutomationClip*>( c );
				if( clip == nullptr ) { continue; }
				QJsonObject o;
				o["clip"] = automationClipRef( clip );
				o["track"] = track->name();
				if( const AutomatableModel* obj = clip->firstObject() )
				{
					o["model"] = obj->displayName();
				}
				clips.append( o );
			}
		}
		result["clips"] = clips;
		return successResponse( result );
	}

	return std::nullopt;
}

// ---------------------------------------------------------------------------
// controllers
// ---------------------------------------------------------------------------

std::optional<QJsonObject> AgentControlService::dispatchControllerTool(
	const QString& tool, const QJsonObject& args )
{
	Song* song = Engine::getSong();
	if( song == nullptr ) { return errorResponse( "tool_failed", tr( "no song" ) ); }
	QJsonObject result;
	QString error;

	if( tool == "createcontroller" )
	{
		const QString type = args.value( "type" ).toString().trimmed();
		Controller* controller = nullptr;
		if( type == "lfo" )
		{
			controller = new LfoController( song );
		}
		else if( type == "midi" )
		{
			controller = new MidiController( song );
		}
		else if( type == "peak" )
		{
			return errorResponse( "not_implemented", tr( "peak controllers are effect-based; add a Peak Controller effect instead" ) );
		}
		else
		{
			return errorResponse( "bad_args", tr( "type must be lfo|midi|peak" ) );
		}
		song->addController( controller );
		const QString name = args.value( "name" ).toString().trimmed();
		if( !name.isEmpty() ) { controller->setName( name ); }
		result["controller"] = controller->displayName();
		result["type"] = type;
		return successResponse( result );
	}

	if( tool == "setlfocontroller" )
	{
		const QString ref = args.value( "controller" ).toString().trimmed();
		if( ref.isEmpty() ) { return errorResponse( "bad_args", tr( "needs controller" ) ); }
		LfoController* lfo = nullptr;
		bool numeric = false;
		const int idx = ref.toInt( &numeric );
		int seen = 0;
		for( Controller* c : song->controllers() )
		{
			if( numeric && seen == idx ) { lfo = dynamic_cast<LfoController*>( c ); break; }
			if( !numeric && c->displayName() == ref ) { lfo = dynamic_cast<LfoController*>( c ); break; }
			++seen;
		}
		if( lfo == nullptr ) { return errorResponse( "tool_failed", tr( "LFO controller not found: %1" ).arg( ref ) ); }
		if( args.contains( "wave" ) ) { lfo->waveModel()->setValue( args.value( "wave" ).toInt() ); }
		if( args.contains( "speed" ) ) { lfo->speedModel()->setValue( static_cast<float>( args.value( "speed" ).toDouble() ) ); }
		if( args.contains( "amount" ) ) { lfo->amountModel()->setValue( static_cast<float>( args.value( "amount" ).toDouble() ) ); }
		if( args.contains( "base" ) ) { lfo->baseModel()->setValue( static_cast<float>( args.value( "base" ).toDouble() ) ); }
		if( args.contains( "phase" ) ) { lfo->phaseModel()->setValue( static_cast<float>( args.value( "phase" ).toDouble() ) ); }
		if( args.contains( "multiplier" ) ) { lfo->multiplierModel()->setValue( args.value( "multiplier" ).toInt() ); }
		result["controller"] = lfo->displayName();
		return successResponse( result );
	}

	if( tool == "connectcontroller" )
	{
		const QString ref = args.value( "controller" ).toString().trimmed();
		const QString address = args.value( "address" ).toString().trimmed();
		if( ref.isEmpty() || address.isEmpty() ) { return errorResponse( "bad_args", tr( "needs controller and address" ) ); }
		Controller* controller = nullptr;
		bool numeric = false;
		const int idx = ref.toInt( &numeric );
		int seen = 0;
		for( Controller* c : song->controllers() )
		{
			if( ( numeric && seen == idx ) || ( !numeric && c->displayName() == ref ) )
			{
				controller = c;
				break;
			}
			++seen;
		}
		if( controller == nullptr ) { return errorResponse( "tool_failed", tr( "controller not found: %1" ).arg( ref ) ); }
		QString canonical;
		QString resolveError;
		AutomatableModel* model = resolveModelAddress( address, canonical, resolveError );
		if( model == nullptr ) { return errorResponse( "bad_address", resolveError ); }
		model->setControllerConnection( new ControllerConnection( controller ) );
		result["controller"] = controller->displayName();
		result["address"] = canonical;
		return successResponse( result );
	}

	if( tool == "disconnectcontroller" )
	{
		const QString address = args.value( "address" ).toString().trimmed();
		if( address.isEmpty() ) { return errorResponse( "bad_args", tr( "needs address" ) ); }
		QString canonical;
		QString resolveError;
		AutomatableModel* model = resolveModelAddress( address, canonical, resolveError );
		if( model == nullptr ) { return errorResponse( "bad_address", resolveError ); }
		model->setControllerConnection( nullptr );
		result["address"] = canonical;
		return successResponse( result );
	}

	if( tool == "describecontrollers" )
	{
		QJsonArray controllers;
		for( Controller* c : song->controllers() )
		{
			QJsonObject o;
			o["name"] = c->displayName();
			if( auto* lfo = dynamic_cast<LfoController*>( c ) )
			{
				o["type"] = "lfo";
				o["wave"] = lfo->waveModel()->value();
				o["speed"] = lfo->speedModel()->value();
				o["amount"] = lfo->amountModel()->value();
				o["base"] = lfo->baseModel()->value();
				o["phase"] = lfo->phaseModel()->value();
				o["multiplier"] = lfo->multiplierModel()->value();
			}
			else if( dynamic_cast<MidiController*>( c ) )
			{
				o["type"] = "midi";
			}
			else
			{
				o["type"] = "other";
			}
			controllers.append( o );
		}
		result["controllers"] = controllers;
		return successResponse( result );
	}

	return std::nullopt;
}

} // namespace lmms
