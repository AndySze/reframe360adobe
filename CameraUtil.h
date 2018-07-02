#pragma once

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
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

using namespace glm;

mat3 pitchMatrix(float pitch) {
	mat4 pitchMat = rotate(pitch, vec3(1.0f, 0, 0));
	return mat3(pitchMat);
}


mat3 yawMatrix(float yaw) {
	mat4 yawMat = rotate(yaw, vec3(0, 1.0f, 0));
	return mat3(yawMat);
}

mat3 rollMatrix(float roll) {
	mat4 rollMat = rotate(roll, vec3(0, 0, 1.0f));
	return mat3(rollMat);
}

glm::mat3 rotationMatrix(float pitch, float yaw, float roll) {
	mat3 pitchMat = pitchMatrix(pitch);
	mat3 yawMat = yawMatrix(yaw);
	mat3 rollMat = rollMatrix(roll);

	return rollMat* pitchMat * yawMat;
}