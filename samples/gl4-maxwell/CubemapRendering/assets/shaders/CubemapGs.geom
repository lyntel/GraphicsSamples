//----------------------------------------------------------------------------------
// File:        gl4-maxwell\CubemapRendering\assets\shaders/CubemapGs.geom
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
#extension GL_EXT_geometry_shader4 : enable

#if USE_INSTANCING
layout(triangles, invocations = 6) in;
layout(triangle_strip, max_vertices = 3) out;
#else
layout(triangles, invocations = 1) in;
layout(triangle_strip, max_vertices = 18) out;
#endif

#define SWZ(SIGNX, X, SIGNY, Y, SIGNZ, Z, SIGNW, W)\
   newpos0.x = SIGNX pos0.X; newpos1.x = SIGNX pos1.X; newpos2.x = SIGNX pos2.X;\
   newpos0.y = SIGNY pos0.Y; newpos1.y = SIGNY pos1.Y; newpos2.y = SIGNY pos2.Y;\
   newpos0.z = SIGNZ pos0.Z; newpos1.z = SIGNZ pos1.Z; newpos2.z = SIGNZ pos2.Z;\
   newpos0.w = SIGNW pos0.W; newpos1.w = SIGNW pos1.W; newpos2.w = SIGNW pos2.W;
   

#define TSWZ(T, S, F) \
   T = ((control & (2 << F)) != 0) ? S.y : S.x; \
   T = ((control & (4 << F)) != 0) ? S.z : T; \
   if((control & (1 << F)) != 0) T = -T;

// Swizzle table: 0xWYX;
   // 0 => +X, 1 => -X
   // 2 => +Y, 3 => -Y
   // 4 => +Z, 5 => -Z
uniform uint table[] = {
	0x035, // out.W = in.X; out.Y = -in.Y; out.X = -in.Z.
	0x134, // so on following the same pattern and the key above
	0x240,
	0x350,
	0x430,
	0x531
};

in VertexData {
	vec3 normal;
} inData[];

#if USE_CULLING
in float planeMask[];
#endif

out VertexData {
	vec3 normal;
} outData;

void main()
{
#if USE_CULLING
	int pm0 = int(planeMask[0]);
	int pm1 = int(planeMask[1]);
	int pm2 = int(planeMask[2]);
	int prim_plane_mask_0 = pm0 & pm1 & pm2;
	int prim_plane_mask_1 = ~pm0 & ~pm1 & ~pm2;
	int combined_mask = prim_plane_mask_0 | (prim_plane_mask_1 << 16);

	int face_mask = 0;
	if((combined_mask & 0x00010f) == 0) face_mask |= 0x01;
	if((combined_mask & 0x0f0200) == 0) face_mask |= 0x02;
	if((combined_mask & 0x110422) == 0) face_mask |= 0x04;
	if((combined_mask & 0x220811) == 0) face_mask |= 0x08;
	if((combined_mask & 0x041038) == 0) face_mask |= 0x10;
	if((combined_mask & 0x382004) == 0) face_mask |= 0x20;
#endif

	vec4 pos0 = gl_PositionIn[0].xyzw;
	vec4 pos1 = gl_PositionIn[1].xyzw;
	vec4 pos2 = gl_PositionIn[2].xyzw;

#if USE_INSTANCING
	int face = gl_InvocationID;
#else
	for(int face = 0; face < 6; face++)
#endif
	{
		vec4 newpos0, newpos1, newpos2;
		gl_Layer = face;

#if USE_INSTANCING
		// Use bitwise and conditional ops to do swizzle instead of a switch operator - to avoid branches in the shader.
		// The only difference vs. the switch version is performance.

		uint control = table[face];

		TSWZ(newpos0.x, pos0, 0);
		TSWZ(newpos0.y, pos0, 4);
		TSWZ(newpos0.w, pos0, 8);
		newpos0.z = 1;

		TSWZ(newpos1.x, pos1, 0);
		TSWZ(newpos1.y, pos1, 4);
		TSWZ(newpos1.w, pos1, 8);
		newpos1.z = 1;

		TSWZ(newpos2.x, pos2, 0);
		TSWZ(newpos2.y, pos2, 4);
		TSWZ(newpos2.w, pos2, 8);
		newpos2.z = 1;
#else
		// The switch is used inside a fully unrolled loop over 'face', and only one case is used in each iteration,
		// so the switch operator is removed by the compiler.

		switch(face)
		{
			case 0:  SWZ(-,z,-,y, ,w, ,x); break;
			case 1:  SWZ( ,z,-,y, ,w,-,x); break;
			case 2:  SWZ( ,x, ,z, ,w, ,y); break;
			case 3:  SWZ( ,x,-,z, ,w,-,y); break;
			case 4:  SWZ( ,x,-,y, ,w, ,z); break;
			default: SWZ(-,x,-,y, ,w,-,z); break;
		}
#endif

#if USE_CULLING
		if(((face_mask >> face) & 1) != 0)
#endif
		{
			gl_Position = newpos0; 
			outData.normal = inData[0].normal;
			EmitVertex();

			gl_Position = newpos1; 
			outData.normal = inData[1].normal;
			EmitVertex();

			gl_Position = newpos2; 
			outData.normal = inData[2].normal;
			EmitVertex();

			EndPrimitive();
		}
	}
}
