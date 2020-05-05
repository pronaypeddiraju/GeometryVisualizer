#include "Game/SceneCooker.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Game/Game.hpp"

//------------------------------------------------------------------------------------------------------------------------------
SceneCooker::SceneCooker(std::string& loadPath)
{
	m_loadPath = loadPath;
}

SceneCooker::SceneCooker(Game* game, std::string& loadPath, std::string& savePath)
{
	m_gameReference = game;

	m_loadPath = loadPath;
	m_savePath = savePath;
}

//------------------------------------------------------------------------------------------------------------------------------
SceneCooker::SceneCooker()
{

}

//------------------------------------------------------------------------------------------------------------------------------
SceneCooker::~SceneCooker()
{
	delete m_readUtils;
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::SetLoadPath(std::string& loadPath)
{
	m_loadPath = loadPath;
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::SetSavePath(std::string& savePath)
{
	m_savePath = savePath;
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::SetEndianNess(eBufferEndianness endianMode)
{
	m_endianNess = endianMode;
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::InitiateLoadingSequence()
{
	//Sequence responsible for loading and creating GHCS scene
	g_devConsole->PrintString(g_devConsole->CONSOLE_INFO, "Loading Binary file" + m_loadPath);

	//Get the test file in buffer
	m_readBuffer.reserve(1000);
	bool success = LoadBinaryFileToExistingBuffer(m_loadPath, m_readBuffer);
	if (!success)
	{
		g_devConsole->PrintString(g_devConsole->CONSOLE_ECHO_COLOR, "FAILED to load file" + m_loadPath);
		return;
	}

	m_readUtils = new BufferReadUtils(m_readBuffer);

	//Load Header
	LoadFileHeader();	//Except table of contents

	LoadTOC();
	
	LoadChunks();

	MakeGameGeometry();
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::InitiateWriteSequence()
{
	//Sequence to write GHCS file
	g_devConsole->PrintString(g_devConsole->CONSOLE_INFO, "Saving Binary file" + m_savePath);

	// Create the test file buffer
	m_writeBuffer.reserve(1000);
	m_writeUtils = new BufferWriteUtils(m_writeBuffer);
	
	//Write File Header and get location to write TOC
	byte TOCOffsetLocation = WriteFileHeader();	//How many bytes into file is the TOC

	ChunkInfo chunkToWrite;
	WriteConvexPolysChunk(chunkToWrite);

	byte convexHullChunkLocation = (byte)m_writeUtils->GetTotalSize();
	WriteConvexHullsChunk(chunkToWrite);

	byte sceneInfoChunkLocation = (byte)m_writeUtils->GetTotalSize();
	WriteSceneInfoChunk(chunkToWrite);

	size_t TOCLocation = m_writeUtils->GetTotalSize();
	WriteTableOfContents();

	m_writeUtils->WriteUint32AtLocation((int)TOCOffsetLocation, (uint)TOCLocation);

	bool success = SaveBinaryFileFromBuffer(m_savePath, m_writeBuffer);
	if (success)
	{
		g_devConsole->PrintString(g_devConsole->CONSOLE_INFO, "...successfully wrote file" + m_savePath);
	}
	else
	{
		g_devConsole->PrintString(g_devConsole->CONSOLE_ECHO_COLOR, "FAILED to write file " + m_savePath);
		g_devConsole->PrintString(g_devConsole->CONSOLE_ECHO_COLOR, "(does the folder exist?)\n");
	}
}

//------------------------------------------------------------------------------------------------------------------------------
byte SceneCooker::WriteFileHeader()
{
	//FourCC
	m_writeUtils->AppendByte('G');
	m_writeUtils->AppendByte('H');
	m_writeUtils->AppendByte('C');
	m_writeUtils->AppendByte('S');

	//Reserved 0 byte
	m_writeUtils->AppendByte(0);

	//Major File Version
	m_writeUtils->AppendByte(m_majorVersion);

	//Minor File version
	m_writeUtils->AppendByte(m_minorVersion);

	//Endian Mode
	if (m_writeUtils->IsEndianModeBig())
	{
		m_writeUtils->AppendByte(2);
	}
	else
	{
		m_writeUtils->AppendByte(1);
	}

	//TOC offset
	uchar seekLocation = m_writeUtils->GetTotalSize();
	//For now we are just going to write the Uint 1 at this location
	m_writeUtils->AppendUint32(1);

	return seekLocation;
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::WriteConvexPolysChunk(ChunkInfo& chunkToWrite)
{
	//Set the chunk to write
	chunkToWrite.m_type = 1;
	chunkToWrite.m_location = (uint)m_writeUtils->GetTotalSize();

	//force little endian ness for chunk
	m_writeUtils->SetEndianMode(BUFFER_LITTLE_ENDIAN);	//Endian mode 1

	//Write FourCC for chunk
	m_writeUtils->AppendByte('\0');
	m_writeUtils->AppendByte('C');
	m_writeUtils->AppendByte('H');
	m_writeUtils->AppendByte('K');

	//Chunk Type
	m_writeUtils->AppendByte(1);	//Chunk type convex polys

	//Chunk endianness
	m_writeUtils->AppendByte(1);	//Always little endian for chunk

	//Write the Convex Polys here
	std::vector<Geometry>& allGeometry = m_gameReference->GetAllGameGeometry();
	int numGeometry = allGeometry.size();

	//We need to write chunk size here
	uint chunkSizeWriteLocation = m_writeUtils->GetTotalSize();	//How many bytes ahead is we need to go to write chunk size
	uint dataSize = 0;	//For now we will write size 0 there
	m_writeUtils->AppendUint32(dataSize);

	m_writeUtils->AppendUint32((uint)numGeometry);

	ConvexPoly2D polyRef;
	//We need to write all convex poly properties now
	for (int polyIndex = 0; polyIndex < numGeometry; polyIndex++)
	{
		//Write num vertices first as short
		polyRef = allGeometry[polyIndex].GetConvexPoly2D();
		short numVerts = (short)polyRef.GetNumVertices();
		m_writeUtils->AppendShort(numVerts);
		dataSize += 2;	//added a short

		//Write each vertex position
		std::vector<Vec2> verts = polyRef.GetConvexPoly2DPoints();
		for (short vertIndex = 0; vertIndex < numVerts; vertIndex++)
		{
			m_writeUtils->AppendVec2(verts[vertIndex]);
			dataSize += 8;	//added Vec2
		}
	}

	chunkToWrite.m_dataSize = dataSize;
	m_writeUtils->WriteUint32AtLocation(chunkSizeWriteLocation, dataSize);

	m_chunksWritten.push_back(chunkToWrite);
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::WriteConvexHullsChunk(ChunkInfo& chunkToWrite)
{
	//Set the chunk to write
	chunkToWrite.m_type = 2;
	chunkToWrite.m_location = (uint)m_writeUtils->GetTotalSize();

	//Reset to endianness passed in
	m_writeUtils->SetEndianMode(m_endianNess);	//Set native mode

	//Write FourCC for chunk
	m_writeUtils->AppendByte('\0');
	m_writeUtils->AppendByte('C');
	m_writeUtils->AppendByte('H');
	m_writeUtils->AppendByte('K');

	//Chunk Type
	m_writeUtils->AppendByte(2);	//Chunk type convex hulls

	//Chunk endianness
	if (m_writeUtils->IsEndianModeBig())
	{
		m_writeUtils->AppendByte(2);
	}
	else
	{
		m_writeUtils->AppendByte(1);
	}

	//Write the Convex Polys here
	std::vector<Geometry>& allGeometry = m_gameReference->GetAllGameGeometry();
	int numGeometry = allGeometry.size();

	//We need to write chunk size here
	uint chunkSizeWriteLocation = m_writeUtils->GetTotalSize();	//How many bytes ahead is we need to go to write chunk size
	uint dataSize = 0;	//For now we will write size 0 there
	m_writeUtils->AppendUint32(dataSize);

	m_writeUtils->AppendUint32((uint)numGeometry);

	ConvexHull2D hullRef;
	//We need to write all convex poly properties now
	for (int hullIndex = 0; hullIndex < numGeometry; hullIndex++)
	{
		//Write num vertices first as short
		hullRef = allGeometry[hullIndex].GetConvexHull2D();
		short numPlanes = (short)hullRef.GetNumPlanes();
		m_writeUtils->AppendShort(numPlanes);
		dataSize += 2;	//added a short

		//Write each vertex position
		std::vector<Plane2D> planes = hullRef.GetPlanes();
		for (short planeIndex = 0; planeIndex < numPlanes; planeIndex++)
		{
			m_writeUtils->AppendVec2(planes[planeIndex].GetNormal());
			dataSize += 8;	//added Vec2

			m_writeUtils->AppendFloat(planes[planeIndex].GetSignedDistance());
			dataSize += 4; //Added float
		}
	}
	chunkToWrite.m_dataSize = dataSize;

	//Write the chunk size at correct location
	m_writeUtils->WriteUint32AtLocation(chunkSizeWriteLocation, dataSize);
	m_chunksWritten.push_back(chunkToWrite);
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::WriteTableOfContents()
{
	//Write FourCC for chunk
	m_writeUtils->AppendByte('\0');
	m_writeUtils->AppendByte('T');
	m_writeUtils->AppendByte('O');
	m_writeUtils->AppendByte('C');

	m_writeUtils->AppendByte((uchar)m_chunksWritten.size());

	for (int i = 0; i < m_chunksWritten.size(); i++)
	{
		m_writeUtils->AppendByte(m_chunksWritten[i].m_type);
		m_writeUtils->AppendUint32(m_chunksWritten[i].m_location);
		m_writeUtils->AppendUint32(m_chunksWritten[i].m_dataSize);
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::LoadFileHeader()
{
	//Set the endianness
	m_readUtils->SetEndianMode(m_endianNess);
	uchar fourCC0_G = m_readUtils->ParseChar(); 
	uchar fourCC1_H = m_readUtils->ParseChar(); 
	uchar fourCC2_C = m_readUtils->ParseChar(); 
	uchar fourCC3_S = m_readUtils->ParseChar(); 

	uchar reserved = m_readUtils->ParseChar();

	GUARANTEE_RECOVERABLE((fourCC0_G == 'G'), "Failed at 1");
	GUARANTEE_RECOVERABLE(fourCC1_H == 'H', "Failed at 2");
	GUARANTEE_RECOVERABLE(fourCC2_C == 'C', "Failed at 3");
	GUARANTEE_RECOVERABLE(fourCC3_S == 'S', "Failed at 4");

	uchar versionMajor = m_readUtils->ParseByte(); // version Major = 1
	uchar versionMinor = m_readUtils->ParseByte(); // version Minor = 0

	GUARANTEE_RECOVERABLE(versionMajor == m_majorVersion, "Major version mismatch");
	GUARANTEE_RECOVERABLE(versionMinor == m_minorVersion, "Minor version mismatch");

	uchar endianMode = m_readUtils->ParseByte();
	if (endianMode == 1)
	{
		m_endianNess = BUFFER_LITTLE_ENDIAN;
	}
	else if(endianMode == 2)
	{
		m_endianNess = BUFFER_BIG_ENDIAN;
	}
	else
	{
		m_endianNess = BUFFER_NATIVE;
	}

	m_readTOCLocation = m_readUtils->ParseUint32();
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::LoadTOC()
{
	int oldReadLocation = m_readUtils->GetTotalSize() - m_readUtils->GetRemainingSize();
	m_readUtils->SetReadLocation(m_readTOCLocation);

	//Check FourCC
	uchar fourCC0 = m_readUtils->ParseChar();
	uchar fourCC1 = m_readUtils->ParseChar();
	uchar fourCC2 = m_readUtils->ParseChar();
	uchar fourCC3 = m_readUtils->ParseChar();

	GUARANTEE_RECOVERABLE((fourCC0 == '\0'), "Failed at 1");
	GUARANTEE_RECOVERABLE(fourCC1 == 'T', "Failed at 2");
	GUARANTEE_RECOVERABLE(fourCC2 == 'O', "Failed at 3");
	GUARANTEE_RECOVERABLE(fourCC3 == 'C', "Failed at 4");

	uchar numChunks = m_readUtils->ParseChar();

	for (uchar chunkIndex = 0; chunkIndex < numChunks; chunkIndex++)
	{
		ChunkInfo chunk;
		chunk.m_type = m_readUtils->ParseByte();
		chunk.m_location = m_readUtils->ParseUint32();
		chunk.m_dataSize = m_readUtils->ParseUint32();
		m_chunksRead.push_back(chunk);
	}

	m_readUtils->SetReadLocation(oldReadLocation);
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::LoadChunks()
{
	//Parse each chunk in m_chunksRead and setup data on Game
	for (int chunkIndex = 0; chunkIndex < m_chunksRead.size(); chunkIndex++)
	{
		switch (m_chunksRead[chunkIndex].m_type)
		{
		case 0:
		{
			LoadSceneInfoChunk(m_chunksRead[chunkIndex]);
			break;
		}
		case 1:
		{
			LoadConvexPolysChunk(m_chunksRead[chunkIndex]);
			break;
		}
		case 2:
		{
			LoadConvexHullsChunk(m_chunksRead[chunkIndex]);
			break;
		}
		case 3:
		{

		}
		default:
		{
			ERROR_RECOVERABLE("Chunk type unsupported");
		}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::LoadConvexPolysChunk(ChunkInfo& chunkInfo)
{
	//Read this chunk by going to location
	m_readUtils->SetReadLocation(chunkInfo.m_location);
	m_readUtils->SetEndianMode(eBufferEndianness::BUFFER_LITTLE_ENDIAN);

	//Check FourCC
	uchar fourCC0 = m_readUtils->ParseChar();
	uchar fourCC1 = m_readUtils->ParseChar();
	uchar fourCC2 = m_readUtils->ParseChar();
	uchar fourCC3 = m_readUtils->ParseChar();

	GUARANTEE_RECOVERABLE((fourCC0 == '\0'), "Failed at 1");
	GUARANTEE_RECOVERABLE(fourCC1 == 'C', "Failed at 2");
	GUARANTEE_RECOVERABLE(fourCC2 == 'H', "Failed at 3");
	GUARANTEE_RECOVERABLE(fourCC3 == 'K', "Failed at 4");

	//Check Chunk type
	uchar chunkType = m_readUtils->ParseByte();
	GUARANTEE_RECOVERABLE(chunkType == 1, "Chunk is not Convex Poly Chunk");

	//Check endianness
	uchar endianness = m_readUtils->ParseByte();
	GUARANTEE_RECOVERABLE(endianness == 1, "Not little indian for Convex Poly Chunk");

	//Data size chcek
	uint dataSize = m_readUtils->ParseUint32();
	uint numObjects = m_readUtils->ParseUint32();

	m_polys.clear();
	for (int i = 0; i < numObjects; i++)
	{
		short numVerts = m_readUtils->ParseShort();

		std::vector<Vec2> verts;
		for (int j = 0; j < numVerts; j++)
		{
			verts.emplace_back(m_readUtils->ParseVec2());
		}

		m_polys.emplace_back(verts);
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::LoadConvexHullsChunk(ChunkInfo& chunkInfo)
{
	//Read this chunk by going to location
	m_readUtils->SetReadLocation(chunkInfo.m_location);
	m_readUtils->SetEndianMode(m_endianNess);

	//Check FourCC
	uchar fourCC0 = m_readUtils->ParseChar();
	uchar fourCC1 = m_readUtils->ParseChar();
	uchar fourCC2 = m_readUtils->ParseChar();
	uchar fourCC3 = m_readUtils->ParseChar();

	GUARANTEE_RECOVERABLE((fourCC0 == '\0'), "Failed at 1");
	GUARANTEE_RECOVERABLE(fourCC1 == 'C', "Failed at 2");
	GUARANTEE_RECOVERABLE(fourCC2 == 'H', "Failed at 3");
	GUARANTEE_RECOVERABLE(fourCC3 == 'K', "Failed at 4");

	//Check Chunk type
	uchar chunkType = m_readUtils->ParseByte();
	GUARANTEE_RECOVERABLE(chunkType == 2, "Chunk is not Convex Hull Chunk");

	//Check endianness
	uchar endianness = m_readUtils->ParseByte();
	//GUARANTEE_RECOVERABLE(endianness == 1, "Not little indian for Convex Hull Chunk");

	//Data size chcek
	uint dataSize = m_readUtils->ParseUint32();
	uint numObjects = m_readUtils->ParseUint32();

	m_hulls.clear();
	for (int i = 0; i < numObjects; i++)
	{
		short numPlanes = m_readUtils->ParseShort();

		std::vector<Plane2D> planes;
		for (int j = 0; j < numPlanes; j++)
		{
			Vec2 normal = m_readUtils->ParseVec2();
			float distance = m_readUtils->ParseFloat();

			planes.emplace_back(normal, distance);
		}

		m_hulls.emplace_back(planes);
	}
	
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::LoadSceneInfoChunk(ChunkInfo& chunkInfo)
{
	//Read this chunk by going to location
	m_readUtils->SetReadLocation(chunkInfo.m_location);
	m_readUtils->SetEndianMode(BUFFER_LITTLE_ENDIAN);

	//Check FourCC
	uchar fourCC0 = m_readUtils->ParseChar();
	uchar fourCC1 = m_readUtils->ParseChar();
	uchar fourCC2 = m_readUtils->ParseChar();
	uchar fourCC3 = m_readUtils->ParseChar();

	GUARANTEE_RECOVERABLE((fourCC0 == '\0'), "Failed at 1");
	GUARANTEE_RECOVERABLE(fourCC1 == 'C', "Failed at 2");
	GUARANTEE_RECOVERABLE(fourCC2 == 'H', "Failed at 3");
	GUARANTEE_RECOVERABLE(fourCC3 == 'K', "Failed at 4");

	//Check Chunk type
	uchar chunkType = m_readUtils->ParseByte();
	GUARANTEE_RECOVERABLE(chunkType == 0, "Chunk is not SceneInfo Chunk");

	//Check endianness
	uchar endianness = m_readUtils->ParseByte();
	//GUARANTEE_RECOVERABLE(endianness == 1, "Not little indian for Convex Hull Chunk");

	//Data size chcek
	uint dataSize = m_readUtils->ParseUint32();
	
	float minX = m_readUtils->ParseFloat();
	float minY = m_readUtils->ParseFloat();
	float maxX = m_readUtils->ParseFloat();
	float maxY = m_readUtils->ParseFloat();

	m_gameReference->m_orthoMinsRead = Vec2(minX, minY);
	m_gameReference->m_orthoMaxsRead = Vec2(maxX, maxY);
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::MakeGameGeometry()
{
	std::vector<Geometry> geometry;

	for (int i = 0; i < m_polys.size(); i++)
	{
		geometry.emplace_back(m_polys[i], m_hulls[i]);
	}

	m_gameReference->SetAllGameGeometry(geometry);
	m_gameReference->m_loadedFromCookedData = true;
	m_gameReference->ui_numGeometry = m_polys.size();
}

//------------------------------------------------------------------------------------------------------------------------------
void SceneCooker::WriteSceneInfoChunk(ChunkInfo& chunkToWrite)
{
	//Set the chunk to write
	chunkToWrite.m_type = 0;
	chunkToWrite.m_location = (uint)m_writeUtils->GetTotalSize();

	//Reset to endianness passed in
	m_writeUtils->SetEndianMode(BUFFER_LITTLE_ENDIAN);	//Set native mode

	//Write FourCC for chunk
	m_writeUtils->AppendByte('\0');
	m_writeUtils->AppendByte('C');
	m_writeUtils->AppendByte('H');
	m_writeUtils->AppendByte('K');

	//Chunk Type
	m_writeUtils->AppendByte(0);

	//Chunk endianness
	m_writeUtils->AppendByte(1);

	//Chunk Data size
	m_writeUtils->AppendUint32(16);

	m_writeUtils->AppendFloat(0.f);
	m_writeUtils->AppendFloat(0.f);
	m_writeUtils->AppendFloat(WORLD_WIDTH);
	m_writeUtils->AppendFloat(WORLD_HEIGHT);
	
	chunkToWrite.m_dataSize = 16;
	m_chunksWritten.push_back(chunkToWrite);
}
