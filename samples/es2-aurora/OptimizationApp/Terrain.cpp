//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp/Terrain.cpp
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
#include "Terrain.h"
#include "NV/NvLogs.h"
#include "NvImage/NvImage.h"
#include "NvGLUtils/NvImageGL.h"

typedef uint16_t ushort;

ushort MortonTable256[] = 
{
  0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015, 
  0x0040, 0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055, 
  0x0100, 0x0101, 0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115, 
  0x0140, 0x0141, 0x0144, 0x0145, 0x0150, 0x0151, 0x0154, 0x0155, 
  0x0400, 0x0401, 0x0404, 0x0405, 0x0410, 0x0411, 0x0414, 0x0415, 
  0x0440, 0x0441, 0x0444, 0x0445, 0x0450, 0x0451, 0x0454, 0x0455, 
  0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511, 0x0514, 0x0515, 
  0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554, 0x0555, 
  0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015, 
  0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055, 
  0x1100, 0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115, 
  0x1140, 0x1141, 0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155, 
  0x1400, 0x1401, 0x1404, 0x1405, 0x1410, 0x1411, 0x1414, 0x1415, 
  0x1440, 0x1441, 0x1444, 0x1445, 0x1450, 0x1451, 0x1454, 0x1455, 
  0x1500, 0x1501, 0x1504, 0x1505, 0x1510, 0x1511, 0x1514, 0x1515, 
  0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551, 0x1554, 0x1555, 
  0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014, 0x4015, 
  0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055, 
  0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115, 
  0x4140, 0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155, 
  0x4400, 0x4401, 0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415, 
  0x4440, 0x4441, 0x4444, 0x4445, 0x4450, 0x4451, 0x4454, 0x4455, 
  0x4500, 0x4501, 0x4504, 0x4505, 0x4510, 0x4511, 0x4514, 0x4515, 
  0x4540, 0x4541, 0x4544, 0x4545, 0x4550, 0x4551, 0x4554, 0x4555, 
  0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011, 0x5014, 0x5015, 
  0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054, 0x5055, 
  0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115, 
  0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155, 
  0x5400, 0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415, 
  0x5440, 0x5441, 0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455, 
  0x5500, 0x5501, 0x5504, 0x5505, 0x5510, 0x5511, 0x5514, 0x5515, 
  0x5540, 0x5541, 0x5544, 0x5545, 0x5550, 0x5551, 0x5554, 0x5555
};

inline uint32_t ZIndex(ushort x, ushort y)
{
  return    MortonTable256[y >> 8]   << 17 | 
          MortonTable256[x >> 8]   << 16 |
          MortonTable256[y & 0xFF] <<  1 | 
          MortonTable256[x & 0xFF];
}

struct HeightSampler
{
  HeightSampler(int32_t sizeX, int32_t sizeY, const float* a_data) : w(float(sizeX)), h(float(sizeY)), data(a_data) {} 

  static int32_t myclamp (int32_t x, int32_t minVal, int32_t maxVal)
  {
    if(x < minVal) return minVal;
    if(x > maxVal) return maxVal;
    return x;
  }

  float texture2D(float x, float y) 
  {
    float u = x*(w-1.0f);
    float v = y*(h-1.0f);

    int32_t ui = (int32_t)myclamp(int32_t(u), 0, int32_t(w-1));
    int32_t vi = (int32_t)myclamp(int32_t(v), 0, int32_t(h-1));

    int32_t ui1 = (ui+1 >= w) ? ui : ui+1;
    int32_t vi1 = (vi+1 >= h) ? vi : vi+1;

    int32_t iw = int32_t(w);

    float du = u - float(ui);
    float dv = v - float(vi);

    return data[vi*iw + ui]*(1.0f-du)*(1.0f-dv) + data[vi*iw + ui1]*du*(1.0f-dv) + data[vi1*iw + ui1]*du*dv + data[vi1*iw + ui]*(1.0f-du)*dv;
  }

  float w;
  float h;
  const float* data;
};


bool Terrain::CheckInputSizeForZCurve(int32_t a_x, int32_t a_y)
{ 
  int32_t mult = 1;

  bool iterate = true;
  while(iterate)
  {
    if(a_x%mult != 0 || a_y%mult != 0)
      return false;

    if(a_x/mult <= 1 && a_y%mult <= 0)
      return true;

    mult *= 2;
  }

  return false;
}


