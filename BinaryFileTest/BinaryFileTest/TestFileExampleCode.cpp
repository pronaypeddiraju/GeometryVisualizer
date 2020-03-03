//------------------------------------------------------------------------------------------------
bool Command_TestBinaryFileLoad( NamedProperties& args );
bool Command_TestBinaryFileSave( NamedProperties& args );

//------------------------------------------------------------------------------------------------
constexpr int TEST_BUF_SIZE = 204;

//------------------------------------------------------------------------------------------------
void InitializeBufferTest()
{
	RegisterEventCallbackFunction( "TestBinaryFileLoad", Command_TestBinaryFileLoad, "Load test binary file" );
	RegisterEventCallbackFunction( "TestBinaryFileSave", Command_TestBinaryFileSave, "Save test binary file" );

	// Just trying stuff out here initially to see if buffer writing/parsing seems to be working
	Vector3 pos( 3.f, 4.f, 5.f );
	IntVector3 coords( 3, 4, 5 );
	
	BufferParser bufVec3( (unsigned char*) &pos, sizeof(pos) );
	float x = bufVec3.ParseFloat();
	float y = bufVec3.ParseFloat();
	float z = bufVec3.ParseFloat();

	BufferParser bufLittle( (unsigned char*) &coords, sizeof(coords) );
	int x1 = bufLittle.ParseInt32();
	int y1 = bufLittle.ParseInt32();
	int z1 = bufLittle.ParseInt32();

	BufferParser bufBig( (unsigned char*) &coords, sizeof(coords), eBufferEndian::BIG );
	int x2 = bufBig.ParseInt32();
	int y2 = bufBig.ParseInt32();
	int z2 = bufBig.ParseInt32();

	BufferParser bufNative( (unsigned char*) &coords, sizeof(coords), eBufferEndian::NATIVE );
	int x3 = bufNative.ParseInt32();
	int y3 = bufNative.ParseInt32();
	int z3 = bufNative.ParseInt32();

	Buffer b;
	b.push_back( 0xff );
	b.push_back( 0x00 );
	b.push_back( 0xa3 );

	BufferWriter bw( b, eBufferEndian::BIG );
	bw.AppendByte( 0x12 );
	bw.AppendByte( 0x34 );
	bw.AppendByte( 0x56 );
	bw.AppendByte( 0x78 );
	bw.AppendInt32( 42 );
	bw.AppendFloat( 3.14159265f );

	BufferWriter bw2( b, eBufferEndian::LITTLE );
	bw2.AppendInt32( 15 );

	double dp1 = 3.1415926535897932384626433832795;
	BufferParser dpBuf( (unsigned char*) &dp1, sizeof(dp1) );
	double dp2 = dpBuf.ParseDouble();
	GUARANTEE_RECOVERABLE_NOREPEAT0( dp1 == dp2 );

	Buffer dpBuf2;
	BufferWriter dpWrite2( dpBuf2 );
	dpWrite2.AppendDouble( dp1 );
	
	BufferParser dpParse2( dpBuf2 );
	double dp3 = dpParse2.ParseDouble();
	GUARANTEE_RECOVERABLE_NOREPEAT0( dp1 == dp3 );
}


//------------------------------------------------------------------------------------------------
void AppendTestFileBufferData( BufferWriter& bufWrite, eBufferEndian endianMode )
{
	bufWrite.SetEndianMode( endianMode );
	bufWrite.AppendChar( 'T' );
	bufWrite.AppendChar( 'E' );
	bufWrite.AppendChar( 'S' );
	bufWrite.AppendChar( 'T' );
	bufWrite.AppendByte( 1 ); // Version 1
	bufWrite.AppendBool( bufWrite.IsEndianModeBig() ); // written as byte 0 or 1
	bufWrite.AppendUint32( 0x12345678 );
	bufWrite.AppendInt32( -7 ); // signed 32-bit int
	bufWrite.AppendFloat( 1.f ); // in memory looks like hex: 00 00 80 3F (or 3F 80 00 00 in big endian)
	bufWrite.AppendDouble( 3.1415926535897932384626433832795 ); // actually 3.1415926535897931 (best it can do)
	bufWrite.AppendStringZeroTerminated( "Hello" ); // written with a trailing 0 ('\0') after (6 bytes total)
	bufWrite.AppendStringAfter32BitLength( "Is this thing on?" ); // uint 17, then 17 chars (no zero-terminator after)
	bufWrite.AppendRgba( Rgba( 200, 100, 50, 255 ) ); // four bytes in RGBA order (endian-independent)
	bufWrite.AppendByte( 8 ); // 0x08 == 8 (byte)
	bufWrite.AppendRgb( Rgba( 238, 221, 204, 255 ) ); // written as 3 bytes (RGB) only; ignores Alpha
	bufWrite.AppendByte( 9 ); // 0x09 == 9 (byte)
	bufWrite.AppendIntVec2( IntVector2( 1920, 1080 ) );
	bufWrite.AppendVec2( Vector2( -0.6f, 0.8f ) );
	bufWrite.AppendVertexPCU( VertexPCU( 3.f, 4.f, 5.f, Rgba(100,101,102,103), 0.125f, 0.625f ) );
}


