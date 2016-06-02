#version 100
varying mediump vec2 v_TexCoord;

uniform sampler2D blurred_texture;
uniform sampler2D original_texture;
uniform mediump float u_glowIntensity;

mediump vec4 temp1;


void main()
{
	temp1  = 1.0*texture2D(original_texture,v_TexCoord);
	temp1 += u_glowIntensity*texture2D(blurred_texture,v_TexCoord);

	gl_FragColor = temp1;
}