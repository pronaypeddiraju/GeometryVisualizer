/////////////////////////////////////////////////////////////////////////////////////////////////
// BufferUtilities.hpp
//
// Basic byte-buffer binary utility functions.  Used for de facto network & file i/o via buffers.
//
#pragma once
#include "Engine/Core/EngineBase.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Core/IntVector2.hpp"
#include "Engine/Core/Vector2.hpp"
#include "Engine/Core/Vector3.hpp"
#include "Engine/Core/Rgba.hpp"
#include <string>
#include <vector>


//------------------------------------------------------------------------------------------------
typedef std::vector<unsigned char> Buffer;


//------------------------------------------------------------------------------------------------
enum class eBufferEndian
{
	NATIVE,
	LITTLE,
	BIG
};


//------------------------------------------------------------------------------------------------
// Some basic buffer-related utility functions
//
void Reverse2BytesInPlace( void* ptrTo16BitWord ); // for short
void Reverse4BytesInPlace( void* ptrTo32BitDword ); // for int, uint, float
void Reverse8BytesInPlace( void* ptrTo64BitQword ); // for double, uint64
void ReverseBytesInPlacePtrSizeT( void* ptrToPtrOrSizeT ); // For "pointer-sized" objects (e.g. size_t), either 4 or 8 bytes (in x86 and x64, respectively)


//////////////////////////////////////////////////////////////////////////////////////////////////
// Transient (temporary, typically stack-local) utility class for parsing binary data from a
//	byte-buffer.  Handles endian-correctness, buffer underflow, etc.
//////////////////////////////////////////////////////////////////////////////////////////////////
class BufferParser
{
public:
	BufferParser( const unsigned char* bufferData, size_t bufferSize, eBufferEndian endianMode=eBufferEndian::NATIVE );
	BufferParser( const Buffer& buffer, eBufferEndian endianMode=eBufferEndian::NATIVE );

	void			SetEndianMode( eBufferEndian endianModeToUseForSubsequentReads );
	bool			IsEndianModeBig() const					{ return (m_endianMode == eBufferEndian::BIG) || (m_endianMode == eBufferEndian::NATIVE && PLATFORM_IS_BIG_ENDIAN); }
	bool			IsEndianModeOppositeNative() const		{ return m_isOppositeEndian; }

	unsigned char			ParseByte();
	char					ParseChar();
	int						ParseInt32();
	unsigned int			ParseUint32();
	float					ParseFloat();
	double					ParseDouble();
	bool					ParseBool();
	const unsigned char*	ParseBytes( size_t numBytes );
	void					ParseByteArray( std::vector<unsigned char>& out_bytes, size_t numBytesToParse );
	void					ParseByteArray( unsigned char* out_destArray, size_t numBytesToParse );
	void					ParseStringZeroTerminated( std::string& out_string );
	void					ParseStringAfter32BitLength( std::string& out_string );
	void					ParseStringAfter8BitLength( std::string& out_string );
	void					ParseStringOfLength( std::string& out_string, unsigned int stringLength );
	Rgba					ParseRgb(); // reads only 3 bytes (R,G,B) and assumes Alpha=255
	Rgba					ParseRgba(); // reads 4 bytes in RGBA order
	Vec2					ParseVec2(); // reads 2 floats: x,y
	Vec3					ParseVec3(); // reads 3 floats: x,y,z
	IntVec2					ParseIntVec2(); // reads 2 ints: x,y
	VertexPCU				ParseVertexPCU(); // reads Vec3 pos, Rgba color, Vec2 uv

	size_t			GetTotalSize() const		{ return m_bufferSize; }
	size_t			GetRemainingSize() const	{ return (m_scanEnd - m_scanPosition) + 1; }
	bool			IsAtEnd() const				{ return m_scanPosition > m_scanEnd; }
	bool			IsBufferDataAvailable( size_t numBytes ) const; // return false if not
	void			GuaranteeBufferDataAvailable( size_t numBytes ) const; // error/assert if not!

private:
	eBufferEndian			m_endianMode		= eBufferEndian::NATIVE;
	bool					m_isOppositeEndian	= false;
	size_t					m_bufferSize		= 0;
	const unsigned char*	m_scanStart			= nullptr; // start of buffer data being parsed
	const unsigned char*	m_scanPosition		= nullptr; // moves from start to end+1
	const unsigned char*	m_scanEnd			= nullptr; // last VALID character in buffer
};


//------------------------------------------------------------------------------------------------
inline bool BufferParser::IsBufferDataAvailable( size_t numBytes ) const
{
	return m_scanPosition + (numBytes-1) <= m_scanEnd;
}


