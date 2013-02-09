#include <cstdlib>
#include <cassert>
#include <sys/stat.h>
#include <ctype.h>

#include <QDir>
#include <QApplication>
#include <QVector>
#include <QDomDocument>
#include <QLocale>
#include <QTextCodec>

#include <algorithm>
#include "LocalFileMng.h"


/* New QtXml based methods */

QString LocalFileMng::readXmlString( QDomNode node , const QString& nodeName, const QString& defaultValue, bool bCanBeEmpty, bool bShouldExists, bool tinyXmlCompatMode)
{
 	QDomElement element = node.firstChildElement( nodeName );
	
	if( !node.isNull() && !element.isNull() ){
		if(  !element.text().isEmpty() ){
			return element.text();
		} else {
			if ( !bCanBeEmpty ) {
				//_WARNINGLOG( "Using default value in " + nodeName );
			}
			return defaultValue;
		}
	} else {	
		if(  bShouldExists ){
			//_WARNINGLOG( "'" + nodeName + "' node not found" );
			
		}
		return defaultValue;
	}
}

float LocalFileMng::readXmlFloat( QDomNode node , const QString& nodeName, float defaultValue, bool bCanBeEmpty, bool bShouldExists, bool tinyXmlCompatMode)
{
	QLocale c_locale = QLocale::c();
 	QDomElement element = node.firstChildElement( nodeName );
	
	if( !node.isNull() && !element.isNull() ){
		if(  !element.text().isEmpty() ){
			return c_locale.toFloat(element.text());
		} else {
			if ( !bCanBeEmpty ) {
				//_WARNINGLOG( "Using default value in " + nodeName );
			}
			return defaultValue;
		}
	} else {	
		if(  bShouldExists ){
			//_WARNINGLOG( "'" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}

int LocalFileMng::readXmlInt( QDomNode node , const QString& nodeName, int defaultValue, bool bCanBeEmpty, bool bShouldExists, bool tinyXmlCompatMode)
{
	QLocale c_locale = QLocale::c();
 	QDomElement element = node.firstChildElement( nodeName );
	
	if( !node.isNull() && !element.isNull() ){
		if(  !element.text().isEmpty() ){
			return c_locale.toInt( element.text() );
		} else {
			if ( !bCanBeEmpty ) {
				//_WARNINGLOG( "Using default value in " + nodeName );
			}
			return defaultValue;
		}
	} else {	
		if(  bShouldExists ){
			//_WARNINGLOG( "'" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}

bool LocalFileMng::readXmlBool( QDomNode node , const QString& nodeName, bool defaultValue, bool bShouldExists, bool tinyXmlCompatMode)
{
 	QDomElement element = node.firstChildElement( nodeName );
	
	if( !node.isNull() && !element.isNull() ){
		if(  !element.text().isEmpty() ){
			if( element.text() == "true"){
				return true;
			} else {
				return false;
			}
		} else {
			//_WARNINGLOG( "Using default value in " + nodeName );
			return defaultValue;
		}
	} else {	
		if(  bShouldExists ){
			//_WARNINGLOG( "'" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}


/* Convert (in-place) an XML escape sequence into a literal byte,
 * rather than the character it actually refers to.
 */
void LocalFileMng::convertFromTinyXMLString( QByteArray* str )
{
	/* When TinyXML encountered a non-ASCII character, it would
	 * simply write the character as "&#xx;" -- where "xx" is
	 * the hex character code.  However, this doesn't respect
	 * any encodings (e.g. UTF-8, UTF-16).  In XML, &#xx; literally
	 * means "the Unicode character # xx."  However, in a UTF-8
	 * sequence, this could be an escape character that tells
	 * whether we have a 2, 3, or 4-byte UTF-8 sequence.
	 *
	 * For example, the UTF-8 sequence 0xD184 was being written
	 * by TinyXML as "&#xD1;&#x84;".  However, this is the UTF-8
	 * sequence for the cyrillic small letter EF (which looks
	 * kind of like a thorn or a greek phi).  This letter, in
	 * XML, should be saved as &#x00000444;, or even literally
	 * (no escaping).  As a consequence, when &#xD1; is read
	 * by an XML parser, it will be interpreted as capital N
	 * with a tilde (~).  Then &#x84; will be interpreted as
	 * an unknown or control character.
	 *
	 * So, when we know that TinyXML wrote the file, we can
	 * simply exchange these hex sequences to literal bytes.
	 */
	int pos = 0;

	pos = str->indexOf("&#x");
	while( pos != -1 ) {
		if( isxdigit(str->at(pos+3))
		    && isxdigit(str->at(pos+4))
		    && (str->at(pos+5) == ';') ) {
			char w1 = str->at(pos+3);
			char w2 = str->at(pos+4);

			w1 = tolower(w1) - 0x30;  // '0' = 0x30
			if( w1 > 9 ) w1 -= 0x27;  // '9' = 0x39, 'a' = 0x61
			w1 = (w1 & 0xF);

			w2 = tolower(w2) - 0x30;  // '0' = 0x30
			if( w2 > 9 ) w2 -= 0x27;  // '9' = 0x39, 'a' = 0x61
			w2 = (w2 & 0xF);

			char ch = (w1 << 4) | w2;
			(*str)[pos] = ch;
			++pos;
			str->remove(pos, 5);
		}
		pos = str->indexOf("&#x");
	}
}

bool LocalFileMng::checkTinyXMLCompatMode( const QString& filename )
{
	/*
		Check if filename was created with TinyXml or QtXml
		TinyXML: return true
		QtXml: return false
	*/

	QFile file( filename );

	if ( !file.open(QIODevice::ReadOnly) )
		return false;

	QString line = file.readLine();
	file.close();
	if ( line.startsWith( "<?xml" )){
		return false;
	} else  {
		//_WARNINGLOG( QString("File '%1' is being read in "
		//		    "TinyXML compatability mode")
		//	    .arg(filename) );
		return true;
	}



}


QDomDocument LocalFileMng::openXmlDocument( const QString& filename )
{
	bool TinyXMLCompat = LocalFileMng::checkTinyXMLCompatMode( filename );

	QDomDocument doc;
	QFile file( filename );

	if ( !file.open(QIODevice::ReadOnly) )
		return QDomDocument();

	if( TinyXMLCompat ) {
	    QString enc = QTextCodec::codecForLocale()->name();
	    if( enc == QString("System") ) {
		    enc = "UTF-8";
	    }
	    QByteArray line;
	    QByteArray buf = QString("<?xml version='1.0' encoding='%1' ?>\n")
		.arg( enc )
		.toLocal8Bit();

	    //_INFOLOG( QString("Using '%1' encoding for TinyXML file").arg(enc) );

	    while( !file.atEnd() ) {
			line = file.readLine();
			LocalFileMng::convertFromTinyXMLString( &line );
			buf += line;
	    }

	    if( ! doc.setContent( buf ) ) {
			file.close();
			return QDomDocument();
	    }

	} else {
	    if( ! doc.setContent( &file ) ) {
			file.close();
			return QDomDocument();
	    }
	}
	file.close();
	
	return doc;
}

