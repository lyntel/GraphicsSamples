//----------------------------------------------------------------------------------
// File:        es3aep-kepler\HDR/ShaderDeclaration.h
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
#ifndef SHADER_DECLARATION_H_
#define SHADER_DECLARATION_H_


//////////////MatteObject///////////////
const char* atb_matteObject[] = {
	"PosAttribute", "myNormal", "uvTexCoord" };
const char* uni_matteObject[] = {
"viewProjMatrix", "ModelMatrix", "eyePos", "emission", "color"};
const char* spl_matteObject[] = {
"envMap","envMapIrrad","diffuseMap"};
const char* vtx_matteObject = "\
precision highp float;\
attribute vec3 PosAttribute;\
attribute vec3 myNormal;\
attribute vec2 uvTexCoord;\
uniform mat4 viewProjMatrix;\
uniform mat4 ModelMatrix;\
uniform vec3 eyePos;\
varying vec4 Position;\
varying vec3 Normal;\
varying vec3 IncidentVector;\
varying vec2 texcoord;\
void main()\
{\
   vec4 P = ModelMatrix * vec4(PosAttribute, 1.0);\
   vec3 N = normalize(mat3(ModelMatrix) * myNormal);\
   vec3 I = P.xyz - eyePos;\
   Position = P;\
   Normal = N;\
   IncidentVector = I;\
   texcoord = uvTexCoord;\
   gl_Position = viewProjMatrix * P;\
}";

const char* frg_matteObject = "\
precision highp float;\
varying vec4 Position;\
varying vec3 Normal;\
varying vec3 IncidentVector;\
varying vec2 texcoord;\
uniform vec3 emission;\
uniform vec4 color;\
uniform samplerCube envMap;\
uniform samplerCube envMapIrrad;\
uniform sampler2D diffuseMap;\
float my_fresnel(vec3 I, vec3 N, float power,  float scale,  float bias)\
{\
    return bias + (pow(clamp(1.0 - dot(I, N), 0.0, 1.0), power) * scale);\
}\
void main()\
{\
    vec3 I = normalize(IncidentVector);\
    vec3 N = normalize(Normal);\
    vec3 R = reflect(I, N);\
    float fresnel = my_fresnel(-I, N, 5.0, 1.0, 0.1);\
    vec3 Creflect = textureCube(envMap, R).rgb;\
	vec3 irrad = textureCube(envMapIrrad, N).rgb;\
	vec3 diffuse = texture2D(diffuseMap, texcoord).rgb * color.a + color.rgb;\
	gl_FragColor = vec4(mix(diffuse*irrad, Creflect, fresnel*color.a)+emission, 1.0);\
}";


//////////////refractObject///////////////
const char* atb_refractObject[] = {
	"PosAttribute", "myNormal" };
const char* uni_refractObject[] = {
"viewProjMatrix", "ModelMatrix", "eyePos", "emission", "color"};
const char* spl_refractObject[] = {
"envMap"};
const char* vtx_refractObject = "\
precision highp float;\
attribute vec3 PosAttribute;\
attribute vec3 myNormal;\
uniform mat4 viewProjMatrix;   \
uniform mat4 ModelMatrix;    \
uniform vec3 eyePos;\
varying vec4 Position;\
varying vec3 Normal;\
varying vec3 IncidentVector;\
void main()\
{\
   vec4 P = ModelMatrix * vec4(PosAttribute, 1.0);\
   vec3 N = normalize(mat3(ModelMatrix) * myNormal);\
   vec3 I = P.xyz - eyePos;\
   Position = P;\
   Normal = N;\
   IncidentVector = I;\
   gl_Position = viewProjMatrix * P;\
}";