//------------------------------------------------------------------------------------------------
void ParseTestFileBufferData( BufferParser& bufParse, eBufferEndian endianMode )
{
	// Parse known test elements
	bufParse.SetEndianMode( endianMode );
	char fourCC0_T			= bufParse.ParseChar(); // 'T' == 0x54 hex == 84 decimal
	char fourCC1_E			= bufParse.ParseChar(); // 'E' == 0x45 hex == 84 decimal
	char fourCC2_S			= bufParse.ParseChar(); // 'S' == 0x53 hex == 69 decimal
	char fourCC3_T			= bufParse.ParseChar(); // 'T' == 0x54 hex == 84 decimal
	unsigned char version	= bufParse.ParseByte(); // version 1
	bool isBigEndian		= bufParse.ParseBool(); // written in buffer as byte 0 or 1
	unsigned int largeUint	= bufParse.ParseUint32(); // 0x12345678
	int negativeSeven		= bufParse.ParseInt32(); // -7 (as signed 32-bit int)
	float oneF				= bufParse.ParseFloat(); // 1.0f
	double pi				= bufParse.ParseDouble(); // 3.1415926535897932384626433832795 (or as best it can)

	std::string helloString, isThisThingOnString;
	bufParse.ParseStringZeroTerminated( helloString ); // written with a trailing 0 ('\0') after (6 bytes total)
	bufParse.ParseStringAfter32BitLength( isThisThingOnString ); // written as uint 17, then 17 characters (no zero-terminator after)

	Rgba rustColor			= bufParse.ParseRgba(); // Rgba( 200, 100, 50, 255 )
	unsigned char eight		= bufParse.ParseByte(); // 0x08 == 8 (byte)
	Rgba seashellColor		= bufParse.ParseRgb(); // Rgba(238,221,204) written as 3 bytes (RGB) only; ignores Alpha
	unsigned char nine		= bufParse.ParseByte(); // 0x09 == 9 (byte)
	IntVector2 highDefRes	= bufParse.ParseIntVec2(); // IntVector2( 1920, 1080 )
	Vector2 normal2D		= bufParse.ParseVec2(); // Vector2( -0.6f, 0.8f )
	VertexPCU vertex		= bufParse.ParseVertexPCU(); // VertexPCU( 3.f, 4.f, 5.f, Rgba(100,101,102,103), 0.125f, 0.625f ) );

	// Validate actual values parsed
	GUARANTEE_RECOVERABLE_NOREPEAT0( fourCC0_T == 'T' );
	GUARANTEE_RECOVERABLE_NOREPEAT0( fourCC1_E == 'E' );
	GUARANTEE_RECOVERABLE_NOREPEAT0( fourCC2_S == 'S' );
	GUARANTEE_RECOVERABLE_NOREPEAT0( fourCC3_T == 'T' );
	GUARANTEE_RECOVERABLE_NOREPEAT0( version == 1 );
	GUARANTEE_RECOVERABLE_NOREPEAT0( isBigEndian == bufParse.IsEndianModeBig() );
	GUARANTEE_RECOVERABLE_NOREPEAT0( largeUint == 0x12345678 );
	GUARANTEE_RECOVERABLE_NOREPEAT0( negativeSeven == -7 );
	GUARANTEE_RECOVERABLE_NOREPEAT0( oneF == 1.f );
	GUARANTEE_RECOVERABLE_NOREPEAT0( pi == 3.1415926535897932384626433832795 );
	GUARANTEE_RECOVERABLE_NOREPEAT0( helloString == "Hello" );
	GUARANTEE_RECOVERABLE_NOREPEAT0( isThisThingOnString == "Is this thing on?" );
	GUARANTEE_RECOVERABLE_NOREPEAT0( rustColor == Rgba( 200, 100, 50, 255 ) );
	GUARANTEE_RECOVERABLE_NOREPEAT0( eight == 8 );
	GUARANTEE_RECOVERABLE_NOREPEAT0( seashellColor == Rgba( 238,221,204 ) );
	GUARANTEE_RECOVERABLE_NOREPEAT0( nine == 9 );
	GUARANTEE_RECOVERABLE_NOREPEAT0( highDefRes == IntVec2( 1920, 1080 ) );
	GUARANTEE_RECOVERABLE_NOREPEAT0( normal2D == Vec2( -0.6f, 0.8f ) );
	GUARANTEE_RECOVERABLE_NOREPEAT0( vertex.m_position == Vec3( 3.f, 4.f, 5.f ) );
	GUARANTEE_RECOVERABLE_NOREPEAT0( vertex.m_color == Rgba( 100,101,102,103 ) );
	GUARANTEE_RECOVERABLE_NOREPEAT0( vertex.m_texCoords == Vec2( 0.125f, 0.625f ) );
}