Terrain::Terrain(const TerrainInput& a_input)
{
    NvImage* heightImage = NvImage::CreateFromDDSFile(a_input.heightmap.c_str());

    if(!heightImage)
        return;

    width  = heightImage->getWidth();
    height = heightImage->getHeight(); 

    // read heightmap from file
    int32_t size = width*height;
    float* heights = new float [size * sizeof(float)];

    const uint8_t *src = (const uint8_t*)heightImage->getLevel(0);
 
    for(int32_t x=0;x<width;x++)
        for(int32_t y=0;y<height;y++)
            heights[y*width+x] = float(src[width*y + x])/255.0f; 

    delete heightImage;

    HeightSampler heightSampler(width, height, heights);

    // create terrain geometry
    nv::matrix4f normalmatrix;
    normalmatrix = a_input.transform;
    normalmatrix.set_row(3, nv::vec4f(0,0,0,1)); // seems that nv::matrix store transpased mmatrix
    normalmatrix = transpose(inverse(normalmatrix));

    int32_t subdivsX = a_input.subdivsX - 1;
    int32_t subdivsY = a_input.subdivsY - 1;

    float minX = -0.5f;
    float minY = -0.5f;

    float maxX = +0.5f;
    float maxY = +0.5f;

    float tMinX = 0.0f;
    float tMaxX = 1.0f;

    float tMinY = 0.0f;
    float tMaxY = 1.0f;

    VPosNormTex* vertices = new VPosNormTex[subdivsX*subdivsY*4];
    int32_t* indices          = new int32_t[subdivsX*subdivsY*6]; 

    float halfX = 0.5f/float(subdivsX);
    float halfY = 0.5f/float(subdivsY);

    int32_t* indexCache = new int32_t [(subdivsX+1)*(subdivsY+1)];
    for(int32_t i=0;i<(subdivsX+1)*(subdivsY+1);i++)
        indexCache[i] = -1;

    bool sizeForZCurveIsOk = CheckInputSizeForZCurve(subdivsX+1, subdivsY+1);

    int32_t indOffset  = 0;

    for(int32_t y=0;y<subdivsY;y++)
    {
        float ty0 = float(y)/float(subdivsY);
        float ty1 = float(y+1)/float(subdivsY); 

        for(int32_t x=0;x<subdivsX;x++)
        {
            float tx0 = float(x)/float(subdivsX);
            float tx1 = float(x+1)/float(subdivsX);

            VPosNormTex quad[4];

            quad[0].pos.x = minX  + tx0*(maxX-minX);
            quad[0].pos.y = heightSampler.texture2D(tx0, ty1); 
            quad[0].pos.z = minY  + ty1*(maxY-minY);
            quad[0].t.x   = tMinX + tx0*(tMaxX-tMinX);
            quad[0].t.y   = tMinY + ty1*(tMaxY-tMinY);

            quad[1].pos.x = minX + tx0*(maxX-minX);
            quad[1].pos.y = heightSampler.texture2D(tx0, ty0);  
            quad[1].pos.z = minY + ty0*(maxY-minY);
            quad[1].t.x   = tMinX + tx0*(tMaxX-tMinX);
            quad[1].t.y   = tMinY + ty0*(tMaxY-tMinY);

            quad[2].pos.x = minX + tx1*(maxX-minX);
            quad[2].pos.y = heightSampler.texture2D(tx1, ty1); 
            quad[2].pos.z = minY + ty1*(maxY-minY);
            quad[2].t.x   = tMinX + tx1*(tMaxX-tMinX);
            quad[2].t.y   = tMinY + ty1*(tMaxY-tMinY);

            quad[3].pos.x = minX + tx1*(maxX-minX);
            quad[3].pos.y = heightSampler.texture2D(tx1, ty0); 
            quad[3].pos.z = minY + ty0*(maxY-minY);
            quad[3].t.x   = tMinX + tx1*(tMaxX-tMinX);
            quad[3].t.y   = tMinY + ty0*(tMaxY-tMinY);
      
            for(int32_t i=0;i<4;i++)
            {
                float dyx = heightSampler.texture2D(quad[i].t.x + halfX, quad[i].t.y) - heightSampler.texture2D(quad[i].t.x-halfX, quad[i].t.y); 
                float dyz = heightSampler.texture2D(quad[i].t.x, quad[i].t.y+halfY)   - heightSampler.texture2D(quad[i].t.x, quad[i].t.y - halfY); 

                nv::vec3f vx = nv::vec3f(1, dyx, 0.0f);
                nv::vec3f vz = nv::vec3f(0.0f, -dyz, -1);
                nv::vec3f v  = normalize(cross(vx, vz)); 

                quad[i].norm = normalize(cross(vx, vz));
            }

            for(int32_t i=0;i<4;i++)
            {
                quad[i].pos  = a_input.transform*quad[i].pos;
                quad[i].norm = normalize(normalmatrix*quad[i].norm);
            }

            int32_t quadIndex[4] = { (y+1)*(subdivsX+1) + x, y*(subdivsX+1) + x, (y+1)*(subdivsX+1) + x + 1, y*(subdivsX+1) + x + 1 }; // use ZCurve if possible
            if(sizeForZCurveIsOk)
            {
                quadIndex[0] = ZIndex(ushort(x),ushort(y+1));
                quadIndex[1] = ZIndex(ushort(x),ushort(y));
                quadIndex[2] = ZIndex(ushort(x+1), ushort(y+1));
                quadIndex[3] = ZIndex(ushort(x+1), ushort(y));
            }

            for(int32_t i=0;i<4;i++)
            {
                int32_t offset = quadIndex[i];

                if(indexCache[offset] == -1)
                {
                    vertices  [offset] = quad[i];
                    indexCache[offset] = offset;
                }
            }
      

            indices[indOffset+0] = indexCache[quadIndex[2]];
            indices[indOffset+1] = indexCache[quadIndex[1]];
            indices[indOffset+2] = indexCache[quadIndex[0]];

            indices[indOffset+3] = indexCache[quadIndex[3]];
            indices[indOffset+4] = indexCache[quadIndex[1]];
            indices[indOffset+5] = indexCache[quadIndex[2]];

            indOffset += 6;
        }
    }

    m_vertNum    = (subdivsX+1)*(subdivsY+1);
    m_indexCount = indOffset;
 
    glGenBuffers(1, &m_quadsBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadsBuffer);                                            CHECK_GL_ERROR();
    glBufferData(GL_ARRAY_BUFFER, m_vertNum*sizeof(VPosNormTex), vertices, GL_STATIC_DRAW);  CHECK_GL_ERROR();

    glGenBuffers(1, &m_indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);                                   CHECK_GL_ERROR();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indOffset*sizeof(int32_t), indices, GL_STATIC_DRAW);  CHECK_GL_ERROR();
  
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    delete [] indexCache; indexCache = NULL;
    delete [] vertices;   vertices   = NULL;
    delete [] indices;    indices    = NULL;
    delete [] heights;    heights    = NULL;

    NvImage::VerticalFlip(false);
    m_colorTex = NvImageGL::UploadTextureFromDDSFile(a_input.colormap.c_str());
    NvImage::VerticalFlip(true);
}