const char* frg_refractObject = "\
precision highp float;\
varying vec4 Position;\
varying vec3 Normal;\
varying vec3 IncidentVector;\
uniform samplerCube envMap;\
uniform vec3 emission;\
uniform vec4 color;\
float eta=0.7;\
float deta=-0.006;\
float my_fresnel(vec3 I, vec3 N, float power,  float scale,  float bias)\
{\
    return bias + (pow(clamp(1.0 - dot(I, N), 0.0, 1.0), power) * scale);\
}\
void main()\
{\
    vec3 I = normalize(IncidentVector);\
    vec3 N = normalize(Normal);\
    vec3 R = reflect(I, N);\
    vec3 T1 = refract(I, N, eta);\
	vec3 T2 = refract(I, N, eta+deta);\
	vec3 T3 = refract(I, N, eta+2.0*deta);\
    float fresnel = my_fresnel(-I, N, 4.0, 0.99, 0.1);\
    vec3 Creflect = textureCube(envMap, R).rgb;\
	vec3 Crefract;\
    Crefract.r = textureCube(envMap, T1).r;\
	Crefract.g = textureCube(envMap, T2).g;\
	Crefract.b = textureCube(envMap, T3).b;\
    Crefract *= color.rgb;\
    gl_FragColor = vec4(mix(Crefract, Creflect, fresnel)+emission, 1.0);\
}";

//////////////reflectObject///////////////
const char* atb_reflectObject[] = {
	"PosAttribute", "myNormal" };
const char* uni_reflectObject[] = {
"viewProjMatrix", "ModelMatrix", "eyePos", "emission", "color"};
const char* spl_reflectObject[] = {
"envMap","envMapRough"};
const char* vtx_reflectObject = "\
precision highp float;\
attribute vec3 PosAttribute;\
attribute vec3 myNormal;\
uniform mat4 viewProjMatrix;   \
uniform mat4 ModelMatrix;    \
uniform vec3 eyePos;\
varying vec4 Position;\
varying vec3 Normal;\
varying vec3 IncidentVector;\
void main()\
{\
   vec4 P = ModelMatrix * vec4(PosAttribute, 1.0);\
   vec3 N = normalize(mat3(ModelMatrix) * myNormal);\
   vec3 I = P.xyz - eyePos;\
   Position = P;\
   Normal = N;\
   IncidentVector = I;\
   gl_Position = viewProjMatrix * P;\
}";

const char* frg_reflectObject = "\
precision highp float;\
varying vec4 Position;\
varying vec3 Normal;\
varying vec3 IncidentVector;\
uniform vec3 emission;\
uniform vec4 color;\
uniform samplerCube envMap;\
uniform samplerCube envMapRough;\
float my_fresnel(vec3 I, vec3 N, float power,  float scale,  float bias)\
{\
    return bias + (pow(clamp(1.0 - dot(I, N), 0.0, 1.0), power) * scale);\
}\
void main()\
{\
    vec3 I = normalize(IncidentVector);\
    vec3 N = normalize(Normal);\
    vec3 R = reflect(I, N);\
    float fresnel = my_fresnel(-I, N, 5.0, 1.0, 0.1);\
    vec3 Creflect = textureCube(envMap, R).rgb;\
	vec3 CreflectRough = textureCube(envMapRough, R).rgb;\
    CreflectRough *= color.rgb;\
	Creflect *= color.rgb;\
	gl_FragColor = vec4(mix(mix(CreflectRough,Creflect,fresnel),mix(Creflect,CreflectRough,fresnel),color.a)+emission, 1.0);\
}";

//////////////skyBoxRender///////////////
const char* atb_skybox[] = {
	"PosAttribute" };
const char* uni_skybox[] = {
"viewMatrix", "ProjMatrix"};
const char* spl_skybox[] = {
"envMap"};
const char* vtx_skybox = "\
precision highp float;\
attribute vec3 PosAttribute;\
uniform mat4 viewMatrix;   \
uniform mat4 ProjMatrix;\
varying vec3 TexCoord;\
void main()\
{\
    TexCoord  = mat3(viewMatrix) * vec3(PosAttribute.xyz);\
	gl_Position = ProjMatrix * vec4(PosAttribute, 1.0);\
}";

const char* frg_skybox = "\
precision highp float;\
varying vec3 TexCoord;\
uniform samplerCube envMap;\
void main()\
{\
	gl_FragColor = textureCube(envMap, TexCoord);\
	gl_FragColor.a = gl_FragCoord.z;\
}";


//////////////downSample///////////////
const char* atb_downSample[] = {
	"PosAttribute", "TexAttribute" };
const char* uni_downSample[] = {
"mvp"};
const char* spl_downSample[] = {
"sampler"};

const char* vtx_downSample = "\
precision highp float;\
attribute vec3 PosAttribute;\
attribute vec2 TexAttribute; \
varying vec2 TexCoord;\
void main()\
{\
	gl_Position = vec4(PosAttribute, 1.0);\
	TexCoord    = TexAttribute; \
}";

