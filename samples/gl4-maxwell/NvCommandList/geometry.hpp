//----------------------------------------------------------------------------------
// File:        gl4-maxwell\NvCommandList/geometry.hpp
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

#ifndef NV_GEOMETRY_INCLUDED
#define NV_GEOMETRY_INCLUDED

#include <vector>
#include "NV/NvMatrix.h"
#include "NV/NvMath.h"

#define nv_pi  float(3.14159265358979323846264338327950288419716939937510582)

namespace geometry {
    struct Vertex
    {
      Vertex
        (
        nv::vec3f const & position,
        nv::vec3f const & normal,
        nv::vec2f const & texcoord
        ) :
          position( nv::vec4f(position,1.0f)),
          normal  ( nv::vec4f(normal,0.0f)),
          texcoord( nv::vec4f(texcoord,0.0f,0.0f))
      {}

      nv::vec4f position;
      nv::vec4f normal;
      nv::vec4f texcoord;
    };

    template <class TVertex>
    class Mesh {
    public:
      std::vector<TVertex>        m_vertices;
      std::vector<nv::vec3<unsigned int> >     m_indicesTriangles;
      std::vector<nv::vec2<unsigned int> >     m_indicesOutline;

      void append(Mesh<TVertex>& geo)
      {
        m_vertices.reserve(geo.m_vertices.size() + m_vertices.size());
        m_indicesTriangles.reserve(geo.m_indicesTriangles.size() + m_indicesTriangles.size());
        m_indicesOutline.reserve(geo.m_indicesOutline.size() + m_indicesOutline.size());

        unsigned int offset = (unsigned int)(m_vertices.size());

        for (size_t i = 0; i < geo.m_vertices.size(); i++){
          m_vertices.push_back(geo.m_vertices[i]);
        }

        for (size_t i = 0; i < geo.m_indicesTriangles.size(); i++){
          m_indicesTriangles.push_back(geo.m_indicesTriangles[i] + nv::vec3<unsigned int>(offset));
        }

        for (size_t i = 0; i < geo.m_indicesOutline.size(); i++){
          m_indicesOutline.push_back(geo.m_indicesOutline[i] +  nv::vec2<unsigned int>(offset));
        }
      }

      void flipWinding()
      {
        for (size_t i = 0; i < m_indicesTriangles.size(); i++){
          std::swap(m_indicesTriangles[i].x,m_indicesTriangles[i].z);
        }
      }

      size_t getTriangleIndicesSize() const{
        return m_indicesTriangles.size() * sizeof(nv::vec3<unsigned int>);
      }

      unsigned int getTriangleIndicesCount() const{
        return (unsigned int)m_indicesTriangles.size() * 3;
      }

      size_t getOutlineIndicesSize() const{
        return m_indicesOutline.size() * sizeof(nv::vec2<unsigned int>);
      }

      unsigned int getOutlineIndicesCount() const{
        return (unsigned int)m_indicesOutline.size() * 2;
      }

      size_t getVerticesSize() const{
        return m_vertices.size() * sizeof(TVertex);
      }

      unsigned int getVerticesCount() const{
        return (unsigned int)m_vertices.size();
      }
    };

    template <class TVertex>
    class Plane : public Mesh<TVertex> {
    public:
      static void add(Mesh<TVertex>& geo, const nv::matrix4f& mat, int w, int h)
      {
        int xdim = w;
        int ydim = h;

        float xmove = 1.0f/(float)xdim;
        float ymove = 1.0f/(float)ydim;

        int width = (xdim + 1);

        unsigned int vertOffset = (unsigned int)geo.m_vertices.size();

        int x,y;
        for (y = 0; y < ydim + 1; y++){
          for (x = 0; x < xdim + 1; x++){
            float xpos = ((float)x * xmove);
            float ypos = ((float)y * ymove);
            nv::vec3f pos;
            nv::vec2f uv;
            nv::vec3f normal;

            pos[0] = (xpos - 0.5f) * 2.0f;
            pos[1] = (ypos - 0.5f) * 2.0f;
            pos[2] = 0;

            uv[0] = xpos;
            uv[1] = ypos;

            normal[0] = 0.0f;
            normal[1] = 0.0f;
            normal[2] = 1.0f;

            Vertex vert = Vertex(pos,normal,uv);
            vert.position = mat * vert.position;
            vert.normal   = mat * vert.normal;
            geo.m_vertices.push_back(TVertex(vert));
          }
        }

        for (y = 0; y < ydim; y++){
          for (x = 0; x < xdim; x++){
            // upper tris
            geo.m_indicesTriangles.push_back(
              nv::vec3<unsigned int>(
              (x)      + (y + 1) * width + vertOffset,
              (x)      + (y)     * width + vertOffset,
              (x + 1)  + (y + 1) * width + vertOffset
              )
              );
            // lower tris
            geo.m_indicesTriangles.push_back(
              nv::vec3<unsigned int>(
              (x + 1)  + (y + 1) * width + vertOffset,
              (x)      + (y)     * width + vertOffset,
              (x + 1)  + (y)     * width + vertOffset
              )
              );
          }
        }

        for (y = 0; y < ydim; y++){
          geo.m_indicesOutline.push_back(
            nv::vec2<unsigned int>(
            (y)     * width + vertOffset,
            (y + 1) * width + vertOffset
            )
            );
        }
        for (y = 0; y < ydim; y++){
          geo.m_indicesOutline.push_back(
            nv::vec2<unsigned int>(
            (y)     * width + xdim + vertOffset,
            (y + 1) * width + xdim + vertOffset
            )
            );
        }
        for (x = 0; x < xdim; x++){
          geo.m_indicesOutline.push_back(
            nv::vec2<unsigned int>(
            (x)     + vertOffset,
            (x + 1) + vertOffset
            )
            );
        }
        for (x = 0; x < xdim; x++){
          geo.m_indicesOutline.push_back(
            nv::vec2<unsigned int>(
            (x)     + ydim * width + vertOffset,
            (x + 1) + ydim * width + vertOffset
            )
            );
        }
      }

