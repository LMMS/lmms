#include <QtXml/QDomDocument>
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QProgressDialog>
#include <QTextStream>
#include <stdlib.h>

#include "LocalFileMng.h"
#include "HydrogenImport.h"
#include "song.h"
#include "engine.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "note.h"
#include "Pattern.h"
#include "track.h"
#include "bb_track.h"
#include "bb_track_container.h"
#include "Instrument.h"

#define MAX_LAYERS 4
extern "C"
{

Plugin::Descriptor PLUGIN_EXPORT hydrogenimport_plugin_descriptor =
{
	STRINGIFY( PLUGIN_NAME ),
	"Hydrogen Import",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Filter for importing Hydrogen files into LMMS" ),
	"frank mather",
	0x0100,
	Plugin::ImportFilter,
	NULL,
	NULL,
	NULL
} ;

}

QString filename;
class NoteKey
{
public:
	enum Key {
		C = 0,
		Cs,
		D,
		Ef,
		E,
		F,
		Fs,
		G,
		Af,
		A,
		Bf,
		B,
	};

	static int stringToNoteKey( const QString& str )
	{
		int m_key = NoteKey::C;


		QString sKey = str.left( str.length() - 1 );
		QString sOct = str.mid( str.length() - 1, str.length() );

		if ( sKey.endsWith( "-" ) )
		{
			sKey.replace( "-", "" );
			sOct.insert( 0, "-" );
		}
		int nOctave = sOct.toInt();

		if ( sKey == "C" ) 
		{
			m_key = NoteKey::C;
		} 
		else if ( sKey == "Cs" ) 
		{
			m_key = NoteKey::Cs;
		} 
		else if ( sKey == "D" ) 
		{
			m_key = NoteKey::D;
		}
		else if ( sKey == "Ef" ) 
		{
			m_key = NoteKey::Ef;
		}
		else if ( sKey == "E" ) 
		{
			m_key = NoteKey::E;
		} 
		else if ( sKey == "F" ) 
		{
			m_key = NoteKey::F;
		} 
		else if ( sKey == "Fs" ) 
		{
			m_key = NoteKey::Fs;
		} 
		else if ( sKey == "G" ) 
		{
			m_key = NoteKey::G;
		} 
		else if ( sKey == "Af" ) 
		{
			m_key = NoteKey::Af;
		} 
		else if ( sKey == "A" ) 
		{
			m_key = NoteKey::A;
		} 
		else if ( sKey == "Bf" ) 
		{
			m_key = NoteKey::Bf;
		} 
		else if ( sKey == "B" ) {
			m_key = NoteKey::B;
		} 
        return m_key + (nOctave*12)+57;
	}

};
HydrogenImport::HydrogenImport( const QString & _file ) :
	ImportFilter( _file, &hydrogenimport_plugin_descriptor )
{
	filename = _file;
}




