#version 100
precision mediump float;
varying vec2 v_TexCoord;
uniform vec3 u_Color;
uniform sampler2D font_tex;

vec2 t;

void main() 
{
    	vec2 t;
	vec4 tex;
	tex=texture2D(font_tex,v_TexCoord);
	t.x=tex.a;
	tex=texture2D(font_tex,v_TexCoord-vec2(1.0/2048.0,0));
	t.y=tex.a;
	
	gl_FragColor = vec4(u_Color*max(t.x*1.0,t.y*0.2), clamp(t.x+t.y,0.0,1.0));
}
