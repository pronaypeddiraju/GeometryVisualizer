#pragma once
#include "Engine/Core/BufferWriteUtils.hpp"
#include "Engine/Core/BufferReadUtils.hpp"

#include "Engine/Math/ConvexHull2D.hpp"
#include "Engine/Math/ConvexPoly2D.hpp"

#include <string>

//------------------------------------------------------------------------------------------------------------------------------
typedef uchar byte;
class Game;

struct ChunkInfo
{
	uchar	m_type = 0;
	uint	m_location = 0;
	uint	m_dataSize = 0;
};

//------------------------------------------------------------------------------------------------------------------------------
//Class responsible for loading and saving the scene
//------------------------------------------------------------------------------------------------------------------------------
class SceneCooker
{
public:
	SceneCooker();
	SceneCooker(Game* game, std::string& loadPath, std::string& savePath);
	SceneCooker(std::string& loadPath);
	~SceneCooker();

	void				SetLoadPath(std::string& loadPath);
	void				SetSavePath(std::string& savePath);
	void				SetEndianNess(eBufferEndianness endianMode);

	void				InitiateWriteSequence();
	void				InitiateLoadingSequence();
private:

	//Write methods
	byte				WriteFileHeader();
	void				WriteConvexPolysChunk(ChunkInfo& chunkToWrite);
	void				WriteConvexHullsChunk(ChunkInfo& chunkToWrite);
	void				WriteSceneInfoChunk(ChunkInfo& chunkToWrite);
	void				WriteTableOfContents();

	//Read methods
	void				LoadFileHeader();
	void				LoadTOC();
	void				LoadChunks();

	void				LoadConvexPolysChunk(ChunkInfo& chunkInfo);
	void				LoadConvexHullsChunk(ChunkInfo& chunkInfo);
	void				LoadSceneInfoChunk(ChunkInfo& chunkInfo);

	void				MakeGameGeometry();

private:
	std::string			m_loadPath = "";
	std::string			m_savePath = "";

	byte				m_majorVersion = 1;
	byte				m_minorVersion = 0;

	uint				m_readTOCLocation = 0;

	eBufferEndianness	m_endianNess = eBufferEndianness::BUFFER_NATIVE;
	
	BufferReadUtils*	m_readUtils = nullptr;
	Buffer				m_readBuffer;

	BufferWriteUtils*	m_writeUtils = nullptr;
	Buffer				m_writeBuffer;

	Game*				m_gameReference = nullptr;

	std::vector<ChunkInfo>	m_chunksRead;
	std::vector<ChunkInfo>	m_chunksWritten;

	//On read
	std::vector<ConvexPoly2D>			m_polys;
	std::vector<ConvexHull2D>			m_hulls;
};