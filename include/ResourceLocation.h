/*
 * ResourceLocation.h - header file for ResourceLocation
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef _RESOURCE_LOCATION_H
#define _RESOURCE_LOCATION_H

#include <QtCore/QString>

class QDomNode;
class ResourceProvider;

/*! \brief The ResourceLocation class stores information about a location of
 * resources and creates according ResourceLocation instances.
 *
 * A ResourceLocation instance holds all information about a location of
 * resources which are neccessary to create instances of the various
 * ResourceLocation implementations.
 */

class ResourceLocation
{
public:
	/*! Lists all supported location types. */
	enum Types
	{
		Unknown,			/*!< Unknown location */
		LocalDirectory,		/*!< Resources are stored inside and below a locally accessible directory. */
		Web,				/*!< Resources are stored at a website which supports LMMS' WebResources. */
		NumTypes
	} ;
	typedef Types Type;

	/*! \brief Constructs a ResourceLocation object.
	* \param locationCfg A QDomNode holding all information stored in a ResourceLocation object
	*/
	ResourceLocation( const QDomNode & locationCfg );

	/*! \brief Constructs a ResourceLocation object.
	* \param name The (descriptive) name of the location
	* \param type The #Type of the location
	* \param address The address of the location
	*/
	ResourceLocation( const QString & name = QString(),
						Type type = Unknown,
						const QString & address = QString() );

	/*! \brief Destroys the ResourceLocation object. */
	~ResourceLocation();

	/*! \brief Creates a ResourceProvider instance depending on type and address. */
	ResourceProvider * createResourceProvider();

	/*! \brief Sets the name of the resource location. */
	void setName( const QString & name )
	{
		m_name = name;
	}

	/*! \brief Sets the type of the resource location. */
	void setType( Type type )
	{
		m_type = type;
	}

	/*! \brief Sets the address of the resource location. */
	void setAddress( const QString & address )
	{
		m_address = address;
	}

	/*! \brief Returns the name of the resource location. */
	const QString & name() const
	{
		return m_name;
	}

	/*! \brief Returns the type of the resource location. */
	Type type() const
	{
		return m_type;
	}

	/*! \brief Returns the address of the resource location. */
	const QString & address() const
	{
		return m_address;
	}

	/*! \brief Returns whether all neccessary information on the location are valid. */
	bool isValid() const
	{
		return type() > Unknown && type() < NumTypes && !address().isEmpty();
	}


private:
	QString m_name;
	Type m_type;
	QString m_address;

} ;


#endif
