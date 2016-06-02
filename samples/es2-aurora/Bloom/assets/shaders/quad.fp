#version 100
precision mediump float;
varying vec2 v_TexCoord;
uniform sampler2D tex;

vec2 t;

void main() 
{
	lowp vec4 tex = texture2D(tex,v_TexCoord);
	gl_FragColor = tex.rgba;
}
