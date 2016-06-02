#version 100
varying mediump vec2 v_TexCoord;

uniform sampler2D texture_to_blur;
uniform mediump vec2 u_TexelOffsets;

mediump vec4 temp1;
mediump vec2 sample_texcoord;

void main()
{
	sample_texcoord = v_TexCoord;

	sample_texcoord -= u_TexelOffsets*5.0;
	temp1  = texture2D(texture_to_blur,sample_texcoord);

	sample_texcoord += u_TexelOffsets;
	temp1 += texture2D(texture_to_blur,sample_texcoord);

	sample_texcoord += u_TexelOffsets;
	temp1 += 2.0*texture2D(texture_to_blur,sample_texcoord);

	sample_texcoord += u_TexelOffsets;
	temp1 += 2.0*texture2D(texture_to_blur,sample_texcoord);

	sample_texcoord += u_TexelOffsets;
	temp1 += 3.0*texture2D(texture_to_blur,sample_texcoord);

	sample_texcoord += u_TexelOffsets;
	temp1 += 3.0*texture2D(texture_to_blur,sample_texcoord);

	sample_texcoord += u_TexelOffsets;
	temp1 += 3.0*texture2D(texture_to_blur,sample_texcoord);

	sample_texcoord += u_TexelOffsets;
	temp1 += 2.0*texture2D(texture_to_blur,sample_texcoord);

	sample_texcoord += u_TexelOffsets;
	temp1 += 2.0*texture2D(texture_to_blur,sample_texcoord);

	sample_texcoord += u_TexelOffsets;
	temp1 += texture2D(texture_to_blur,sample_texcoord);

	sample_texcoord += u_TexelOffsets;
	temp1 += texture2D(texture_to_blur,sample_texcoord);

	temp1/=21.0;

	gl_FragColor = temp1;
}