HydrogenImport::~HydrogenImport()
{
}
Instrument * ins;
bool HydrogenImport::readSong() 
{
	QHash<QString, InstrumentTrack *> drum_track;
	QHash<QString, int> pattern_length;
	QHash<QString, int> pattern_id;

	song *s = engine::getSong();
	int song_num_tracks = s->tracks().size();
	if ( QFile( filename ).exists() == false ) 
	{
		printf( "Song file not found \n" );
		return false;
	}
	QDomDocument doc = LocalFileMng::openXmlDocument( filename );
	QDomNodeList nodeList = doc.elementsByTagName( "song" );

	if( nodeList.isEmpty() )
	{
		printf( "Error reading song: song node not found\n" );
		return false;
	}
	QDomNode songNode = nodeList.at( 0 );

	QString m_sSongVersion = LocalFileMng::readXmlString( songNode , "version", "Unknown version" );


	QString sName( LocalFileMng::readXmlString( songNode, "name", "Untitled Song" ) );
	QString sAuthor( LocalFileMng::readXmlString( songNode, "author", "Unknown Author" ) );
	QString sNotes( LocalFileMng::readXmlString( songNode, "notes", "..." ) );
	QString sLicense( LocalFileMng::readXmlString( songNode, "license", "Unknown license" ) );
	QString sMode = LocalFileMng::readXmlString( songNode, "mode", "pattern" );

	QDomNode instrumentListNode = songNode.firstChildElement( "instrumentList" );
	if ( ( ! instrumentListNode.isNull()  ) ) 
	{

		int instrumentList_count = 0;
		QDomNode instrumentNode;
		instrumentNode = instrumentListNode.firstChildElement( "instrument" );
		while ( ! instrumentNode.isNull()  ) 
		{
			instrumentList_count++;
			QString sId = LocalFileMng::readXmlString( instrumentNode, "id", "" );			// instrument id
			QString sDrumkit = LocalFileMng::readXmlString( instrumentNode, "drumkit", "" );	// drumkit
			QString sName = LocalFileMng::readXmlString( instrumentNode, "name", "" );		// name
			float fVolume = LocalFileMng::readXmlFloat( instrumentNode, "volume", 1.0 );	// volume
			float fPan_L = LocalFileMng::readXmlFloat( instrumentNode, "pan_L", 0.5 );	// pan L
			float fPan_R = LocalFileMng::readXmlFloat( instrumentNode, "pan_R", 0.5 );	// pan R

			if ( sId.isEmpty() ) {
				printf( "Empty ID for instrument. skipping \n" );
				instrumentNode = (QDomNode) instrumentNode.nextSiblingElement( "instrument" );
				continue;
			}
			QDomNode filenameNode = instrumentNode.firstChildElement( "filename" );

			if ( ! filenameNode.isNull() ) 
			{
				return false;
			} 
			else 
			{
				unsigned nLayer = 0;
				QDomNode layerNode = instrumentNode.firstChildElement( "layer" );
				while (  ! layerNode.isNull()  ) 
				{
					if ( nLayer >= MAX_LAYERS ) 
					{
						printf( "nLayer >= MAX_LAYERS" );
						continue;
					}
					QString sFilename = LocalFileMng::readXmlString( layerNode, "filename", "" );
					QString sMode = LocalFileMng::readXmlString( layerNode, "smode", "forward" );

					if ( nLayer == 0 ) 
					{
						drum_track[sId] = ( InstrumentTrack * ) track::create( track::InstrumentTrack,engine::getBBTrackContainer() );
						drum_track[sId]->volumeModel()->setValue( fVolume * 100 );
						drum_track[sId]->panningModel()->setValue( ( fPan_R - fPan_L ) * 100 );
						ins = drum_track[sId]->loadInstrument( "audiofileprocessor" );
						ins->loadFile( sFilename );
					}
					nLayer++;
					layerNode = ( QDomNode ) layerNode.nextSiblingElement( "layer" );
				}
			}

			instrumentNode = (QDomNode) instrumentNode.nextSiblingElement( "instrument" );
		}
		if ( instrumentList_count == 0 ) 
		{
			return false;
		}
	} 
	else 
	{
		return false;
	}
	QDomNode patterns = songNode.firstChildElement( "patternList" );
	int pattern_count = 0;
	int nbb = engine::getBBTrackContainer()->numOfBBs();
	QDomNode patternNode =  patterns.firstChildElement( "pattern" );
	int pn = 1;
	while (  !patternNode.isNull()  ) 
	{
		if ( pn > 0 ) 
		{
			pattern_count++;
			s->addBBTrack();
			pn = 0;
		}
		QString sName;	// name
		sName = LocalFileMng::readXmlString( patternNode, "name", sName );

		QString sCategory = ""; // category
		sCategory = LocalFileMng::readXmlString( patternNode, "category", sCategory ,false ,false );
		int nSize = -1;
		nSize = LocalFileMng::readXmlInt( patternNode, "size", nSize, false, false );
		pattern_length[sName] = nSize;
		QDomNode pNoteListNode = patternNode.firstChildElement( "noteList" );
		if ( ! pNoteListNode.isNull() ) {
			QDomNode noteNode = pNoteListNode.firstChildElement( "note" );
			while ( ! noteNode.isNull()  ) {
				int nPosition = LocalFileMng::readXmlInt( noteNode, "position", 0 );
				float fVelocity = LocalFileMng::readXmlFloat( noteNode, "velocity", 0.8f );	
				float fPan_L = LocalFileMng::readXmlFloat( noteNode, "pan_L", 0.5 );
				float fPan_R = LocalFileMng::readXmlFloat( noteNode, "pan_R", 0.5 );	
				QString sKey = LocalFileMng::readXmlString( noteNode, "key", "C0", false, false );
				QString nNoteOff = LocalFileMng::readXmlString( noteNode, "note_off", "false", false, false );

				QString instrId = LocalFileMng::readXmlString( noteNode, "instrument", 0,false, false );
				int i = pattern_count - 1 + nbb;
				pattern_id[sName] = pattern_count - 1;
				Pattern*p = dynamic_cast<Pattern*>( drum_track[instrId]->getTCO( i ) );
				note n; 
				n.setPos( nPosition );
				if ( (nPosition + 48) <= nSize ) 
				{
  					n.setLength( 48 );
				} 
				else 
				{
					n.setLength( nSize - nPosition );
				} 
				n.setVolume( fVelocity * 100 );
				n.setPanning( ( fPan_R - fPan_L ) * 100 );
				n.setKey( NoteKey::stringToNoteKey( sKey ) );
				p->addNote( n,false );
				pn = pn + 1;
				noteNode = ( QDomNode ) noteNode.nextSiblingElement( "note" );
			}        
		}
		patternNode = ( QDomNode ) patternNode.nextSiblingElement( "pattern" );
	}
	// Pattern sequence
	QDomNode patternSequenceNode = songNode.firstChildElement( "patternSequence" );
	QDomNode groupNode = patternSequenceNode.firstChildElement( "group" );
	int pos = 0;
	while (  !groupNode.isNull()  ) 
	{
	    int best_length = 0;
		QDomNode patternId = groupNode.firstChildElement( "patternID" );
		while (  !patternId.isNull()  ) 
		{
			QString patId = patternId.firstChild().nodeValue();
			patternId = ( QDomNode ) patternId.nextSiblingElement( "patternID" );

			int i = pattern_id[patId]+song_num_tracks;
			track *t = ( bbTrack * ) s->tracks().at( i );
 			trackContentObject *tco = t->createTCO( pos );      
			tco->movePosition( pos );

			
			if ( pattern_length[patId] > best_length ) 
			{
				best_length = pattern_length[patId];
			}
		}
		pos = pos + best_length;
		groupNode = groupNode.nextSiblingElement( "group" );
	}

	if ( pattern_count == 0 ) 
	{
		return false;
	}
	return true;
}
bool HydrogenImport::tryImport( TrackContainer* tc )
{
	if( openFile() == false )
	{
		return false;
	}
	return readSong();
}



extern "C"
{

// necessary for getting instance out of shared lib
Plugin * PLUGIN_EXPORT lmms_plugin_main( Model *, void * _data )
{
	return new HydrogenImport( QString::fromUtf8(
									static_cast<const char *>( _data ) ) );
}


}