Terrain::~Terrain()
{
  glDeleteBuffers(1, &m_quadsBuffer); 
  glDeleteTextures(1, &m_colorTex);
}

#define OFFSET(n) ((char *)NULL + (n))

void Terrain::Draw(GLint vPositionHandle, GLint vNormalHandle, GLint vTexCoordHandle)
{
  glBindBuffer(GL_ARRAY_BUFFER, m_quadsBuffer);  CHECK_GL_ERROR();

  glVertexAttribPointer(vPositionHandle, 3, GL_FLOAT, GL_FALSE, sizeof(VPosNormTex), OFFSET(0)); 

  if (vNormalHandle >= 0) {
    glVertexAttribPointer(vNormalHandle,   3, GL_FLOAT, GL_FALSE, sizeof(VPosNormTex), OFFSET(1*sizeof(nv::vec3f))); 
    glEnableVertexAttribArray(vNormalHandle);   
  }
  if (vTexCoordHandle >= 0) {
      glVertexAttribPointer(vTexCoordHandle, 2, GL_FLOAT, GL_FALSE, sizeof(VPosNormTex), OFFSET(2*sizeof(nv::vec3f))); 
    glEnableVertexAttribArray(vTexCoordHandle);  
  }

  glEnableVertexAttribArray(vPositionHandle);  

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);

  glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0); CHECK_GL_ERROR();

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