//------------------------------------------------------------------------------------------------
inline void BufferParser::GuaranteeBufferDataAvailable( size_t numBytes ) const
{
	GUARANTEE_OR_DIE( m_scanPosition + (numBytes-1) <= m_scanEnd, Stringf( "Insufficient data in buffer to read %i bytes", numBytes ) );
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// Transient (temporary, typically stack-local) utility class for building a byte-buffer by
//	pushing ints, float, strings, etc. into it.  Handles endian-correctness, etc.
//////////////////////////////////////////////////////////////////////////////////////////////////
class BufferWriter
{
public:
	BufferWriter( Buffer& buffer, eBufferEndian endianMode=eBufferEndian::NATIVE );

	// Endian-related settings; if ignored, assumes build platform's native endian (LITTLE for x86/x64)
	void SetEndianMode( eBufferEndian endianModeToUseForSubsequentReads );
	bool IsEndianModeBig() const				{ return (m_endianMode == eBufferEndian::BIG) || (m_endianMode == eBufferEndian::NATIVE && PLATFORM_IS_BIG_ENDIAN); }
	bool IsEndianModeOppositeNative() const		{ return m_isOppositeEndian; }

	Buffer&	GetBuffer() const			{ return m_buffer; }
	size_t	GetTotalSize() const		{ return m_buffer.size(); }
	size_t	GetAppendedSize() const		{ return m_buffer.size() - m_initialSize; }
	void	ReserveAdditional( size_t additionalBytes ); // stretch capacity +N bytes beyond current *size*

	// Each of these appends to the buffer's end
	void AppendByte( unsigned char b );
	void AppendChar( char c );
	void AppendInt32( int i );
	void AppendUint32( unsigned int u );
	void AppendFloat( float f );
	void AppendDouble( double d );
	void AppendBool( bool b ); // writes as byte 0x00 for false, 0x01 for true
	void AppendByteArray( const unsigned char* byteArray, size_t numBytesToWrite );
	void AppendByteArray( const std::vector<unsigned char>& byteArray );
	void AppendStringZeroTerminated( const char* s );
	void AppendStringZeroTerminated( const std::string& s );
	void AppendStringAfter32BitLength( const char* s, unsigned int knownLength = 0 );
	void AppendStringAfter32BitLength( const std::string& s );
	void AppendStringAfter8BitLength( const char* s, unsigned char knownLength = 0 );
	void AppendStringAfter8BitLength( const std::string& s );
	void AppendRgb( const Rgba& rgb_ignoreAlpha ); // writes only 3 bytes (R,G,B) and ignores Alpha
	void AppendRgba( const Rgba& rgba ); // writes 4 bytes, always in RGBA order
	void AppendVec2( const Vec2& vec2 ); // writes 2 floats: x,y
	void AppendVec3( const Vec3& vec3 ); // writes 3 floats: x,y,z
	void AppendIntVec2( const IntVec2& ivec2 ); // writes 2 ints: x,y
	void AppendVertexPCU( const VertexPCU& vertex ); // writes Vec3 pos, Rgba color, Vec2 uv
	void AppendZeros( size_t howManyBytesWorthOfZerosToAppend );
	unsigned char* AppendUninitializedBytes( size_t howManyJunkBytesToAppend ); // returns the START of the new uninitialized bytes (typically to copy into)

	eBufferEndian			m_endianMode		= eBufferEndian::NATIVE;
	bool					m_isOppositeEndian	= false; // true if buffer matches native (don't need to reverse)
	Buffer&					m_buffer;
	size_t					m_initialSize		= 0; // # of bytes in buffer BEFORE this session began appending
};


//------------------------------------------------------------------------------------------------
inline void Reverse2BytesInPlace( void* ptrTo16BitWord )
{
	unsigned short u = *(unsigned short*) ptrTo16BitWord;
	*(unsigned short*) ptrTo16BitWord =	((u & 0x00ff) << 8)  |
										((u & 0xff00) >> 8);
}


//------------------------------------------------------------------------------------------------
inline void Reverse4BytesInPlace( void* ptrTo32BitDword )
{
	unsigned int u = *(unsigned int*) ptrTo32BitDword;
	*(unsigned int*) ptrTo32BitDword =	((u & 0x000000ff) << 24) |
										((u & 0x0000ff00) << 8)  |
										((u & 0x00ff0000) >> 8)  |
										((u & 0xff000000) >> 24);
}


//------------------------------------------------------------------------------------------------
inline void Reverse8BytesInPlace( void* ptrTo64BitQword )
{
	int64_t u = *(int64_t*) ptrTo64BitQword;
	*(int64_t*) ptrTo64BitQword =	((u & 0x00000000000000ff) << 56) |
									((u & 0x000000000000ff00) << 40) |
									((u & 0x0000000000ff0000) << 24) |
									((u & 0x00000000ff000000) << 8)  |
									((u & 0x000000ff00000000) >> 8)  |
									((u & 0x0000ff0000000000) >> 24) |
									((u & 0x00ff000000000000) >> 40) |
									((u & 0xff00000000000000) >> 56);
}


//------------------------------------------------------------------------------------------------
inline void ReverseBytesInPlacePtrSizeT( void* ptrToPtrOrSizeT )
{
	if constexpr( sizeof(void*) == 8 )
	{
		Reverse8BytesInPlace( ptrToPtrOrSizeT );
	}
	else
	{
		Reverse4BytesInPlace( ptrToPtrOrSizeT );
	}
}
