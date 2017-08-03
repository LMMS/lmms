
#include "MetaData.h"

MetaData::MetaData()
{
	//test
	//set("Structure","IABABBO");
	//set("YouTube","http://www.youtube.com/");
}

bool MetaData::clear()
{
	bool r=(m_data.size()>0);
	if(r) m_data.clear();
	return r;
}

QString MetaData::get(const QString& k)
{
	return m_data.value(k,QString(""));
}

bool MetaData::set(const QString& k,const QString& v)
{
	const QString& old=get(k);
	bool r=(old!=v);
	if(r)
	{
		//qWarning("meta data '%s' has changed: %s",qPrintable(k),qPrintable(v));
		m_data.insert(k,v);
	}
	return r;
}

bool MetaData::load( QDomDocument& doc, QDomElement& head )
{
	bool r=false;
	const QDomNodeList& a=head.elementsByTagName("meta");
	for(int i=0;i<a.length();i++)
	{
		const QDomNode& n=a.at(i);
		if(n.isElement())
		{
			const QDomElement& e=n.toElement();
			if(e.hasAttribute("name")&&
			   e.hasAttribute("value"))
			{
				const QString& k=e.attribute("name");
				const QString& v=e.attribute("value");
				if((k!="")&&(v!=""))
					if(set(k,v)) r=true;
			}
		}

	}
	return r;
}

void MetaData::save( QDomDocument& doc, QDomElement& head )
{
	QMapIterator<QString,QString> i(m_data);
	while (i.hasNext())
	{
		i.next();
		const QString& v=i.value();
		if(v!="")
		{
			const QString& k=i.key();
			QDomElement me = doc.createElement("meta");
			me.setAttribute("value",v);
			me.setAttribute("name",k);
			head.appendChild(me);
		}
	}
}
