#ifndef MATHUTIL
#define MATHUTIL

#include <algorithm>

#include <glm/vec2.hpp> // vec2, bvec2, dvec2, ivec2 and uvec2
#include <glm/vec3.hpp> // vec3, bvec3, dvec3, ivec3 and uvec3
#include <glm/vec4.hpp> // vec4, bvec4, dvec4, ivec4 and uvec4
#include <glm/mat2x2.hpp> // mat2, dmat2
#include <glm/mat2x3.hpp> // mat2x3, dmat2x3
#include <glm/mat2x4.hpp> // mat2x4, dmat2x4
#include <glm/mat3x2.hpp> // mat3x2, dmat3x2
#include <glm/mat3x3.hpp> // mat3, dmat3
#include <glm/mat3x4.hpp> // mat3x4, dmat2
#include <glm/mat4x2.hpp> // mat4x2, dmat4x2
#include <glm/mat4x3.hpp> // mat4x3, dmat4x3
#include <glm/mat4x4.hpp> // mat4, dmat4
#include <glm/common.hpp> // all the GLSL common functions
#include <glm/exponential.hpp> // all the GLSL exponential functions
#include <glm/geometric.hpp> // all the GLSL geometry functions
#include <glm/integer.hpp> // all the GLSL integer functions
#include <glm/matrix.hpp> // all the GLSL matrix functions
#include <glm/packing.hpp> // all the GLSL packing functions
#include <glm/trigonometric.hpp> // all the GLSL trigonometric functions
#include <glm/vector_relational.hpp> // all the GLSL vector relational functions

using namespace glm;

static vec2 repairUv(vec2 uv){
    vec2 outuv = vec2( 0, 0 );
    
    if (uv.x<0) {
        outuv.x = 1.0 + uv.x;
    }
    else if (uv.x > 1.0){
        outuv.x = uv.x - 1.0;
    }
    else {
        outuv.x = uv.x;
    }
    
    if (uv.y<0) {
        outuv.y = 1.0 + uv.y;
    }
    else if (uv.y > 1.0){
        outuv.y = uv.y - 1.0;
    }
    else {
        outuv.y = uv.y;
    }
    
    return outuv;
}

static vec2 polarCoord(vec3 dir) {
    vec3 ndir = normalize(dir);
    float longi = -atan2(ndir.z, ndir.x);
    
    float lat = acos(-ndir.y);
    
    vec2 uv;
    uv.x = longi;
    uv.y = lat;
    
    vec2 pitwo = vec2(M_PI, M_PI);
    uv /= pitwo;
    uv.x /= 2.0;
    vec2 ones = vec2(1.0, 1.0);
    uv = modf(uv, ones);
    return uv;
}

static vec3 fisheyeDir(vec3 dir, const mat3 rotMat) {
    
    dir.x = dir.x / dir.z;
    dir.y = dir.y / dir.z;
    dir.z = dir.z / dir.z;
    
    vec2 uv;
    uv.x = dir.x;
    uv.y = dir.y;
    float r = sqrtf(uv.x*uv.x + uv.y*uv.y);
    
    float phi = atan2f(uv.y, uv.x);
    
    float theta = r;
    
    vec3 fedir = vec3( 0, 0, 0 );
    fedir.x = sin(theta) * cos(phi);
    fedir.y = sin(theta) * sin(phi);
    fedir.z = cos(theta);
    
    fedir = rotMat * fedir;
    
    return fedir;
}

static vec3 tinyPlanetSph(vec3 uv) {
    vec3 sph;
    vec2 uvxy;
    uvxy.x = uv.x / uv.z;
    uvxy.y = uv.y / uv.z;
    
    float u = length(uvxy);
    float alpha = atan2(2.0f, u);
    float phi = M_PI - 2 * alpha;
    float z = cos(phi);
    float x = sin(phi);
    
    uvxy = normalize(uvxy);
    
    sph.z = z;
    
    vec2 sphxy = uvxy * x;
    
    sph.x = sphxy.x;
    sph.y = sphxy.y;
    
    return sph;
}

static float fitRange(float value, float in_min, float in_max, float out_min, float out_max){
    float out = out_min + ((out_max - out_min) / (in_max - in_min)) * (value - in_min);
    return std::min(out_max, std::max(out, out_min));
}

double static interpParam_CPU(int paramID, PF_InData* in_data, float offset) {
	PrTime time = in_data->current_time;
	PrTime timeStep = in_data->time_step;

	PF_ParamDef	def;
	AEFX_CLR_STRUCT(def);

	if (offset == 0) {
		PF_CHECKOUT_PARAM(in_data, paramID, time, timeStep, in_data->time_scale, &def);
		float outVal = def.u.fs_d.value;
		PF_CHECKIN_PARAM(in_data, &def);
		return outVal;
	}
	else if (offset < 0) {
		offset = -offset;
		float floor = std::floor(offset);
		float frac = offset - floor;

		PF_CHECKOUT_PARAM(in_data, paramID, time - (floor + 1)*timeStep, timeStep, in_data->time_scale, &def);
		float part1 = def.u.fs_d.value * frac;
		PF_CHECKIN_PARAM(in_data, &def);
		AEFX_CLR_STRUCT(def);
		PF_CHECKOUT_PARAM(in_data, paramID, time - floor * timeStep, timeStep, in_data->time_scale, &def);
		float part2 = def.u.fs_d.value * (1 - frac);
		PF_CHECKIN_PARAM(in_data, &def);
		
		return part1 + part2;
	}
	else {
		float floor = std::floor(offset);
		float frac = offset - floor;

		PF_CHECKOUT_PARAM(in_data, paramID, time + (floor + 1)*timeStep, timeStep, in_data->time_scale, &def);
		float part1 = def.u.fs_d.value * frac;
		PF_CHECKIN_PARAM(in_data, &def);
		AEFX_CLR_STRUCT(def);
		PF_CHECKOUT_PARAM(in_data, paramID, time + floor * timeStep, timeStep, in_data->time_scale, &def);
		float part2 = def.u.fs_d.value * (1 - frac);
		PF_CHECKIN_PARAM(in_data, &def);

		return part1 + part2;
	}
}

static float getCameraBlend_CPU(PF_InData* in_data, float inBlend, float offset) {

	float accel = interpParam_CPU(AUX_ACCELERATION, in_data, offset);
	float blend = inBlend;

	if (blend < 0.5) {
		blend = fitRange(blend, 0, 0.5, 0, 1);
		blend = std::pow(blend, accel);
		blend = fitRange(blend, 0, 1, 0, 0.5);
	}
	else {
		blend = fitRange(blend, 0.5, 1.0, 0, 1);
		blend = 1.0 - blend;
		blend = std::pow(blend, accel);
		blend = 1.0 - blend;
		blend = fitRange(blend, 0, 1, 0.5, 1.0);
	}

	return blend;
}


#endif
