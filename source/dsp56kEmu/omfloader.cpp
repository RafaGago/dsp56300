#include "pch.h"

#include "omfloader.h"
#include "memory.h"
#include "error.h"

#include <istream>
#include <fstream>

static const std::string g_tagStart  ("_START ");
static const std::string g_tagData   ("_DATA ");
static const std::string g_tagSymbol ("_SYMBOL ");

namespace dsp56k
{

// _____________________________________________________________________________
// load
//
bool dsp56k::OMFLoader::load( const char* _filename, Memory& _dst )
{
	std::ifstream i;
	i.open( _filename, std::ios_base::in );
	if( !i.is_open() )
		return false;

	std::list<std::string> lines;

	while(!i.eof())
	{
		std::string line;
		std::getline( i, line );
		lines.push_back( line );
	}

	i.close();

	return load( lines, _dst );
}

// _____________________________________________________________________________
// load
//
bool OMFLoader::load( const std::string& _istr, Memory& _dst )
{
	return load( _istr.c_str(), _dst );
}

// _____________________________________________________________________________
// load
//
bool OMFLoader::load( const std::list<std::string>& _lines, Memory& _dst )
{
	m_currentArea = MemArea_COUNT;
	m_currentTargetAddress = 0;
	m_currentBitSize = m_currentByteSize = 0;

	for( std::list<std::string>::const_iterator it = _lines.begin(); it != _lines.end(); ++it )
	{
		const std::string& line = *it;

		parseLine( line, _dst );
	}

	return true;
}

// _____________________________________________________________________________
// parseLine
//
void OMFLoader::parseLine( const std::string& _line, Memory& _dst )
{
	if( _line.empty() )
		return;

	if( _line[0] == '_' )
	{
		// check for new memory area etc

		if( _line.substr( 0, g_tagData.size() ) == g_tagData )
		{
			char memoryLocation = _line[6];

			m_currentArea = MemArea_COUNT;

			m_currentBitSize = 0;

			switch( memoryLocation )
			{
			case 'X':	m_currentArea = MemArea_X;	m_currentBitSize = 24; break;
			case 'Y':	m_currentArea = MemArea_Y;	m_currentBitSize = 24; break;
			case 'P':	m_currentArea = MemArea_P;	m_currentBitSize = 24; break;
			case 'L':	m_currentArea = MemArea_X;	m_currentBitSize = 48; break;
			default:	assert( 0 && "unknown area" ); break;
			}

			m_currentByteSize = m_currentBitSize >> 3;

			m_currentTargetAddress = parse24Bit( _line.c_str() + 8 );
		}
		else
		{
			m_currentArea = MemArea_COUNT;
			m_currentBitSize = m_currentByteSize = 0;
			m_currentTargetAddress = 0;
		}
	}
	else
	{
		if( m_currentArea != MemArea_COUNT )
		{
			// read data into one of our memory targets
			const size_t len = _line.size();
			const char* src = _line.c_str();

			if( m_currentBitSize == 24 )
			{
				int remaining = int(_line.size());
				while( remaining >= 6 )
				{
					uint result = parse24Bit( src );				

					_dst.set( m_currentArea, m_currentTargetAddress, result );

					m_currentTargetAddress++;

					src += 7;		// 6 hex digits + space
					remaining -= 7;
				}
				assert( remaining == 0 );
			}
			else if( m_currentBitSize == 48 )
			{
				int remaining = int(_line.size());

				while( remaining >= 13 )
				{
					const uint res0 = parse24Bit( src );
					const uint res1 = parse24Bit( src+7 );

					// TODO: this might need to get reversed? probably not....
					_dst.set( MemArea_X, m_currentTargetAddress, res0 );
					_dst.set( MemArea_Y, m_currentTargetAddress, res1 );

					++m_currentTargetAddress;

					src += 14;		// 2 times 6 hex digits + space
					remaining -= 14;
				}
				assert( remaining == 0 );
			}
		}
	}
}
// _____________________________________________________________________________
// parse24Bit
//
uint OMFLoader::parse24Bit( const char* _src )
{
	char temp[3] = {0,0,0};

	uint result = 0;

	uint shift = 16;

	for( size_t i=0; i<6; i+=2, shift -= 8 )
	{
		temp[0] = _src[i];
		temp[1] = _src[i+1];

		uint r;
		sscanf_s( temp, "%02x", &r );

		result |= (r<<shift);
	}
	return result;
}
// _____________________________________________________________________________
// parse24Bit
//
void OMFLoader::parse24Bit( const char* _src, uchar* _dst )
{
	char temp[3] = {0,0,0};

	uint offset = 0;

	for( size_t i=0; i<6; i+=2, ++offset )
	{
		temp[0] = _src[i];
		temp[1] = _src[i+1];

		uint r;
		sscanf_s( temp, "%02x", &r );

		_dst[offset] = (r&0xff);
	}
}

}
