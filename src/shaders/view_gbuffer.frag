#version 430

##import lib/util

layout (binding = 0) uniform sampler2D gBuffer0;
layout (binding = 1) uniform sampler2D gBuffer1;
layout (binding = 2) uniform sampler2D depthStencil;

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;

uniform int mode;
uniform mat4 invProjMat;

void main()
{
	if (mode == 1) {
		outFragColor = texture(gBuffer0, inTexCoord);
	} else if (mode == 2) {
		outFragColor = texture(gBuffer1, inTexCoord);
	} else if (mode == 3) {
		float depth = texture(depthStencil, inTexCoord).r;
		vec3 position = ScreenPosToViewPos(inTexCoord, depth, invProjMat);
		outFragColor.rgb = vec3(-position.z / 32.0);
	}
}