const char* frg_downSample = "\
precision highp float;\
varying vec2 TexCoord;\
uniform sampler2D sampler;\
void main()\
{\
	gl_FragColor = texture2D(sampler, TexCoord);\
}";


//////////////downSample4x///////////////
const char* atb_downSample4x[] = {
	"PosAttribute", "TexAttribute" };
const char* uni_downSample4x[] = {
"twoTexelSize"};
const char* spl_downSample4x[] = {
"sampler"};
const char* vtx_downSample4x = "\
precision highp float;\
attribute vec3 PosAttribute; \
attribute vec2 TexAttribute; \
uniform vec2 twoTexelSize;\
varying vec2 TexCoord1;\
varying vec2 TexCoord2;\
varying vec2 TexCoord3;\
varying vec2 TexCoord4;\
void main()\
{\
  TexCoord1 = TexAttribute;\
  TexCoord2 = TexAttribute + vec2(twoTexelSize.x, 0);\
  TexCoord3 = TexAttribute + vec2(twoTexelSize.x, twoTexelSize.y);\
  TexCoord4 = TexAttribute + vec2(0, twoTexelSize.y);\
  gl_Position = vec4(PosAttribute, 1.0);\
}";

const char* frg_downSample4x = "\
precision highp float;\
varying vec2 TexCoord1;\
varying vec2 TexCoord2;\
varying vec2 TexCoord3;\
varying vec2 TexCoord4;\
uniform sampler2D sampler;\
void main()\
{\
		gl_FragColor = (texture2D(sampler, TexCoord1) + \
            				texture2D(sampler, TexCoord2) +\
            				texture2D(sampler, TexCoord3) +\
            				texture2D(sampler, TexCoord4))*0.25;\
}";


//////////////extractHighLightArea///////////////
const char* atb_extractHL[] = {
	"PosAttribute", "TexAttribute" };
const char* uni_extractHL[] = {
"threshold","scalar"};
const char* spl_extractHL[] = {
"sampler"};
const char* vtx_extractHL = "\
precision highp float;\
attribute vec3 PosAttribute;\
attribute vec2 TexAttribute; \
varying vec2 TexCoord;\
void main()\
{\
	gl_Position = vec4(PosAttribute, 1.0);\
	TexCoord    = TexAttribute; \
}";

const char* frg_extractHL = "\
precision highp float;\
varying vec2 TexCoord;\
uniform float	threshold;\
uniform float	scalar;\
uniform sampler2D sampler;\
void main()\
{\
	gl_FragColor = max((texture2D(sampler, TexCoord) - threshold)*scalar, vec4(0.0,0.0,0.0,0.0));\
}";

//////////////gaussian blur///////////////
const char* atb_blur[] = {
	"PosAttribute", "TexAttribute" };
const char* uni_blur[] = {
"MVPMatrix"};
const char* spl_blur[] = {
"TexSampler"};
const char* vtx_blur = "\
precision highp float;\
attribute vec3  PosAttribute;\
attribute vec2  TexAttribute;\
varying vec2 TexCoord0;\
void main()\
{\
	gl_Position = vec4(PosAttribute, 1.0);\
	TexCoord0 = TexAttribute;\
}";


//////////////gaussian blur composition///////////////
const char* atb_gaussianCompose[] = {
	"PosAttribute", "TexAttribute" };
const char* uni_gaussianCompose[] = {
"coeff"};
const char* spl_gaussianCompose[] = {
"sampler1","sampler2","sampler3","sampler4"};
const char* vtx_gaussianCompose = "\
precision highp float;\
attribute vec3 PosAttribute;\
attribute vec2 TexAttribute;\
varying vec2 TexCoord;\
void main()\
{\
	gl_Position = vec4(PosAttribute, 1.0);\
	TexCoord    = TexAttribute;\
}";

const char* frg_gaussianCompose = "\
precision highp float;\
varying vec2 TexCoord;\
uniform vec4 coeff;\
uniform sampler2D sampler1;\
uniform sampler2D sampler2;\
uniform sampler2D sampler3;\
uniform sampler2D sampler4;\
void main()\
{\
	gl_FragColor = texture2D(sampler1, TexCoord)*coeff.x + texture2D(sampler2, TexCoord)*coeff.y + texture2D(sampler3, TexCoord)*coeff.z + texture2D(sampler4, TexCoord)*coeff.w;\
}";


