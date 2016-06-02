#version 100
precision mediump float;
uniform vec4 u_Color;
varying float v_Alpha;

void main() 
{
	gl_FragColor = u_Color*v_Alpha;
}
