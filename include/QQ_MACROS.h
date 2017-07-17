/*
 * QQ_MACROS.h - macros to avoid boiler plate code
 *
 * Copyright (c) 2017 gi0e5b06 (on github.com)
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef QQ_MACROS_H
#define QQ_MACROS_H

#define QQ_CONSTANT_PROPERTY(type,name,getf,val)\
	Q_PROPERTY(type name READ getf STORED false)\
	public:    const type getf() { return val; }\

#define QQ_CONSTANT_PTR_PROPERTY(type,name,getf,val)\
	Q_PROPERTY(type name READ getf STORED false)\
	public:    type getf() { return val; }\

#define QQ_CONSTANT_REF_PROPERTY(type,name,getf,val)\
	Q_PROPERTY(type name READ getf STORED false)\
	public:    const type & getf() { return val; }\


#define QQ_READONLY_LOCAL_PROPERTY(type,name,getf)				\
	Q_PROPERTY(type name READ getf STORED true)\
	protected: type m_##name;\
	public:    type getf() { return m_##name; }\

#define QQ_READONLY_LOCAL_PTR_PROPERTY(type,name,getf)\
	Q_PROPERTY(type name READ getf STORED true)\
	protected: type m_##name;\
	public:    type getf() { return m_##name; }\

#define QQ_READONLY_LOCAL_REF_PROPERTY(type,name,getf)\
	Q_PROPERTY(type name READ getf STORED true)\
	protected: type * m_##name;\
	public:    type & getf() { return *m_##name; }\


#define QQ_LOCAL_PROPERTY(type,name,getf,setf)\
	Q_PROPERTY(type name READ getf WRITE setf STORED true NOTIFY name##Changed)\
	protected: type m_##name;\
	public:    type getf() { return m_##name; }\
	           void setf(const type new_val) { if(m_##name!=new_val) { m_##name=new_val; Q_EMIT name##Changed(new_val); Q_EMIT wasModified(); } }\
	  Q_SIGNAL void name##Changed(type);\

#define QQ_LOCAL_PTR_PROPERTY(type,name,getf,setf)\
	Q_PROPERTY(type name READ getf WRITE setf STORED true NOTIFY name##Changed)\
	protected: type m_##name;\
	public:    type getf() { return m_##name; }\
	           void setf(type new_val) { if(m_##name!=new_val) { m_##name=new_val; Q_EMIT name##Changed(new_val); Q_EMIT wasModified(); } }\
	  Q_SIGNAL void name##Changed(type);\

#define QQ_LOCAL_REF_PROPERTY(type,name,getf,setf)\
	Q_PROPERTY(type name READ getf WRITE setf STORED true NOTIFY name##Changed)\
	protected: type * m_##name;\
	public:    type & getf() { return *m_##name; }\
	           void setf(type & new_val) { if(*m_##name!=new_val) { m_##name=&new_val; Q_EMIT name##Changed(new_val); Q_EMIT wasModified(); } }\
	  Q_SIGNAL void name##Changed(type &);\


#define QQ_LOCAL_PROPERTY_DV(type,name,getf,setf,def_val)\
	Q_PROPERTY(type name READ getf WRITE setf STORED true NOTIFY name##Changed)\
	protected: type m_##name = def_val;\
	public:    type getf() { return m_##name; }\
	           void setf(const type new_val) { if(m_##name!=new_val) { m_##name=new_val; Q_EMIT name##Changed(new_val); Q_EMIT wasModified(); } }\
	  Q_SIGNAL void name##Changed(type);\

#define QQ_LOCAL_PTR_PROPERTY_DV(type,name,getf,setf,def_val)\
	Q_PROPERTY(type name READ getf WRITE setf STORED true NOTIFY name##Changed)\
	protected: type m_##name = def_val;\
	public:    type getf() { return m_##name; }\
	           void setf(type new_val) { if(m_##name!=new_val) { m_##name=new_val; Q_EMIT name##Changed(new_val); Q_EMIT wasModified(); } }\
	  Q_SIGNAL void name##Changed(type);\

#define QQ_LOCAL_REF_PROPERTY_DV(type,name,getf,setf,def_val)\
	Q_PROPERTY(type name READ getf WRITE setf STORED true NOTIFY name##Changed)\
	protected: type * m_##name = &def_val;\
	public:    type & getf() { return *m_##name; }\
	           void setf(type & new_val) { if(*m_##name!=new_val) { m_##name=&new_val; Q_EMIT name##Changed(new_val); Q_EMIT wasModified(); } }\
	  Q_SIGNAL void name##Changed(type &);\


#define QQ_LOCAL_RESETABLE_PROPERTY(type,name,getf,setf,resetf,def_val)\
	Q_PROPERTY(type name READ getf WRITE setf STORED true NOTIFY name##Changed)\
	protected: type m_##name = def_val;\
	public:    type getf() { return m_##name; }\
		   void setf(const type new_val) { if(m_##name!=new_val) { m_##name=new_val; Q_EMIT name##Changed(new_val); } }\
		   void resetf() { if(m_##name!=def_val) setf(def_val); }\
	  Q_SIGNAL void name##Changed(type);\

#define QQ_LOCAL_RESETABLE_PTR_PROPERTY(type,name,getf,setf,resetf,def_val)\
	Q_PROPERTY(type name READ getf WRITE setf STORED true NOTIFY name##Changed)\
	protected: type m_##name = def_val;\
	public:    type getf() { return m_##name; }\
		   void setf(type new_val) { if(m_##name!=new_val) { m_##name=new_val; Q_EMIT name##Changed(new_val); } }\
		   void resetf() { if(m_##name!=def_val) setf(def_val); }\
	  Q_SIGNAL void name##Changed(type);\

#define QQ_LOCAL_RESETABLE_REF_PROPERTY(type,name,getf,setf,resetf,def_val)\
	Q_PROPERTY(type name READ getf WRITE setf STORED true NOTIFY name##Changed)\
	protected: type * m_##name = &def_val;\
	public:    type & getf() { return *m_##name; }\
		   void setf(type & new_val) { if(*m_##name!=new_val) { m_##name=&new_val; Q_EMIT name##Changed(new_val); } }\
		   void resetf() { if(*m_##name!=def_val) setf(def_val); }\
	  Q_SIGNAL void name##Changed(type &);\


#define QQ_READONLY_DELEGATED_PROPERTY(type,name,getf1,delegate,getf2)\
	Q_PROPERTY(type name READ getf1 STORED false)\
	public:   type getf1() { return delegate.getf2(); }\

#define QQ_READONLY_DELEGATED_PTR_PROPERTY(type,name,getf1,delegate,getf2)\
	Q_PROPERTY(type name READ getf1 STORED false)\
	public:   type getf1() { return delegate.getf2(); }\

#define QQ_READONLY_DELEGATED_REF_PROPERTY(type,name,getf1,delegate,getf2)\
	Q_PROPERTY(type name READ getf1 STORED false)\
	public:   type & getf1() { return delegate.getf2(); }\


#define QQ_READONLY_PTR_DELEGATED_PROPERTY(type,name,getf1,delegate,getf2)\
	Q_PROPERTY(type name READ getf1 STORED false)\
	public:   type getf1() { return delegate->getf2(); }\

#define QQ_READONLY_PTR_DELEGATED_PTR_PROPERTY(type,name,getf1,delegate,getf2)\
	Q_PROPERTY(type name READ getf1 STORED false)\
	public:   type getf1() { return delegate->getf2(); }\

#define QQ_READONLY_PTR_DELEGATED_REF_PROPERTY(type,name,getf1,delegate,getf2)\
	Q_PROPERTY(type name READ getf1 STORED false)\
	public:   type & getf1() { return delegate->getf2(); }\


#define QQ_DELEGATED_PROPERTY(type,name,getf1,setf1,delegate,getf2,setf2)\
	Q_PROPERTY(type name READ getf1 WRITE setf1 STORED false NOTIFY name##Changed)\
	public:   type getf1() { return delegate.getf2(); }\
		  void setf1(const type new_val) { if(delegate.getf2()!=new_val) { delegate.setf2(new_val); Q_EMIT name##Changed(new_val); } }\
	 Q_SIGNAL void name##Changed(type);\

#define QQ_DELEGATED_PTR_PROPERTY(type,name,getf1,setf1,delegate,getf2,setf2)\
	Q_PROPERTY(type name READ getf1 WRITE setf1 STORED false NOTIFY name##Changed)\
	public:   type getf1() { return delegate.getf2(); }\
		  void setf1(type new_val) { if(delegate.getf2()!=new_val) { delegate.setf2(new_val); Q_EMIT name##Changed(new_val); } }\
	 Q_SIGNAL void name##Changed(type);\

#define QQ_DELEGATED_REF_PROPERTY(type,name,getf1,setf1,delegate,getf2,setf2)\
	Q_PROPERTY(type name READ getf1 WRITE setf1 STORED false NOTIFY name##Changed)\
	public:   type & getf1() { return delegate.getf2(); }\
		  void setf1(const type & new_val) { if(delegate.getf2()!=new_val) { delegate.setf2(new_val); Q_EMIT name##Changed(new_val); } }\
	 Q_SIGNAL void name##Changed(type);\


#define QQ_PTR_DELEGATED_PROPERTY(type,name,getf1,setf1,delegate,getf2,setf2)\
	Q_PROPERTY(type name READ getf1 WRITE setf1 STORED false NOTIFY name##Changed)\
	public:   type getf1() { return delegate->getf2(); }\
		  void setf1(const type new_val) { if(delegate->getf2()!=new_val) { delegate->setf2(new_val); Q_EMIT name##Changed(new_val); } }\
	 Q_SIGNAL void name##Changed(type);\

#define QQ_PTR_DELEGATED_PTR_PROPERTY(type,name,getf1,setf1,delegate,getf2,setf2)\
	Q_PROPERTY(type name READ getf1 WRITE setf1 STORED false NOTIFY name##Changed)\
	public:   type getf1() { return delegate->getf2(); }\
		  void setf1(type new_val) { if(delegate->getf2()!=new_val) { delegate->setf2(new_val); Q_EMIT name##Changed(new_val); } }\
	 Q_SIGNAL void name##Changed(type);\

#define QQ_PTR_DELEGATED_REF_PROPERTY(type,name,getf1,setf1,delegate,getf2,setf2)\
	Q_PROPERTY(type name READ getf1 WRITE setf1 STORED false NOTIFY name##Changed)\
	public:   type & getf1() { return delegate->getf2(); }\
		  void setf1(const type & new_val) { if(delegate->getf2()!=new_val) { delegate->setf2(new_val); Q_EMIT name##Changed(new_val); } }\
	 Q_SIGNAL void name##Changed(type);\


#define QQ_OBJECT(classname)\
	Q_OBJECT\
        public:  Q_SIGNAL void wasModified();

/***
  Usage:
    - Using default value is possible but not advised
    - REF_PROPERTY only works on primitive and copy-value types. Use PTR_PROPERTY for
      QObject-derived types.

  Example:

#include <QObject>
#include <QString>

class Field : public QObject
{
	QQ_OBJECT(Field)
};

const static QString HELLO="Hello";
static QString HELLO2="Hello";
static Field   BYE;
static QObject OBJET;

class Sandbox : public QObject
{
	QQ_OBJECT(Sandbox)

	QQ_CONSTANT_PROPERTY(int    ,wc,wc,10)
	QQ_CONSTANT_PROPERTY(QColor ,xc,xc,Qt::white)
	QQ_CONSTANT_PROPERTY(QString,yc,yc,HELLO)
	QQ_CONSTANT_PTR_PROPERTY(Field*,ac,ac,&BYE)
	QQ_CONSTANT_REF_PROPERTY(QString,zc,zc,HELLO)
		//this is a QObject: QQ_CONSTANT_PROPERTY(Field  ,zc,zc,BYE)
		//this is a QObject: QQ_CONSTANT_REF_PROPERTY(Field ,bc,bc,BYE)

	QQ_LOCAL_PROPERTY(int    ,wl,wl,setWl)
	QQ_LOCAL_PROPERTY(QColor ,xl,xl,setXl)
	QQ_LOCAL_PROPERTY(QString,yl,yl,setYl)
	QQ_LOCAL_PTR_PROPERTY(Field*,al,al,setAl)
	QQ_LOCAL_REF_PROPERTY(QString,zl,zl,setZl)
		//this is a QObject: QQ_LOCAL_PROPERTY(Field  ,zl,zl,setZl)
		//this is a QObject: QQ_LOCAL_REF_PROPERTY(Field ,bl,bl,setBl)

	QQ_LOCAL_RESETABLE_PROPERTY(int    ,wr,wr,setWr,resetWr,10)
        QQ_LOCAL_RESETABLE_PROPERTY(QColor ,xr,xr,setXr,resetXr,Qt::white)
        QQ_LOCAL_RESETABLE_PROPERTY(QString,yr,yr,setYr,resetYr,HELLO)
	QQ_LOCAL_RESETABLE_PTR_PROPERTY(Field*,ar,ar,setAr,resetAr,&BYE)
	QQ_LOCAL_RESETABLE_REF_PROPERTY(QString,zr,zr,setZr,resetZr,HELLO2)
		//this is a QObject: QQ_LOCAL_RESETABLE_PROPERTY(Field  ,zr,zr,setZr,resetZr,BYE)
		//this is a QObject: QQ_LOCAL_RESETABLE_REF_PROPERTY(Field ,br,br,setBr,resetBr,BYE)

	QQ_DELEGATED_PROPERTY(QString,wa,wa,setWa,OBJET,objectName,setObjectName)
	QQ_DELEGATED_PTR_PROPERTY(QObject*,aa,aa,setAa,OBJET,parent,setParent)
		// not a ref: QQ_DELEGATED_REF_PROPERTY(QString,za,za,setZa,OBJET,objectName,setObjectName)
		//QQ_READONLY_DELEGATED_REF_PROPERTY(NSString,ba,ba,HELLO,toNSString)

public:
	void update() { };

private:
	Sandbox();
};
*/

#endif
