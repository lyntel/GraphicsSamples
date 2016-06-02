//----------------------------------------------------------------------------------
// File:        es3aep-kepler\Mercury\assets\shaders/renderSurfGS.glsl
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
// Version ID added via C-code prefixing

#extension GL_EXT_shader_io_blocks : enable
#extension GL_EXT_gpu_shader4 : enable
#extension GL_EXT_geometry_shader : enable
#extension GL_OES_geometry_shader : enable

precision highp float;
#UNIFORMS


layout (triangles) in;
layout (triangle_strip) out;
layout (max_vertices = 15) out;


in block {
    vec4 pos;
    vec4 normal;
	vec4 color;
} In[];

out block {
    vec4 pos;
    vec4 normal;
    vec4 color;
} Out;

layout( binding=2 ) buffer TriTable {
    int triTable[];
};

const int edgeTable[256] = int[](
    0x0, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
    0x190, 0x99, 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
    0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
    0x230, 0x339, 0x33, 0x13a, 0x636, 0x73f, 0x435, 0x53c,
    0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
    0x3a0, 0x2a9, 0x1a3, 0xaa, 0x7a6, 0x6af, 0x5a5, 0x4ac,
    0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
    0x460, 0x569, 0x663, 0x76a, 0x66, 0x16f, 0x265, 0x36c,
    0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
    0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff, 0x3f5, 0x2fc,
    0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
    0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55, 0x15c,
    0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
    0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc,
    0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
    0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
    0xcc, 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
    0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
    0x15c, 0x55, 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
    0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
    0x2fc, 0x3f5, 0xff, 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
    0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
    0x36c, 0x265, 0x16f, 0x66, 0x76a, 0x663, 0x569, 0x460,
    0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
    0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa, 0x1a3, 0x2a9, 0x3a0,
    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
    0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33, 0x339, 0x230,
    0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
    0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99, 0x190,
    0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
    0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0 );    


vec4 VertexInterp(in vec4 p1, in vec4 p2, in float valp1, in float valp2)
{
    return (p1 + (-valp1 / (valp2 - valp1)) * (p2 - p1));
}

// Pos.xyz contains the position of the vertex of the cube
// Pos.w contains the isovalue for the respective vertex
void main() {

    Out.color = In[0].color;
    vec4 VertexList[12];

    // Primitive's first vertex stores the starting position. Rest are calculated using offset
    vec4 vertexStartPos = In[0].pos;
    
    vec4 pos0 = vec4(vertexStartPos.x, vertexStartPos.y, vertexStartPos.z, 1.0);    
    vec4 pos1 = vec4(vertexStartPos.x, vertexStartPos.y + cellsize, vertexStartPos.z, 1.0);
    vec4 pos2 = vec4(vertexStartPos.x + cellsize, vertexStartPos.y + cellsize, vertexStartPos.z, 1.0);
    vec4 pos3 = vec4(vertexStartPos.x + cellsize, vertexStartPos.y, vertexStartPos.z, 1.0);
    vec4 pos4 = vec4(vertexStartPos.x, vertexStartPos.y, vertexStartPos.z + cellsize, 1.0);
    vec4 pos5 = vec4(vertexStartPos.x, vertexStartPos.y + cellsize, vertexStartPos.z + cellsize, 1.0);
    vec4 pos6 = vec4(vertexStartPos.x + cellsize, vertexStartPos.y + cellsize, vertexStartPos.z + cellsize, 1.0);
    vec4 pos7 = vec4(vertexStartPos.x + cellsize, vertexStartPos.y, vertexStartPos.z + cellsize, 1.0);

    vec4 isoValuesPacked0 = In[1].pos;
    vec4 isoValuesPacked1 = In[2].pos;
    
    float val0 = isoValuesPacked0.x;
    float val1 = isoValuesPacked0.y;
    float val2 = isoValuesPacked0.z;
    float val3 = isoValuesPacked0.w;
    float val4 = isoValuesPacked1.x;
    float val5 = isoValuesPacked1.y;
    float val6 = isoValuesPacked1.z;
    float val7 = isoValuesPacked1.w;

    //Determine the index into the edge table which
    //tells us which vertices are inside of the surface
    int CubeIndex = 0;
    if (val0 < 0.0) 
        CubeIndex |= 0x1;
    if (val1 < 0.0) 
        CubeIndex |= 0x2;
    if (val2 < 0.0) 
        CubeIndex |= 0x4;
    if (val3 < 0.0) 
        CubeIndex |= 0x8;
    if (val4 < 0.0) 
        CubeIndex |= 0x10;
    if (val5 < 0.0) 
        CubeIndex |= 0x20;
    if (val6 < 0.0) 
        CubeIndex |= 0x40;
    if (val7 < 0.0) 
        CubeIndex |= 0x80;


    // //Cube is entirely in/out of the surface
    if (edgeTable[CubeIndex] == 0)
        return;   

    //Find the vertices where the surface intersects the cube
    if (bool(edgeTable[CubeIndex] & 1))
        VertexList[0] = VertexInterp(pos0, pos1, val0, val1);
    if (bool(edgeTable[CubeIndex] & 2))
        VertexList[1] = VertexInterp(pos1, pos2, val1, val2);
    if (bool(edgeTable[CubeIndex] & 4))
        VertexList[2] = VertexInterp(pos2, pos3, val2, val3);
    if (bool(edgeTable[CubeIndex] & 8))
        VertexList[3] = VertexInterp(pos3, pos0, val3, val0);
    if (bool(edgeTable[CubeIndex] & 16))
        VertexList[4] = VertexInterp(pos4, pos5, val4, val5);
    if (bool(edgeTable[CubeIndex] & 32))
        VertexList[5] = VertexInterp(pos5, pos6, val5, val6);
    if (bool(edgeTable[CubeIndex] & 64))
        VertexList[6] = VertexInterp(pos6, pos7, val6, val7);
    if (bool(edgeTable[CubeIndex] & 128))
        VertexList[7] = VertexInterp(pos7, pos4, val7, val4);
    if (bool(edgeTable[CubeIndex] & 256))
        VertexList[8] = VertexInterp(pos0, pos4, val0, val4);
    if (bool(edgeTable[CubeIndex] & 512))
        VertexList[9] = VertexInterp(pos1, pos5, val1, val5);
    if (bool(edgeTable[CubeIndex] & 1024))
        VertexList[10] = VertexInterp(pos2, pos6, val2, val6);
    if (bool(edgeTable[CubeIndex] & 2048))
        VertexList[11] = VertexInterp(pos3, pos7, val3, val7);    

    // Create the triangle 
    for (int i = 0; triTable[CubeIndex * 16 + i] != -1; i += 3) {
        vec4 v0 = VertexList[triTable[CubeIndex * 16 + i]];
        vec4 v1 = VertexList[triTable[CubeIndex * 16 + i + 1]];
        vec4 v2 = VertexList[triTable[CubeIndex * 16 + i + 2]];

        Out.normal = normalize(v0);
        Out.pos = v0;
        gl_Position = ProjectionMatrix * ModelView * Out.pos;
        EmitVertex();        
        Out.normal = normalize(v1);
        Out.pos = v1;
        gl_Position = ProjectionMatrix * ModelView * Out.pos;
        EmitVertex();        
        Out.normal = normalize(v2);
        Out.pos = v2;
        gl_Position = ProjectionMatrix * ModelView * Out.pos;
        EmitVertex();
        
        EndPrimitive();        
    }
}



