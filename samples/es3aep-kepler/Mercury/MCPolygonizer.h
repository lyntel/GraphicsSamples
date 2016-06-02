//----------------------------------------------------------------------------------
// File:        es3aep-kepler\Mercury/MCPolygonizer.h
// SDK Version: v3.00 
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014-2015, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------
#ifndef MCPOLYGONIZER_H
#define MCPOLYGONIZER_H

#include "NV/NvMath.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "ShaderBuffer.h"
#include <vector>

using namespace nv;
using std::vector;

struct Triangle {
	Triangle() {
		p[0] = vec4f(-1.0f, -1.0f, -1.0f, -1.0f);
		p[1] = vec4f(-1.0f, -1.0f, -1.0f, -1.0f);
		p[2] = vec4f(-1.0f, -1.0f, -1.0f, -1.0f);
	}
	vec4f p[3];
};

class MCPolygonizer
{

public:
	


	MCPolygonizer(float boxSize, float cubeSize, const char* shaderPrefix);
	~MCPolygonizer();
	
	void generateMesh(ShaderBuffer<vec4f>* positions, ShaderBuffer<vec4f>* velocities, float timeDelta);
	GLuint getIndexBuffer() const { return m_indices->getBuffer(); }

	GLuint getGridCellPosBuffer() const { return m_pos->getBuffer(); }
	GLuint getPackedPosValBuffer() const { return m_packedPosVal->getBuffer(); }
	GLuint getTriTableBuffer() const { return m_triTable->getBuffer(); }
	GLuint getGridCellIsoValBuffer() const { return m_val->getBuffer(); }

	uint32_t getCellsDim() const { return m_cellsDim; }
	float getCellSize() const { return m_cellSize; }

	//TODO: Delete this
	ShaderBuffer<vec4f>* getCellPosBufferValues() const { return m_pos; }


private:
    MCPolygonizer();
    size_t m_cellsDim;			// No of cells per dimension
	float m_cellSize;
	vec4f m_startPos;


	GLuint m_programPipeline;
	GLuint m_updateProg;
	const char* m_shaderPrefix;

	// Positions and isovalues of vertices
	ShaderBuffer <vec4f>* m_pos;       // Gridcell positions. 8 per cell (8 verts)
	ShaderBuffer <vec4f>* m_packedPosVal;       // Gridcell positions. 8 per cell (8 verts)
	ShaderBuffer <float>* m_val;	   // Gridcell isovalues. 8 per cell

	ShaderBuffer<int>* m_triTable;
	
	ShaderBuffer <uint32_t> *m_indices;
};
#endif // MCPOLYGONIZER_H