//////////////star glare///////////////
const char* atb_starStreak[] = {
	"PosAttribute", "TexAttribute" };
const char* uni_starStreak[] = {
"stepSize","Stride","colorCoeff"};
const char* spl_starStreak[] = {
"sampler"};

const char* vtx_starStreak = "\
precision highp float;\
attribute vec3 PosAttribute; \
attribute vec2 TexAttribute; \
uniform vec2 stepSize;\
uniform float Stride;\
varying vec2 TexCoord1;\
varying vec2 TexCoord2;\
varying vec2 TexCoord3;\
varying vec2 TexCoord4;\
void main()\
{\
  TexCoord1 = TexAttribute;\
  TexCoord2 = TexAttribute + stepSize*Stride;\
  TexCoord3 = TexAttribute + stepSize*2.0*Stride;\
  TexCoord4 = TexAttribute + stepSize*3.0*Stride;\
  gl_Position = vec4(PosAttribute, 1.0);\
}";

const char* frg_starStreak = "\
precision highp float;\
uniform vec4 colorCoeff[4];\
varying vec2 TexCoord1;\
varying vec2 TexCoord2;\
varying vec2 TexCoord3;\
varying vec2 TexCoord4;\
uniform sampler2D sampler;\
void main()\
{\
	gl_FragColor = texture2D(sampler, TexCoord1)*colorCoeff[0] + texture2D(sampler, TexCoord2)*colorCoeff[1] + texture2D(sampler, TexCoord3)*colorCoeff[2] + texture2D(sampler, TexCoord4)*colorCoeff[3];\
}";

//////////////glare composition///////////////
const char* atb_glareCompose[] = {
	"PosAttribute", "TexAttribute" };
const char* uni_glareCompose[] = {
"mixCoeff"};
const char* spl_glareCompose[] = {
"sampler1","sampler2","sampler3"};

const char* vtx_glareCompose = "\
precision highp float;\
attribute vec3 PosAttribute;\
attribute vec2 TexAttribute;\
varying vec2 TexCoord;\
void main()\
{\
	gl_Position = vec4(PosAttribute, 1.0);\
	TexCoord    = TexAttribute;\
}";

const char* frg_glareCompose = "\
precision highp float;\
varying vec2 TexCoord;\
uniform vec4 mixCoeff;\
uniform sampler2D sampler1;\
uniform sampler2D sampler2;\
uniform sampler2D sampler3;\
void main()\
{\
	gl_FragColor = texture2D(sampler1, TexCoord)*mixCoeff.x + texture2D(sampler2, TexCoord)*mixCoeff.y + texture2D(sampler3, TexCoord)*mixCoeff.z;\
}";


//////////////star streak composition///////////////
const char* atb_starStreakCompose[] = {
	"PosAttribute", "TexAttribute" };
const char* uni_starStreakCompose[] = {
"coeff"};
const char* spl_starStreakCompose[] = {
"sampler1","sampler2","sampler3","sampler4"};

const char* vtx_starStreakCompose = "\
precision highp float;\
attribute vec3 PosAttribute;\
attribute vec2 TexAttribute;\
varying vec2 TexCoord;\
void main()\
{\
	gl_Position = vec4(PosAttribute, 1.0);\
	TexCoord    = TexAttribute;\
}";

const char* frg_starStreakCompose = "\
precision highp float;\
varying vec2 TexCoord;\
uniform vec4 coeff;\
uniform sampler2D sampler1;\
uniform sampler2D sampler2;\
uniform sampler2D sampler3;\
uniform sampler2D sampler4;\
void main()\
{\
  vec4 color1 = max(texture2D(sampler1, TexCoord), texture2D(sampler2, TexCoord));\
  vec4 color2 = max(texture2D(sampler3, TexCoord), texture2D(sampler4, TexCoord));\
  gl_FragColor = max(color1, color2);\
}";


//////////////ghost image///////////////
const char* atb_ghostImage[] = {
	"PosAttribute", "TexAttribute" };
const char* uni_ghostImage[] = {
"scalar","colorCoeff"};
const char* spl_ghostImage[] = {
"sampler1","sampler2","sampler3","sampler4"};

