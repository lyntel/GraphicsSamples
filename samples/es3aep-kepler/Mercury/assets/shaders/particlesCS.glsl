//----------------------------------------------------------------------------------
// File:        es3aep-kepler\Mercury\assets\shaders/particlesCS.glsl
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
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

precision highp sampler3D;

#UNIFORMS

uniform float timeStep;

layout( std140, binding=1 ) buffer Pos {
    vec4 pos[];
};

layout( std140, binding=2 ) buffer Vel {
    vec4 vel[];
};

layout(local_size_x = WORK_GROUP_SIZE,  local_size_y = 1, local_size_z = 1) in;


// compute shader to update particles
void main() {
    uint i = gl_GlobalInvocationID.x;

    // thread block size may not be exact multiple of number of particles
    if (i >= numParticles) return;

    // read particle position and velocity from buffers
    vec3 p_old = pos[i].xyz;
    vec3 v_old = vel[i].xyz;

    float density = 0.0;
    const float atomradius = 2.5;
    const float norm_smoothing_norm = 1.5667;

    // Gravity in this case is toward the origin
    
    vec3 gravity_direction = -normalize(p_old.xyz);
    float gravity_magnitude = length(p_old.xyz);
    // For each fluid atom
    // TODO: Currently density is being calculated based on contributions from all particles in the scene
    //       this can be very expensive. Switch to using only contributions from neighboring particles.
    vec3 velocity = vec3(0.0,0.0,0.0);

    // Intentionally only taking contributions from only a few particles to create a more interesting effect
    for (uint j = 0U; j < uint(numParticles); j++)
    {
        if (j != i) {
            vec3 r = p_old - pos[j].xyz;
            velocity += r;
            density += norm_smoothing_norm * pow((atomradius - length(r) * length(r)),3.0);
        }        
    }

    const float gas_constant = 0.0041;

    // Calculate force on each particle
    float force =  gas_constant * density;

    float fTimeStep = timeStep;


    // Calculate new velocity
    //vec3 v_new = v_old + fTimeStep * force * velocity/length(velocity) ;
    float feedback_constant = 0.0001;
    //vec3 v_new = v_old + fTimeStep * force * p_old /length (p_old);
    vec3 v_new = v_old + fTimeStep * force * velocity/length(velocity) +  fTimeStep * gravity_magnitude * gravity_direction;
    vec3 p_new = p_old + fTimeStep * v_new;


    // write new values
    pos[i] = vec4(p_new, 1.0);
    vel[i] = vec4(v_new, 0.0);
}