      Plane (int segments = 1 ){
		nv::matrix4f mat;
		mat.make_identity();

        add(*this,mat,segments,segments);
      }
    };

    template <class TVertex>
    class Box : public Mesh<TVertex> {
    public:
      static void add(Mesh<TVertex>& geo, const nv::matrix4f& mat, int w, int h, int d)
      {
        int configs[6][2] = {
          {w,h},
          {w,h},

          {d,h},
          {d,h},

          {w,d},
          {w,d},
        };

        for (int side = 0; side < 6; side++){
          nv::matrix4f matrixRot;
		  matrixRot.make_identity();

          switch (side)
          {
          case 0:
            break;
          case 1:
            nv::rotationY(matrixRot, nv_pi);
            break;
          case 2:
            nv::rotationY(matrixRot, nv_pi * 0.5f);
            break;
          case 3:
            nv::rotationY(matrixRot, nv_pi * 1.5f);
            break;
          case 4:
            nv::rotationX(matrixRot, nv_pi * 0.5f);
            break;
          case 5:
            nv::rotationX(matrixRot, nv_pi * 1.5f);
            break;
          }

          nv::matrix4f matrixMove;
		  nv::translation(matrixMove, 0.0f, 0.0f, 1.0f);

          Plane<TVertex>::add(geo, mat * matrixRot * matrixMove,configs[side][0],configs[side][1]);
        }
      }

      Box (int segments = 1 ){
		nv::matrix4f mat;
		mat.make_identity();

        add(*this, mat /*nv::matrix4f(1)*/,segments,segments,segments);
      }
    };

    template <class TVertex>
    class Sphere : public Mesh<TVertex> {
    public:
      static void add(Mesh<TVertex>& geo, const nv::matrix4f& mat, int w, int h)
      {
        int xydim = w;
        int zdim  = h;

        unsigned int vertOffset = (unsigned int)geo.m_vertices.size();

        float xyshift = 1.0f / (float)xydim;
        float zshift  = 1.0f / (float)zdim;
        int width = xydim + 1;

        int xy,z;
        for (z = 0; z < zdim + 1; z++){
          for (xy = 0; xy < xydim + 1; xy++){
            nv::vec3f pos;
            nv::vec3f normal;
            nv::vec2f uv;
            float curxy = xyshift * (float)xy;
            float curz  = zshift  * (float)z;
            float anglexy = curxy * nv_pi * 2.0f;
            float anglez  = (1.0f-curz) * nv_pi;
            pos[0] = cosf(anglexy) * sinf(anglez);
            pos[1] = sinf(anglexy) * sinf(anglez);
            pos[2] = cosf(anglez);
            normal = pos;
            uv[0]  = curxy;
            uv[1]  = curz;

            Vertex vert = Vertex(pos,normal,uv);
            vert.position = mat * vert.position;
            vert.normal   = mat * vert.normal;

            geo.m_vertices.push_back(TVertex(vert));
          }
        }

        int vertex = 0;
        for (z = 0; z < zdim; z++){
          for (xy = 0; xy < xydim; xy++, vertex++){
            nv::vec3<unsigned int> indices;
            if (z != zdim-1){
              indices[2] = vertex + vertOffset;
              indices[1] = vertex + width + vertOffset;
              indices[0] = vertex + width + 1 + vertOffset;
              geo.m_indicesTriangles.push_back(indices);
            }

            if (z != 0){
              indices[2] = vertex + width + 1 + vertOffset;
              indices[1] = vertex + 1 + vertOffset;
              indices[0] = vertex + vertOffset;
              geo.m_indicesTriangles.push_back(indices);
            }

          }
          vertex++;
        }

        int middlez = zdim / 2;

        for (xy = 0; xy < xydim; xy++){
          nv::vec2<unsigned int> indices;
          indices[0] = middlez * width + xy + vertOffset;
          indices[1] = middlez * width + xy + 1 + vertOffset;
          geo.m_indicesOutline.push_back(indices);
        }

        for (int i = 0; i < 4; i++){
          int x = (xydim * i) / 4;
          for (z = 0; z < zdim; z++){
            nv::vec2<unsigned int> indices;
            indices[0] = x + width * (z) + vertOffset;
            indices[1] = x + width * (z + 1) + vertOffset;
            geo.m_indicesOutline.push_back(indices);
          }
        }
      }

      Sphere (int w=16, int h=8 ){
		nv::matrix4f mat;
		mat.make_identity();

        add(*this, mat,w,h);
      }
    };
 }



#endif

