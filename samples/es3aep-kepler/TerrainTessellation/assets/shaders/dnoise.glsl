//----------------------------------------------------------------------------------
// File:        es3aep-kepler\TerrainTessellation\assets\shaders/dnoise.glsl
// SDK Version: v3.00 
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//----------------------------------------------------------------------------------
// 3D noise with derivatives
// http://www.iquilezles.org/www/articles/morenoise/morenoise.htm

uniform sampler3D randTex3D;

vec3 fade(vec3 t)
{
    return t*t*t*(t*(t*6-15)+10); // new curve (quintic)
}

// derivative of fade function
vec3 dfade(vec3 t)
{
    return 30.0*t*t*(t*(t-2.0)+1.0); // new curve (quintic)
}

// returns random value in [-1, 1] for each integer position
float cellnoise(vec3 p)
{
    return texture(randTex3D, p * invNoise3DSize).x*2.0-1.0;
}

// 3D noise with derivatives
vec4 dnoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 u = p - i;

    vec3 du = dfade(u);
    u = fade(u);

    float a = cellnoise( vec3(i.x+0, i.y+0, i.z+0) );
    float b = cellnoise( vec3(i.x+1, i.y+0, i.z+0) );
    float c = cellnoise( vec3(i.x+0, i.y+1, i.z+0) );
    float d = cellnoise( vec3(i.x+1, i.y+1, i.z+0) );
    float e = cellnoise( vec3(i.x+0, i.y+0, i.z+1) );
    float f = cellnoise( vec3(i.x+1, i.y+0, i.z+1) );
    float g = cellnoise( vec3(i.x+0, i.y+1, i.z+1) );
    float h = cellnoise( vec3(i.x+1, i.y+1, i.z+1) );

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    vec4 r;
    // derivative
    r.x = du.x * (k1 + k4*u.y + k6*u.z + k7*u.y*u.z);
    r.y = du.y * (k2 + k5*u.z + k4*u.x + k7*u.z*u.x);
    r.z = du.z * (k3 + k6*u.x + k5*u.y + k7*u.x*u.y);

    // normal noise value
    r.w = k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z;
    return r;
}
