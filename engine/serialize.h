/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UFOATTACK_SERIALIZE_INCLUDED
#define UFOATTACK_SERIALIZE_INCLUDED

/*	WARNING everything assumes little endian. Nead to rework
	save/load if this changes.
*/

#include <stdio.h>
#include "../grinliz/gldebug.h"
#include "../grinliz/gltypes.h"
#include "../grinliz/glrectangle.h"
#include "../grinliz/glvector.h"
#include "../grinliz/glstringutil.h"
#include "../shared/gamedbreader.h"
#include "enginelimits.h"

struct SDL_RWops;


struct ModelGroup
{
	grinliz::CStr<EL_FILE_STRING_LEN> textureName;
	U16						nVertex;
	U16						nIndex;

	void Set( const char* textureName, int nVertex, int nIndex );
	void Load( const gamedb::Item* item );
};


struct ModelHeader
{
	// flags
	enum {
		RESOURCE_NO_SHADOW	= 0x08,		// model casts no shadow
	};
	bool NoShadow() const			{ return (flags&RESOURCE_NO_SHADOW) ? true : false; }

	grinliz::CStr<EL_FILE_STRING_LEN>	name;
	U16						nTotalVertices;		// in all groups
	U16						nTotalIndices;
	U16						flags;
	U16						nGroups;
	grinliz::Rectangle3F	bounds;
	grinliz::Vector3F		trigger;			// location for gun
	float					eye;				// location model "looks from"
	float					target;				// height of chest shot

	void Set( const char* name, int nGroups, int nTotalVertices, int nTotalIndices,
			  const grinliz::Rectangle3F& bounds );

	void Load(	const gamedb::Item* );	// database connection
	void Save(	gamedb::WItem* parent );	// database connection
};

#define XML_PUSH_ATTRIB( printer, value ) { printer->PushAttribute( #value, value ); }

/*
class XMLUtil
{
public:
	static void OpenElement( FILE* fp, int depth, const char* value );
	static void SealElement( FILE* fp );
	static void CloseElement( FILE* fp, int depth, const char* value );
	static void SealCloseElement( FILE* fp );

	static void Text( FILE* fp, const char* text );

	static void Attribute( FILE* fp, const char* name, const char* value );
	static void Attribute( FILE* fp, const char* name, int value );
	static void Attribute( FILE* fp, const char* name, unsigned value );
	static void Attribute( FILE* fp, const char* name, float value );

private:
	static void Space( FILE* fp, int depth );
};
*/

#endif // UFOATTACK_SERIALIZE_INCLUDED
