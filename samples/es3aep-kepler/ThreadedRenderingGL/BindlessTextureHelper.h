//----------------------------------------------------------------------------------
// File:        es3aep-kepler\ThreadedRenderingGL/BindlessTextureHelper.h
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
#ifndef BindlessTextureHelper_H_
#define BindlessTextureHelper_H_

#include "NvAppBase/gl/NvAppContextGL.h"

namespace Nv
{
    /// Provide an interface to work with Bindless Textures regardless of which
    /// extension is providing the functionality.
    class BindlessTextureHelper
    {
    public:

        /// Enum indicating which version of Bindless Textures are being used
        enum BindlessMode
        {
            BINDLESS_NONE,
            BINDLESS_NV,
            BINDLESS_ARB
        };

        /// Initializes the helper class by querying for extension support 
        /// from the given context.
        /// \param pContext Pointer to the current GL context to query for support
        /// \return True if the context contains usable Bindless Texture support
        static bool init(NvAppContextGL* pContext)
        {
            // Determine Bindless Texture support.  This is not part of any core implementation,
            // so we have to check for an appropriate extension.
            if (pContext->isExtensionSupported("GL_NV_bindless_texture"))
            {
                // The NVIDIA specific extension is available
                ms_bindlessMode = BINDLESS_NV;
                return true;
            }

            if (pContext->isExtensionSupported("GL_ARB_bindless_texture"))
            {
                // The more generic ARB extension is available
                ms_bindlessMode = BINDLESS_ARB;
                return true;
            }

            // Neither of the extensions that we support are available
            ms_bindlessMode = BINDLESS_NONE;
            return false;
        }

        /// Returns a Bindless Texture handle to the given GL
        /// named texture object.
        /// \param id The GL Name of the texture for which to retrieve
        ///           a handle
        /// \return The 64-bit handle that can be used as a texture or 
        ///         sampler in a shader to reference the given texture.
        static GLuint64 getTextureHandle(GLuint id)
        {
            // Choose the correct version of the glGetTextureHandle...() 
            // method, based on the supported extension
            switch (ms_bindlessMode)
            {
            case BINDLESS_NV:
                return glGetTextureHandleNV(id);
            case BINDLESS_ARB:
                return glGetTextureHandleARB(id);
            default:
                return 0;
            }
        }

        /// Makes the Bindless Texture associated with the given handle
        /// resident and available for use in rendering.
        /// \param handle The Bindless Handle of the texture to make
        ///               resident.
        static void makeTextureHandleResident(GLuint64 handle)
        {
            // Choose the correct version of the glMakeTextureResident...() 
            // method, based on the supported extension
            switch (ms_bindlessMode)
            {
            case BINDLESS_NV:
                glMakeTextureHandleResidentNV(handle);
                return;
            case BINDLESS_ARB:
                glMakeTextureHandleResidentARB(handle);
                return;
            default:
                return;
            }
        }

        /// Retrieves the mode indicating which extension is currently supported
        /// \return A member of the BindlessMode enum indicating which Bindless
        ///         Texture extension is available.
        static BindlessMode getBindlessMode()
        {
            return ms_bindlessMode;
        }
        
    private:
        /// Enum indicating which Bindless Texture extension is supported and
        /// currently in use.
        static BindlessMode ms_bindlessMode;

    };
}



#endif // BindlessTextureHelper_H_