const char* vtx_ghostImage = "\
precision highp float;\
attribute vec3 PosAttribute; \
attribute vec2 TexAttribute; \
uniform vec4 scalar;\
varying vec2 TexCoord1;\
varying vec2 TexCoord2;\
varying vec2 TexCoord3;\
varying vec2 TexCoord4;\
void main()\
{\
  TexCoord1 = (TexAttribute - 0.5) * scalar[0] + 0.5;\
  TexCoord2 = (TexAttribute - 0.5) * scalar[1] + 0.5;\
  TexCoord3 = (TexAttribute - 0.5) * scalar[2] + 0.5;\
  TexCoord4 = (TexAttribute - 0.5) * scalar[3] + 0.5;\
  gl_Position = vec4(PosAttribute, 1.0);\
}";

const char* frg_ghostImage = "\
precision highp float;\
uniform vec4 colorCoeff[4];\
varying vec2 TexCoord1;\
varying vec2 TexCoord2;\
varying vec2 TexCoord3;\
varying vec2 TexCoord4;\
uniform sampler2D sampler1;\
uniform sampler2D sampler2;\
uniform sampler2D sampler3;\
uniform sampler2D sampler4;\
void main()\
{\
	gl_FragColor = texture2D(sampler1, TexCoord1)*texture2D(sampler4, TexCoord1).g*colorCoeff[0] + texture2D(sampler1, TexCoord2)*texture2D(sampler4, TexCoord2).g*colorCoeff[1] + texture2D(sampler2, TexCoord3)*texture2D(sampler4, TexCoord3).g*colorCoeff[2] + texture2D(sampler3, TexCoord4)*texture2D(sampler4, TexCoord4).g*colorCoeff[3];\
}";

	//gl_FragColor = texture2D(sampler1, TexCoord4);\


//////////////tonemapping///////////////
const char* atb_tonemap[] = {
	"PosAttribute", "TexAttribute" };
const char* uni_tonemap[] = {
"blurAmount","exposure","gamma"};
const char* spl_tonemap[] = {
"sceneTex","blurTex","lumTex"};
const char* vtx_tonemap = "\
precision highp float;\
attribute vec3 PosAttribute;\
attribute vec2 TexAttribute;\
varying vec2 TexCoord;\
void main()\
{\
	gl_Position = vec4(PosAttribute, 1.0);\
	TexCoord    = TexAttribute; \
}";

const char* frg_tonemap = "\
precision highp float;\
varying vec2 TexCoord;\
uniform sampler2D   sceneTex;\
uniform sampler2D   blurTex;\
uniform sampler2D   lumTex;\
uniform float blurAmount;\
uniform float exposure;\
uniform float gamma;\
float A = 0.15;\
float B = 0.50;\
float C = 0.10;\
float D = 0.20;\
float E = 0.02;\
float F = 0.30;\
float W = 11.2;\
vec3 filmicTonemapping(vec3 x)\
{\
  return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;\
}\
float vignette(vec2 pos, float inner, float outer)\
{\
  float r = length(pos);\
  r = 1.0 - smoothstep(inner, outer, r);\
  return r;\
}\
void main()\
{\
    vec4 scene = texture2D(sceneTex, TexCoord);\
    vec4 blurred = texture2D(blurTex, TexCoord);\
	float lum = texture2D(lumTex, vec2(0.0,0.0)).r;\
    vec3 c = mix(scene.rgb, blurred.rgb, blurAmount);\
    c = c * exposure/lum;\
	c = c * vignette(TexCoord*2.0-1.0, 0.55, 1.5);\
	float ExposureBias = 1.0;\
	c = filmicTonemapping(ExposureBias*c);\
	vec3 whiteScale = 1.0/filmicTonemapping(vec3(W,W,W));\
	c = c*whiteScale;\
    c.r = pow(c.r, gamma);\
    c.g = pow(c.g, gamma);\
    c.b = pow(c.b, gamma);\
	gl_FragColor = vec4(c, 1.0);\
}";
//		gl_FragColor = vec4(lum,lum,lum,1.0);\

