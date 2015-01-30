/*
 * QtXmlWrapper.h - a QtXml based XML backend for ZynAddSubxFX
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

/* File derived from XMLwrapper.h: */
/*
  ZynAddSubFX - a software synthesizer

  XMLwrapper.h - XML wrapper
  Copyright (C) 2003-2005 Nasca Octavian Paul
  Copyright (C) 2009-2009 Mark McCurry
  Author: Nasca Octavian Paul
          Mark McCurry

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#ifndef QT_XML_WRAPPER_H
#define QT_XML_WRAPPER_H

#include "../globals.h"

#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#define QtXmlWrapper XMLwrapper

struct XmlData;

class QtXmlWrapper
{
public:
	QtXmlWrapper();
	~QtXmlWrapper();

	int saveXMLfile( const std::string & filename ) const;
	int loadXMLfile( const std::string & filename );

	char *getXMLdata() const;
	bool putXMLdata( const char *xmldata );

	void addpar( const std::string & name, int val );
	void addparreal( const std::string & name, float val);
	void addparbool( const std::string & name, int val );
	void addparstr( const std::string & name, const std::string & val );

	void beginbranch( const std::string & name );
	void beginbranch( const std::string & name, int id );
	void endbranch();


	int enterbranch( const std::string & name );
	int enterbranch( const std::string & name, int id );
	void exitbranch();
	int getbranchid( int min, int max ) const;

	int getpar( const std::string & name, int defaultpar, int min, int max ) const;
	int getpar127( const std::string & name, int defaultpar ) const;
	int getparbool( const std::string & name, int defaultpar ) const;

	void getparstr( const std::string & name, char * par, int maxstrlen ) const;
	std::string getparstr( const std::string & name, const std::string & defaultpar ) const;
	float getparreal( const char * name, float defaultpar ) const;

	float getparreal(const char *name, float defaultpar, float min, float max) const;

	bool minimal; /**<false if all parameters will be stored (used only for clipboard)*/

	void setPadSynth( bool enabled );
	bool hasPadSynth() const;


private:
	int dosavefile(const char *filename, int compression, const char *xmldata) const;

	char *doloadfile(const std::string &filename) const;

	struct
	{
		int Major;
		int Minor;
		int Revision;
	} version;

	XmlData * d;


};

#endif

