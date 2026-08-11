/*
 * agent_mixer.cpp - mixer domain tools (v2): channels, sends, routing, peaks.
 */

#include "AgentControl.h"

#include <QJsonArray>

#include "Effect.h"
#include "EffectChain.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "SampleTrack.h"
#include "Song.h"
#include "Track.h"

namespace lmms
{

std::optional<QJsonObject> AgentControlService::dispatchMixerTool(
	const QString& tool, const QJsonObject& args )
{
	Mixer* mixer = Engine::mixer();
	if( mixer == nullptr ) { return errorResponse( "tool_failed", tr( "mixer not available" ) ); }
	QJsonObject result;
	QString error;

	if( tool == "listmixerchannels" )
	{
		QJsonArray channels;
		for( mix_ch_t i = 0; i < mixer->numChannels(); ++i )
		{
			MixerChannel* ch = mixer->mixerChannel( i );
			QJsonObject o;
			o["index"] = i;
			o["name"] = ch->m_name;
			o["master"] = ch->isMaster();
			o["volume"] = ch->m_volumeModel.value();
			o["mute"] = ch->m_muteModel.value();
			o["solo"] = ch->m_soloModel.value();
			QJsonArray sends;
			for( MixerRoute* route : ch->m_sends )
			{
				QJsonObject send;
				send["to"] = route->receiver()->m_name;
				send["amount"] = route->amount()->value();
				sends.append( send );
			}
			o["sends"] = sends;
			QJsonArray effects;
			for( Effect* fx : ch->m_fxChain.effects() )
			{
				if( fx != nullptr ) { effects.append( fx->displayName() ); }
			}
			o["effects"] = effects;
			channels.append( o );
		}
		result["channels"] = channels;
		return successResponse( result );
	}

	if( tool == "createchannel" )
	{
		mix_ch_t index = mixer->createChannel();
		const QString name = args.value( "name" ).toString().trimmed();
		if( !name.isEmpty() )
		{
			mixer->mixerChannel( index )->m_name = name;
		}
		result["index"] = index;
		result["name"] = mixer->mixerChannel( index )->m_name;
		return successResponse( result );
	}

	MixerChannel* channel = nullptr;
	if( tool == "deletechannel" || tool == "renamechannel" || tool == "movechannel"
		|| tool == "setchannelvolume" || tool == "setchannelmute"
		|| tool == "setchannelsolo" || tool == "addchanneleffect"
		|| tool == "removechanneleffect" || tool == "createsend"
		|| tool == "deletesend" || tool == "setsendamount"
		|| tool == "routetracktochannel" || tool == "getpeaklevels" )
	{
		if( tool != "createsend" && tool != "deletesend" && tool != "setsendamount"
			&& tool != "routetracktochannel" && tool != "getpeaklevels" )
		{
			const QString ref = args.value( "channel" ).toString();
			if( ref.isEmpty() ) { return errorResponse( "bad_args", tr( "needs channel" ) ); }
			channel = resolveMixerChannelRef( ref, error );
			if( channel == nullptr ) { return errorResponse( "tool_failed", error ); }
		}
	}

	if( tool == "deletechannel" )
	{
		const int index = channel->index();
		mixer->deleteChannel( index );
		result["index"] = index;
		return successResponse( result );
	}
	if( tool == "renamechannel" )
	{
		const QString name = args.value( "name" ).toString().trimmed();
		if( name.isEmpty() ) { return errorResponse( "bad_args", tr( "needs name" ) ); }
		channel->m_name = name;
		result["index"] = channel->index();
		result["name"] = name;
		return successResponse( result );
	}
	if( tool == "movechannel" )
	{
		const QString direction = args.value( "direction" ).toString().trimmed();
		if( direction == "left" ) { mixer->moveChannelLeft( channel->index() ); }
		else if( direction == "right" ) { mixer->moveChannelRight( channel->index() ); }
		else { return errorResponse( "bad_args", tr( "direction must be left|right" ) ); }
		result["index"] = channel->index();
		result["direction"] = direction;
		return successResponse( result );
	}
	if( tool == "setchannelvolume" )
	{
		const double value = args.value( "value" ).toDouble( -1.0 );
		if( value < 0.0 || value > 1.0 ) { return errorResponse( "bad_args", tr( "volume must be in 0..1" ) ); }
		channel->m_volumeModel.setValue( static_cast<float>( value ) );
		result["index"] = channel->index();
		result["value"] = channel->m_volumeModel.value();
		return successResponse( result );
	}
	if( tool == "setchannelmute" )
	{
		if( !args.contains( "mute" ) ) { return errorResponse( "bad_args", tr( "needs mute" ) ); }
		channel->m_muteModel.setValue( args.value( "mute" ).toBool() );
		result["index"] = channel->index();
		result["mute"] = channel->m_muteModel.value();
		return successResponse( result );
	}
	if( tool == "setchannelsolo" )
	{
		if( !args.contains( "solo" ) ) { return errorResponse( "bad_args", tr( "needs solo" ) ); }
		channel->m_soloModel.setValue( args.value( "solo" ).toBool() );
		result["index"] = channel->index();
		result["solo"] = channel->m_soloModel.value();
		return successResponse( result );
	}
	if( tool == "addchanneleffect" )
	{
		const QString effectName = args.value( "effect" ).toString().trimmed();
		if( effectName.isEmpty() ) { return errorResponse( "bad_args", tr( "needs effect" ) ); }
		QString displayName;
		const QString resolved = resolveEffectPlugin( effectName, displayName );
		if( resolved.isEmpty() ) { return errorResponse( "tool_failed", tr( "unknown effect: %1" ).arg( effectName ) ); }
		if( auto* effect = Effect::instantiate( resolved, &channel->m_fxChain, nullptr ) )
		{
			channel->m_fxChain.appendEffect( effect );
		}
		result["index"] = channel->index();
		result["effect"] = displayName;
		return successResponse( result );
	}
	if( tool == "removechanneleffect" )
	{
		const QString effectName = args.value( "effect" ).toString().trimmed();
		if( effectName.isEmpty() ) { return errorResponse( "bad_args", tr( "needs effect" ) ); }
		Effect* fx = findEffectInChain( &channel->m_fxChain, effectName );
		if( fx == nullptr ) { return errorResponse( "tool_failed", tr( "effect not found: %1" ).arg( effectName ) ); }
		channel->m_fxChain.removeEffect( fx );
		result["index"] = channel->index();
		result["effect"] = effectName;
		return successResponse( result );
	}
	if( tool == "createsend" )
	{
		const QString fromRef = args.value( "from" ).toString();
		const QString toRef = args.value( "to" ).toString();
		if( fromRef.isEmpty() || toRef.isEmpty() ) { return errorResponse( "bad_args", tr( "needs from and to" ) ); }
		MixerChannel* from = resolveMixerChannelRef( fromRef, error );
		if( from == nullptr ) { return errorResponse( "tool_failed", error ); }
		MixerChannel* to = resolveMixerChannelRef( toRef, error );
		if( to == nullptr ) { return errorResponse( "tool_failed", error ); }
		const float amount = static_cast<float>( args.value( "amount" ).toDouble( 1.0 ) );
		mixer->createChannelSend( from->index(), to->index(), amount );
		result["from"] = from->m_name;
		result["to"] = to->m_name;
		result["amount"] = amount;
		return successResponse( result );
	}
	if( tool == "deletesend" )
	{
		const QString fromRef = args.value( "from" ).toString();
		const QString toRef = args.value( "to" ).toString();
		if( fromRef.isEmpty() || toRef.isEmpty() ) { return errorResponse( "bad_args", tr( "needs from and to" ) ); }
		MixerChannel* from = resolveMixerChannelRef( fromRef, error );
		if( from == nullptr ) { return errorResponse( "tool_failed", error ); }
		MixerChannel* to = resolveMixerChannelRef( toRef, error );
		if( to == nullptr ) { return errorResponse( "tool_failed", error ); }
		MixerRoute* route = nullptr;
		for( MixerRoute* r : from->m_sends )
		{
			if( r->receiver() == to ) { route = r; break; }
		}
		if( route == nullptr ) { return errorResponse( "tool_failed", tr( "no send from %1 to %2" ).arg( from->m_name, to->m_name ) ); }
		mixer->deleteChannelSend( from->index(), to->index() );
		result["from"] = from->m_name;
		result["to"] = to->m_name;
		return successResponse( result );
	}
	if( tool == "setsendamount" )
	{
		const QString fromRef = args.value( "from" ).toString();
		const QString toRef = args.value( "to" ).toString();
		const double amount = args.value( "amount" ).toDouble( -1.0 );
		if( fromRef.isEmpty() || toRef.isEmpty() || amount < 0.0 || amount > 1.0 )
		{
			return errorResponse( "bad_args", tr( "needs from, to, and amount 0..1" ) );
		}
		MixerChannel* from = resolveMixerChannelRef( fromRef, error );
		if( from == nullptr ) { return errorResponse( "tool_failed", error ); }
		MixerChannel* to = resolveMixerChannelRef( toRef, error );
		if( to == nullptr ) { return errorResponse( "tool_failed", error ); }
		FloatModel* sendModel = mixer->channelSendModel( from->index(), to->index() );
		if( sendModel == nullptr ) { return errorResponse( "tool_failed", tr( "no send exists" ) ); }
		sendModel->setValue( static_cast<float>( amount ) );
		result["from"] = from->m_name;
		result["to"] = to->m_name;
		result["amount"] = sendModel->value();
		return successResponse( result );
	}
	if( tool == "routetracktochannel" )
	{
		const QString trackName = args.value( "track" ).toString().trimmed();
		const QString channelRef = args.value( "channel" ).toString();
		if( trackName.isEmpty() || channelRef.isEmpty() ) { return errorResponse( "bad_args", tr( "needs track and channel" ) ); }
		QJsonObject trackArgs{ { "track", trackName } };
		Track* track = resolveTrackRef( trackArgs );
		if( track == nullptr ) { return errorResponse( "tool_failed", tr( "track not found: %1" ).arg( trackName ) ); }
		MixerChannel* target = resolveMixerChannelRef( channelRef, error );
		if( target == nullptr ) { return errorResponse( "tool_failed", error ); }
		if( auto* it = dynamic_cast<InstrumentTrack*>( track ) )
		{
			it->mixerChannelModel()->setValue( target->index() );
		}
		else if( auto* st = dynamic_cast<SampleTrack*>( track ) )
		{
			st->mixerChannelModel()->setValue( target->index() );
		}
		else
		{
			return errorResponse( "tool_failed", tr( "track type cannot be routed" ) );
		}
		result["track"] = track->name();
		result["channel"] = target->m_name;
		return successResponse( result );
	}
	if( tool == "getpeaklevels" )
	{
		QJsonArray peaks;
		if( args.contains( "channel" ) )
		{
			MixerChannel* ch = resolveMixerChannelRef( args.value( "channel" ).toString(), error );
			if( ch == nullptr ) { return errorResponse( "tool_failed", error ); }
			QJsonObject o;
			o["channel"] = ch->m_name;
			o["left"] = ch->m_peakLeft;
			o["right"] = ch->m_peakRight;
			peaks.append( o );
		}
		else
		{
			for( mix_ch_t i = 0; i < mixer->numChannels(); ++i )
			{
				MixerChannel* ch = mixer->mixerChannel( i );
				QJsonObject o;
				o["channel"] = ch->m_name;
				o["left"] = ch->m_peakLeft;
				o["right"] = ch->m_peakRight;
				peaks.append( o );
			}
		}
		result["peaks"] = peaks;
		return successResponse( result );
	}

	return std::nullopt;
}

} // namespace lmms