//////////////calculateLuminance///////////////
const char* uni_calculateLuminance[] = {
"blurAmount"};
const char* tex_calculateLuminance[] = {
"inputImage","outputImage"};
const char* cs_calculateLuminance = "\
#version 430\n\
layout (local_size_x =1, local_size_y = 1) in;\n\
layout(binding=0, rgba16f) uniform highp image2D inputImage;\n\
layout(binding=1, rgba16f) uniform highp image2D outputImage;\n\
vec3 LUMINANCE_VECTOR  = vec3(0.2125, 0.7154, 0.0721);\n\
void main()\n\
{\n\
		float logLumSum = 0.0f;\n\
		int x,y;\n\
		for (y=0; y<16;y++) {\n\
			for (x=0; x<16;x++) {\n\
				logLumSum += (dot(imageLoad(inputImage, ivec2(x,y)).rgb, LUMINANCE_VECTOR)+0.00001);\n\
			}\n\
		}\n\
		logLumSum /= 256.0;\n\
		float val = (logLumSum+0.00001);\n\
		imageStore(outputImage, ivec2(0,0), vec4(val,val,val,val));\n\
}";

const char* cs_calculateLuminanceES = "\
#version 310 es\n\
precision highp float;\
layout (local_size_x =1, local_size_y = 1) in;\n\
layout(binding=0, rgba16f) uniform highp image2D inputImage;\n\
layout(binding=1, rgba16f) uniform highp image2D outputImage;\n\
vec3 LUMINANCE_VECTOR  = vec3(0.2125, 0.7154, 0.0721);\n\
void main()\n\
{\n\
		float logLumSum = 0.0f;\n\
		int x,y;\n\
		for (y=0; y<16;y++) {\n\
			for (x=0; x<16;x++) {\n\
				logLumSum += (dot(imageLoad(inputImage, ivec2(x,y)).rgb, LUMINANCE_VECTOR)+0.00001);\n\
			}\n\
		}\n\
		logLumSum /= 256.0;\n\
		float val = (logLumSum+0.00001);\n\
		imageStore(outputImage, ivec2(0,0), vec4(val,val,val,val));\n\
}";

//////////////calculateAdaptiveLum///////////////
const char* uni_calculateAdaptedLum[] = {
"elapsedTime"};
const char* tex_calculateAdaptedLum[] = {
"currentImage","image0","image1"};
const char* cs_calculateAdaptedLum = "\
#version 430\n\
layout (local_size_x =1, local_size_y = 1) in;\n\
uniform float elapsedTime;\n\
layout(binding=0, rgba16f) uniform highp image2D currentImage;\n\
layout(binding=1, rgba16f) uniform highp image2D image0;\n\
layout(binding=2, rgba16f) uniform highp image2D image1;\n\
void main()\n\
{\n\
	float currentLum = imageLoad(currentImage, ivec2(0,0)).r;\n\
	float lastLum = imageLoad(image0, ivec2(0,0)).r;\n\
	float newLum = lastLum + (currentLum - lastLum) * ( 1.0 - pow( 0.98f, 30.0 * elapsedTime ) );\n\
	imageStore(image1, ivec2(0,0), vec4(newLum,newLum,newLum,newLum));\n\
}";

const char* cs_calculateAdaptedLumES = "\
#version 310 es\n\
precision highp float;\
layout (local_size_x =1, local_size_y = 1) in;\n\
uniform float elapsedTime;\n\
layout(binding=0, rgba16f) uniform highp image2D currentImage;\n\
layout(binding=1, rgba16f) uniform highp image2D image0;\n\
layout(binding=2, rgba16f) uniform highp image2D image1;\n\
void main()\n\
{\n\
	float currentLum = imageLoad(currentImage, ivec2(0,0)).r;\n\
	float lastLum = imageLoad(image0, ivec2(0,0)).r;\n\
	float newLum = lastLum + (currentLum - lastLum) * ( 1.0 - pow( 0.98f, 30.0 * elapsedTime ) );\n\
	imageStore(image1, ivec2(0,0), vec4(newLum,newLum,newLum,newLum));\n\
}";

#define PROGRAM_PARAMETER(nameString) vtx_##nameString, frg_##nameString, atb_##nameString, sizeof(atb_##nameString)/sizeof(char *), uni_##nameString,  sizeof(uni_##nameString)/sizeof(char *),spl_##nameString,sizeof(spl_##nameString)/sizeof(char *)
#define LOC_PARAMETER(nameString) atb_##nameString, sizeof(atb_##nameString)/sizeof(char *), uni_##nameString,  sizeof(uni_##nameString)/sizeof(char *),spl_##nameString,sizeof(spl_##nameString)/sizeof(char *)

#endif  // SHADER_DECLARATION_H_

