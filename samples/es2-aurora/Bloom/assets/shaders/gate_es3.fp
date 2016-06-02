#version 300 es
precision mediump float;

	in mediump vec2 v_TexCoord;
	in mediump vec3 v_Normal;
	in mediump vec3 v_WorldSpacePosition;	
	in mediump vec4 v_ShadowMapTexCoord;
			
	uniform lowp vec3 u_LightVector;
	uniform sampler2D diffuse_tex;
	uniform sampler2D glow_tex;
	uniform float u_glowIntensity;
	
	uniform sampler2DShadow variance_shadowmap_tex;
	
	out vec4 my_FragColor;

	lowp vec3 tex;
	lowp float diffuse_factor;
		 float specular_factor;
	lowp vec3 color;
	     vec3 normal;
	float distance;
	lowp float shadow_factor;     

    void main() 
    {
		// fetching normal
		normal = normalize(v_Normal);

		shadow_factor = (v_ShadowMapTexCoord.w > 0.0) 
		    ? texture(variance_shadowmap_tex,v_ShadowMapTexCoord.xyz) : 1.0;

		tex = texture(diffuse_tex,v_TexCoord.xy).rgb;

		// calculating surface color
    	diffuse_factor = 0.1+shadow_factor*clamp(dot(u_LightVector,normal),0.0,1.0);
	    color.rgb = tex * diffuse_factor 
			+ u_glowIntensity * texture(glow_tex,v_TexCoord.xy).rgb;

		my_FragColor = vec4(color.rgb, 1.0);
    }