//-----------------------------------------------------------------------------------------------
bool Command_TestBinaryFileLoad( NamedProperties& args )
{
	const char* testFilePath = "Save/Test.binary";
	g_console->Printf( CONSOLE_COLOR_INFORMATION_MAJOR, "Loading test binary file '%s'...\n", testFilePath );

	// Load from disk
	Buffer buf;
	bool success = LoadBinaryFileToExistingBuffer( testFilePath, buf );
	if( !success )
	{
		g_console->Printf( CONSOLE_COLOR_ERROR_MAJOR, "FAILED to load file %s\n", testFilePath );
		return false;
	}

	// Parse and verify - note that the test data is in the file TWICE; first as little-endian, then again as big
	BufferParser bufParse( buf );
	GUARANTEE_RECOVERABLE_NOREPEAT0( bufParse.GetRemainingSize() == TEST_BUF_SIZE );
	ParseTestFileBufferData( bufParse, eBufferEndian::LITTLE );
	ParseTestFileBufferData( bufParse, eBufferEndian::BIG );
	GUARANTEE_RECOVERABLE_NOREPEAT0( bufParse.GetRemainingSize() == 0 );

	g_console->Printf( CONSOLE_COLOR_INFORMATION_MINOR, "...successfully read file %s\n", testFilePath );
	return false;
}


//-----------------------------------------------------------------------------------------------
bool Command_TestBinaryFileSave( NamedProperties& args )
{
	const char* testFilePath = "Save/Test.binary";
	g_console->Printf( CONSOLE_COLOR_INFORMATION_MAJOR, "Saving test binary file '%s'...\n", testFilePath );

	// Create the test file buffer
	Buffer buf;
	buf.reserve( 1000 );
	BufferWriter bufWrite( buf );
	GUARANTEE_RECOVERABLE_NOREPEAT0( bufWrite.GetTotalSize() == 0 );
	GUARANTEE_RECOVERABLE_NOREPEAT0( bufWrite.GetAppendedSize() == 0 );
	// Push the test data into the file TWICE; first as little-endian, then a second time after as big-endian
	AppendTestFileBufferData( bufWrite, eBufferEndian::LITTLE );
	AppendTestFileBufferData( bufWrite, eBufferEndian::BIG );
	GUARANTEE_RECOVERABLE_NOREPEAT0( bufWrite.GetAppendedSize() == TEST_BUF_SIZE );
	GUARANTEE_RECOVERABLE_NOREPEAT0( bufWrite.GetTotalSize() == TEST_BUF_SIZE );

	// Write to disk
	bool success = SaveBinaryFileFromBuffer( testFilePath, buf );
	if( success )
	{
		g_console->Printf( CONSOLE_COLOR_INFORMATION_MINOR, "...successfully wrote file %s\n", testFilePath );
	}
	else
	{
		g_console->Printf( CONSOLE_COLOR_ERROR_MAJOR, "FAILED to write file %s\n", testFilePath );
		g_console->Printf( CONSOLE_COLOR_ERROR_MINOR, "(does the folder exist?)\n" );
	}

	return false;
}

