#version 100
varying mediump vec2 v_TexCoord;

uniform sampler2D texture_to_downfilter;
uniform mediump vec2 u_TexelOffsets;

mediump vec4 temp1;
mediump float luminance;

void main()
{	
	
	temp1 = texture2D(texture_to_downfilter,v_TexCoord+vec2(u_TexelOffsets));
	temp1 += texture2D(texture_to_downfilter,v_TexCoord+vec2(u_TexelOffsets.x,-u_TexelOffsets.y));
	temp1 += texture2D(texture_to_downfilter,v_TexCoord+vec2(-u_TexelOffsets.x,u_TexelOffsets.y));
	temp1 += texture2D(texture_to_downfilter,v_TexCoord+vec2(-u_TexelOffsets));
	temp1 *= 0.25;
	
	//temp1 = texture2D(texture_to_downfilter,v_TexCoord);
	luminance = dot(temp1.rgb,vec3(0.2126,0.7152,0.0722));

	gl_FragColor = temp1*(luminance-0.25);

}