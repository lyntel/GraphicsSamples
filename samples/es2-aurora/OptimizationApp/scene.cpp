//----------------------------------------------------------------------------------
// File:        es2-aurora\OptimizationApp/scene.cpp
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

#include "scene.h"
#include <time.h>
#include "NV/NvLogs.h"
#include <string>

/*
This file contains a simple function to place objects in scene and assign materials to them. 
Nothing from this file should be reused in any projects. This was done just to quickly create the example scene.
You'd better store this info in external xml or other scene desctiption format.
*/

float rnd(float a, float b) { return ((b-a)*((float)rand()/RAND_MAX))+a; }

void CreateRandomObjectsOnLandScape(std::vector<MeshObj>& a_modelsArray, std::map<std::string, NvModelGL*>& a_modelStorage, std::map<std::string, GLuint>& a_texStorage, float treeHeight)
{
    bool ADD_TREES=true, ADD_TANKS=true, ADD_COWS=true;

    MeshObj myObj;
    nv::matrix4f modelMatrix;

    a_modelsArray.clear();

    // This seed is chosen at random to give a nice-looking distribution.
    srand(1);

    if (ADD_TREES)
    {
        nv::matrix4f translateMatrix;
        nv::matrix4f rotateMatrix;

        const int32_t TREES_NUMBERZ = 6;
        const int32_t TREES_NUMBERX = 6;
        const float TREES_DISTZ = 75.0f;
        const float TREES_DISTX = 75.0f;

        const float MAX_X = TREES_DISTX*TREES_NUMBERX;
        const float MAX_Z = TREES_DISTZ*TREES_NUMBERZ;

        // We place a few trees non-randomly to give nice interaction with the tank and cows.
        const int32_t N_NON_RANDOM = 4;
        const nv::vec3f nonRandomPositions[N_NON_RANDOM] = 
        {
            nv::vec3f( -65, 0, -30),        // For tank
            nv::vec3f(-160, 0,-120),        // For cow #0
            nv::vec3f(-105, 0, -25),        // For cow #1
            nv::vec3f( 120, 0,-225),        // For cow #2
        };

        for(int32_t i=0;i<TREES_NUMBERZ;i++)
        {
            for(int32_t j=0;j<TREES_NUMBERX;j++)
            {
                float x=0, z=0;

                if (a_modelsArray.size() < N_NON_RANDOM)
                {
                    x = nonRandomPositions[a_modelsArray.size()].x;
                    z = nonRandomPositions[a_modelsArray.size()].z;
                }
                else
                {
                    x = rnd(-MAX_X, MAX_X);
                    z = rnd(-MAX_Z, MAX_Z);
                }

                // palm
                //
                translateMatrix.set_translate(nv::vec3f(x, treeHeight + rnd(-20,0), z));
                nv::rotationY(rotateMatrix, rnd(0.0f,NV_PI));

                modelMatrix.make_identity();
                modelMatrix = translateMatrix*rotateMatrix; //.set_translate(nv::vec3f(x, 50.0, y));

                myObj.m_modelMatrix   = modelMatrix;
                myObj.m_color         = nv::vec4f(1.0f, 1.0f, 1.0f, 1.0f);
                myObj.m_specularValue = 0.0f;
                myObj.m_pModelData    = a_modelStorage["palm"];
                myObj.m_texId         = a_texStorage["palm"];
                myObj.m_cullFacing    = false;
                myObj.m_alphaTest     = true;

                if(myObj.m_pModelData!= NULL)
                    a_modelsArray.push_back(myObj);
                else
                    LOGE("object 'palm' not found in the model storage");

            }
        }

    }

    if (ADD_TANKS)
    {
        const int32_t TANKS_NUMBERZ = 1;
        const int32_t TANKS_NUMBERX = 1;

        const float COWS_DISTZ = 50.0f;
        const float COWS_DISTX = 100.0f;


        modelMatrix.make_identity();

        for(int32_t i=0;i<TANKS_NUMBERZ;i++)
        {
            for(int32_t j=0;j<TANKS_NUMBERX;j++)
            {
                modelMatrix.set_translate(nv::vec3f(COWS_DISTX*(2*j-TANKS_NUMBERX+1), 0.0, COWS_DISTZ*(2*i-TANKS_NUMBERZ+1)));

                myObj.m_modelMatrix   = modelMatrix;
                myObj.m_color         = nv::vec4f(0.75f, 0.75f, 0.75, 1.0f);
                myObj.m_specularValue = 1.0f;
                myObj.m_pModelData    = a_modelStorage["T34-85"];
                myObj.m_texId         = a_texStorage["white_dummy"];
                myObj.m_alphaTest     = false;

                if(myObj.m_pModelData!= NULL)
                    a_modelsArray.push_back(myObj);
                else
                    LOGE("object 'T34-85' not found in the model storage");
            }
        }
    }

    if (ADD_COWS)
    {
        nv::matrix4f translateMatrix;
        nv::matrix4f rotateMatrix;

        const int32_t COWS_NUMBERZ = 2;
        const int32_t COWS_NUMBERX = 2;

//        const float COWS_DISTZ = 50.0f;
//        const float COWS_DISTX = 100.0f;

        nv::vec3f cowsPos[2][2];

        cowsPos[0][0] = nv::vec3f(-90, 9,-90);
        cowsPos[0][1] = nv::vec3f(-35, 4, 50);
        cowsPos[1][0] = nv::vec3f(150, 15,-180);
        cowsPos[1][1] = nv::vec3f(50, 2, 100);

        modelMatrix.make_identity();

        for(int32_t i=0;i<COWS_NUMBERZ;i++)
        {
            for(int32_t j=0;j<COWS_NUMBERX;j++)
            {
                translateMatrix.set_translate(cowsPos[i][j]);
                nv::rotationY(rotateMatrix, rnd(0.0f,NV_PI));

                modelMatrix.make_identity();
                modelMatrix = translateMatrix*rotateMatrix; //.set_translate(nv::vec3f(x, 50.0, y));

                myObj.m_modelMatrix   = modelMatrix;
                myObj.m_color         = nv::vec4f(0.75f, 0.75f, 0.75, 1.0f);
                myObj.m_specularValue = 1.0f;
                myObj.m_pModelData    = a_modelStorage["cow"];
                myObj.m_texId         = a_texStorage["white_dummy"];
                myObj.m_alphaTest     = false;

                if(myObj.m_pModelData!= NULL)
                    a_modelsArray.push_back(myObj);
                else
                    LOGE("object 'cow' not found in the model storage");
            }
        }
    }
}


