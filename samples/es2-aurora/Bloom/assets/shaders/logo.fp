#version 100
precision mediump float;
varying vec2 v_TexCoord;
uniform vec3 u_Color;
uniform sampler2D logo_tex;

vec2 t;

void main() 
{
	vec4 tex;
	tex=texture2D(logo_tex,v_TexCoord);
	
	gl_FragColor = vec4(tex);
}
