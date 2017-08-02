
#include "MetaData.h"

MetaData::MetaData()
{
	//test
	put("Structure","IABABBO");
	put("YouTube","http://www.youtube.com/");
}

QString MetaData::get(const QString& k)
{
	return m_data.value(k,QString(""));
}

void MetaData::put(const QString& k,const QString& v)
{
	m_data.insert(k,v);
}

void MetaData::load( QDomDocument& doc, QDomElement& head )
{

}

void MetaData::save( QDomDocument& doc, QDomElement& head )
{
	QMapIterator<QString,QString> i(m_data);
	while (i.hasNext())
	{
		i.next();
		QDomElement me = doc.createElement("meta");
		me.setAttribute("value",i.value());
		me.setAttribute("name",i.key());
		head.appendChild(me);
	}
}
