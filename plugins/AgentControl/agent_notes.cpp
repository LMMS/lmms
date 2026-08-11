/*
 * agent_notes.cpp - piano-roll note domain tools (v2)
 */

#include "AgentControl.h"

#include <QRandomGenerator>
#include <QJsonArray>

#include "InstrumentFunctions.h"
#include "InstrumentTrack.h"
#include "MidiClip.h"
#include "Note.h"

namespace lmms
{

namespace
{

const InstrumentFunctionNoteStacking::Chord* resolveChord( const QString& name )
{
	const auto& table = InstrumentFunctionNoteStacking::ChordTable::getInstance();
	const InstrumentFunctionNoteStacking::Chord* chord = nullptr;
	if( !table.getChordByName( name ).isEmpty() )
	{
		chord = &table.getChordByName( name );
	}
	return chord;
}

} // namespace

std::optional<QJsonObject> AgentControlService::dispatchNoteTool(
	const QString& tool, const QJsonObject& args )
{
	QJsonObject result;
	QString error;

	MidiClip* clip = nullptr;
	if( tool == "editnotes" || tool == "removenotes" || tool == "clearclip"
		|| tool == "quantizeclip" || tool == "humanizeclip" || tool == "reverseclip"
		|| tool == "splitclipnotes" || tool == "setclipvelocityscale"
		|| tool == "addchord" || tool == "addarpeggio" )
	{
		clip = resolveMidiClip( args, error );
		if( clip == nullptr ) { return errorResponse( "tool_failed", error ); }
	}

	if( tool == "editnotes" )
	{
		const QJsonArray notes = args.value( "notes" ).toArray();
		if( notes.isEmpty() ) { return errorResponse( "bad_args", tr( "edit_notes needs notes" ) ); }
		int edited = 0;
		const auto& existing = clip->notes();
		for( const auto& entry : notes )
		{
			const QJsonObject n = entry.toObject();
			const int key = n.value( "key" ).toInt( -1 );
			const int pos = n.value( "pos" ).toInt( -1 );
			if( key < 0 || pos < 0 ) { continue; }
			for( Note* note : existing )
			{
				if( note->key() == key && note->pos().getTicks() == pos )
				{
					if( n.contains( "length" ) ) { note->setLength( TimePos( qMax( 1, n.value( "length" ).toInt() ) ) ); }
					if( n.contains( "velocity" ) ) { note->setVolume( static_cast<volume_t>( qBound( 1, n.value( "velocity" ).toInt(), 127 ) ) * DefaultVolume / 127 ); }
					if( n.contains( "pan" ) ) { note->setPanning( static_cast<panning_t>( qBound( 0, n.value( "pan" ).toInt(), 100 ) ) * DefaultPanning / 100 ); }
					++edited;
					break;
				}
			}
		}
		clip->updateLength();
		result["edited"] = edited;
		result["track"] = clip->instrumentTrack()->name();
		result["clip_index"] = clip->instrumentTrack()->getClipNum( clip );
		return successResponse( result );
	}

	if( tool == "removenotes" )
	{
		const QJsonArray keys = args.value( "keys" ).toArray();
		if( keys.isEmpty() ) { return errorResponse( "bad_args", tr( "remove_notes needs keys" ) ); }
		QSet<int> wanted;
		for( const auto& k : keys ) { wanted.insert( k.toInt( -1 ) ); }
		int removed = 0;
		const auto notes = clip->notes();
		for( Note* note : notes )
		{
			if( wanted.contains( note->key() ) )
			{
				clip->removeNote( note );
				++removed;
			}
		}
		result["removed"] = removed;
		return successResponse( result );
	}

	if( tool == "clearclip" )
	{
		clip->clear();
		result["message"] = tr( "clip cleared" );
		return successResponse( result );
	}

	if( tool == "quantizeclip" )
	{
		const int resolution = args.value( "resolution" ).toInt( 0 );
		if( resolution <= 0 ) { return errorResponse( "bad_args", tr( "quantize_clip needs resolution 1..192" ) ); }
		const int ticksPerNote = qMax( 1, TimePos::ticksPerBar() / resolution );
		int quantized = 0;
		for( Note* note : clip->notes() )
		{
			note->quantizePos( ticksPerNote );
			++quantized;
		}
		clip->updateLength();
		result["quantized"] = quantized;
		result["resolution"] = resolution;
		return successResponse( result );
	}

	if( tool == "humanizeclip" )
	{
		const double amount = args.value( "amount" ).toDouble( 0.1 );
		if( amount < 0.0 || amount > 1.0 ) { return errorResponse( "bad_args", tr( "amount must be in 0..1" ) ); }
		QRandomGenerator rng( 0x5EED );
		const int maxJitter = qMax( 1, static_cast<int>( TimePos::ticksPerBar() / 32.0 * amount ) );
		int humanized = 0;
		for( Note* note : clip->notes() )
		{
			const int jitter = static_cast<int>( rng.bounded( maxJitter * 2 + 1 ) ) - maxJitter;
			note->setPos( TimePos( qMax( 0, note->pos().getTicks() + jitter ) ) );
			const int v = qBound( 1, static_cast<int>( note->getVolume() / DefaultVolume * 127.0 )
				+ static_cast<int>( rng.bounded( static_cast<quint32>( 30 * amount + 1 ) ) ) - static_cast<int>( 15 * amount ), 127 );
			note->setVolume( static_cast<volume_t>( v ) * DefaultVolume / 127 );
			++humanized;
		}
		clip->updateLength();
		result["humanized"] = humanized;
		return successResponse( result );
	}

	if( tool == "reverseclip" )
	{
		clip->reverseNotes( clip->notes() );
		result["message"] = tr( "notes reversed" );
		return successResponse( result );
	}

	if( tool == "splitclipnotes" )
	{
		bool ok = false;
		const TimePos at = timePosFromArgs( args, ok );
		if( !ok ) { return errorResponse( "bad_args", tr( "split_clip_notes needs tick or bar" ) ); }
		const int splitTick = at.getTicks();
		// Notes after the split move to the clip start; notes spanning the
		// split are shortened; notes before it keep their position.
		for( Note* note : clip->notes() )
		{
			const int pos = note->pos().getTicks();
			const int len = note->length().getTicks();
			if( pos >= splitTick )
			{
				note->setPos( TimePos( qMax( 0, pos - splitTick ) ) );
			}
			else if( pos + len > splitTick )
			{
				note->setLength( TimePos( qMax( 1, splitTick - pos ) ) );
			}
		}
		clip->rearrangeAllNotes();
		clip->updateLength();
		result["at_tick"] = splitTick;
		return successResponse( result );
	}

	if( tool == "setclipvelocityscale" )
	{
		const double scale = args.value( "scale" ).toDouble( -1.0 );
		if( scale < 0.0 || scale > 2.0 ) { return errorResponse( "bad_args", tr( "scale must be in 0..2" ) ); }
		int changed = 0;
		for( Note* note : clip->notes() )
		{
			const int v = qBound( 1, static_cast<int>( note->getVolume() / DefaultVolume * 127.0 * scale ), 127 );
			note->setVolume( static_cast<volume_t>( v ) * DefaultVolume / 127 );
			++changed;
		}
		result["changed"] = changed;
		return successResponse( result );
	}

	if( tool == "addchord" )
	{
		const int root = args.value( "root" ).toInt( -1 );
		const QString chordName = args.value( "chord" ).toString().trimmed();
		if( root < 0 || root > 127 || chordName.isEmpty() )
		{
			return errorResponse( "bad_args", tr( "add_chord needs root (0..127) and chord" ) );
		}
		const auto* chord = resolveChord( chordName );
		if( chord == nullptr )
		{
			return errorResponse( "bad_args", tr( "unknown chord: %1" ).arg( chordName ) );
		}
		bool ok = false;
		TimePos pos = timePosFromArgs( args, ok );
		if( !ok ) { pos = TimePos( 0 ); }
		const int length = args.value( "length" ).toInt( TimePos::ticksPerBar() / 4 );
		const int velocity = args.value( "velocity" ).toInt( 100 );
		const volume_t vol = static_cast<volume_t>( qBound( 1, velocity, 127 ) ) * DefaultVolume / 127;
		int added = 0;
		for( int i = 0; i < chord->size(); ++i )
		{
			const int key = qBound( 0, root + ( *chord )[i], 127 );
			clip->addNote( Note( TimePos( qMax( 1, length ) ), pos, key, vol ), false );
			++added;
		}
		clip->updateLength();
		result["added"] = added;
		result["chord"] = chordName;
		return successResponse( result );
	}

	if( tool == "addarpeggio" )
	{
		const int root = args.value( "root" ).toInt( -1 );
		const QString chordName = args.value( "chord" ).toString().trimmed();
		const QString direction = args.value( "direction" ).toString( "up" );
		const int steps = qBound( 1, args.value( "steps" ).toInt( 8 ), 128 );
		if( root < 0 || root > 127 || chordName.isEmpty() )
		{
			return errorResponse( "bad_args", tr( "add_arpeggio needs root and chord" ) );
		}
		const auto* chord = resolveChord( chordName );
		if( chord == nullptr )
		{
			return errorResponse( "bad_args", tr( "unknown chord: %1" ).arg( chordName ) );
		}
		bool ok = false;
		TimePos pos = timePosFromArgs( args, ok );
		if( !ok ) { pos = TimePos( 0 ); }
		const int stepLen = qMax( 1, args.value( "step_len" ).toInt( TimePos::ticksPerBar() / 16 ) );
		const int velocity = args.value( "velocity" ).toInt( 100 );
		const int octaves = qBound( 1, args.value( "octaves" ).toInt( 1 ), 4 );
		const volume_t vol = static_cast<volume_t>( qBound( 1, velocity, 127 ) ) * DefaultVolume / 127;

		QVector<int> sequence;
		for( int oct = 0; oct < octaves; ++oct )
		{
			for( int i = 0; i < chord->size(); ++i )
			{
				sequence.append( root + ( *chord )[i] + 12 * oct );
			}
		}
		QVector<int> order;
		if( direction == "down" )
		{
			for( int i = sequence.size() - 1; i >= 0; --i ) { order.append( sequence[i] ); }
		}
		else if( direction == "updown" )
		{
			order = sequence;
			for( int i = sequence.size() - 2; i > 0; --i ) { order.append( sequence[i] ); }
		}
		else if( direction == "random" )
		{
			QRandomGenerator rng( 0xA2E0 );
			order = sequence;
			for( int i = order.size() - 1; i > 0; --i )
			{
				order.swapItemsAt( i, static_cast<int>( rng.bounded( static_cast<quint32>( i + 1 ) ) ) );
			}
		}
		else { order = sequence; }

		int added = 0;
		for( int s = 0; s < steps; ++s )
		{
			const int key = qBound( 0, order[s % order.size()], 127 );
			clip->addNote( Note( TimePos( stepLen ), TimePos( pos.getTicks() + s * stepLen ), key, vol ), false );
			++added;
		}
		clip->updateLength();
		result["added"] = added;
		result["chord"] = chordName;
		result["direction"] = direction;
		return successResponse( result );
	}

	return std::nullopt;
}

} // namespace lmms
