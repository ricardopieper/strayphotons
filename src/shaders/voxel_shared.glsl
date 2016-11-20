#ifndef VOXEL_SHARED_GLSL_INCLUDED
#define VOXEL_SHARED_GLSL_INCLUDED

##import raytrace/intersection

const float VoxelGridSize = 256;

const uint MipmapWorkGroupSize = 256;

const uint[13] MaxFragListMask = uint[](8191, 4095, 2047, 1023, 511, 255, 127, 63, 31, 15, 7, 3, 1);
const uint[13] FragListWidthBits = uint[](13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1);

const mat4[3] AxisSwapForward = mat4[](
	mat4(mat3(0, 0, -1, 0, 1, 0, 1, 0, 0)),
	mat4(mat3(1, 0, 0, 0, 0, -1, 0, 1, 0)),
	mat4(1.0)
);

const mat3[3] AxisSwapReverse = mat3[](
	mat3(0, 0, 1, 0, 1, 0, -1, 0, 0),
	mat3(1, 0, 0, 0, 0, 1, 0, -1, 0),
	mat3(1.0)
);

float ReadVoxelAndClear(layout(r32ui) uimage3D packedVoxelImg, ivec3 position, out vec3 outColor, out vec3 outRadiance)
{
	ivec3 index = position * ivec3(4, 1, 1);

	uint data = imageAtomicExchange(packedVoxelImg, index, uint(0)).r;
	float colorRed = float((data & 0xFFFF0000) >> 16) / 0xFF;
	float colorGreen = float(data & 0xFFFF) / 0xFF;

	data = imageAtomicExchange(packedVoxelImg, index + ivec3(1, 0, 0), uint(0)).r;

	float colorBlue = float((data & 0xFFFF0000) >> 16) / 0xFF;
	float radianceRed = float(data & 0xFFFF) / 0x7FF;

	data = imageAtomicExchange(packedVoxelImg, index + ivec3(2, 0, 0), uint(0)).r;

	float radianceGreen = float((data & 0xFFFF0000) >> 16) / 0x7FF;
	float radianceBlue = float(data & 0xFFFF) / 0x7FF;

	outColor = vec3(colorRed, colorGreen, colorBlue);
	outRadiance = vec3(radianceRed, radianceGreen, radianceBlue);
	return float(imageAtomicExchange(packedVoxelImg, index + ivec3(3, 0, 0), uint(0)).r);
}

#ifdef INCLUDE_VOXEL_TRACE

vec3 GetVoxel(vec3 position, int level, out vec3 color, out vec3 radiance)
{
	vec4 colorData = textureLod(voxelColor, position/vec3(int(VoxelGridSize)>>level), 0);
	vec4 radianceData = textureLod(voxelRadiance, position/vec3(int(VoxelGridSize)>>level), level);
	vec4 alphaData = textureLod(voxelAlpha, position/vec3(int(VoxelGridSize)>>level), level);
	color = colorData.rgb / (colorData.a + 0.00001);
	radiance = radianceData.rgb / (radianceData.a + 0.00001);
	return alphaData.xyz / (alphaData.a + 0.00001);
}

bool CheckVoxel(vec3 position, float size)
{
	float level = max(0, log2(size));
	vec4 alphaData = textureLod(voxelAlpha, position/vec3(VoxelGridSize), level);
	return alphaData.a > 0;
}

vec4 SampleVoxel(vec3 position, vec3 dir, float size)
{
	float level = max(0, log2(size));
	vec4 radianceData = textureLod(voxelRadiance, position/vec3(VoxelGridSize), level);
	vec4 alphaData = textureLod(voxelAlpha, position/vec3(VoxelGridSize), level);
	float alpha = dot(alphaData.xyz, abs(dir)) / dot(vec3(1), abs(dir));
	return vec4(radianceData.rgb / (radianceData.a + 0.00001), alpha);
}

vec4 SampleVoxelLod(vec3 position, vec3 dir, float level)
{
	vec4 radianceData = textureLod(voxelRadiance, position/vec3(VoxelGridSize), level);
	vec4 alphaData = textureLod(voxelAlpha, position/vec3(VoxelGridSize), level);
	float alpha = dot(alphaData.xyz, abs(dir)) / dot(vec3(1), abs(dir));
	return vec4(radianceData.rgb / (radianceData.a + 0.00001), alpha);
}

float TraceVoxelGrid(int level, vec3 rayPos, vec3 rayDir, out vec3 hitColor, out vec3 hitRadiance)
{
	float scale = float(1 << level);
	float invScale = 1.0 / scale;

	vec3 voxelVolumeMax = vec3(voxelSize * VoxelGridSize * 0.5);
	vec3 voxelVolumeMin = -voxelVolumeMax;

	rayPos -= voxelGridCenter;

	float tmin, tmax;
	aabbIntersectFast(rayPos, rayDir, 1.0 / rayDir, voxelVolumeMin, voxelVolumeMax, tmin, tmax);

	if (tmin > tmax || tmax < 0)
	{
		hitColor = vec3(0);
		hitRadiance = vec3(0);
        return 0;
	}

	if (tmin > 0)
	{
		rayPos += rayDir * tmin;
	}

	vec3 voxelPos = (rayPos.xyz / voxelSize + VoxelGridSize * 0.5) * invScale;
	ivec3 voxelIndex = ivec3(voxelPos);

	vec3 deltaDist = abs(vec3(scale) / rayDir);
	vec3 raySign = sign(rayDir);
	ivec3 rayStep = ivec3(raySign);

	// Distance to next voxel in each axis
	vec3 sideDist = (raySign * (vec3(voxelIndex) - voxelPos) + (raySign * 0.5) + 0.5) * deltaDist;
	int maxIterations = int((VoxelGridSize * 3) * invScale);
	bvec3 mask;

	for (int i = 0; i < maxIterations; i++)
	{
		vec3 color;
		vec3 radiance;
		vec3 alpha = GetVoxel(voxelPos, level, color, radiance);
		float dirAlpha = dot(alpha, abs(rayDir)) / dot(vec3(1), abs(rayDir));
		if (dirAlpha > 0)
		{
			hitColor = color;
			hitRadiance = radiance;
			return dirAlpha;
		}

		// Find axis with minimum distance
		mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
		voxelPos += rayDir;// * dot(vec3(mask), sideDist);
		voxelIndex += ivec3(mask) * rayStep;
		sideDist = (raySign * (vec3(voxelIndex) - voxelPos) + (raySign * 0.5) + 0.5) * deltaDist;
	}

	hitColor = vec3(0);
	hitRadiance = vec3(0);
    return 0;
}

vec4 ConeTraceGrid(float ratio, vec3 rayPos, vec3 rayDir, vec3 surfaceNormal)
{
	vec3 voxelPos = (rayPos.xyz - voxelGridCenter) / voxelSize + VoxelGridSize * 0.5;
	float dist = max(1.75, min(1.75 / dot(rayDir, surfaceNormal), VoxelGridSize / 2));
	float maxDist = VoxelGridSize * 1.5;

	vec4 result = vec4(0);

	while (dist < maxDist && result.a < 0.9999)
	{
		float size = max(1.0, ratio * dist);
		float planeDist = dot(surfaceNormal, rayDir * dist) - 1.75;
		// If the sample intersects the surface, move it over
		float offset = max(0, size - planeDist);
		vec3 position = voxelPos + rayDir * dist + offset * surfaceNormal;
		vec4 value = SampleVoxel(position, rayDir, size);
		float alphaBias = 0.5 - smoothstep(1.0, 3.0, size) * 0.5;
		result += vec4(value.rgb * value.a, value.a) * (1.0 - result.a + alphaBias) * (1 - step(0, -value.a));

		if (CheckVoxel(position + rayDir * size * 4, size * 8)) {
			dist += size;
		} else {
			dist += size * 4;
		}
	}

	return vec4(result.rgb / (result.a + 0.00001), result.a);
}

vec4 ConeTraceGridDiffuse(vec3 rayPos, vec3 rayDir, vec3 surfaceNormal)
{
	vec3 voxelPos = (rayPos.xyz - voxelGridCenter) / voxelSize + VoxelGridSize * 0.5;
	float startDist = max(1.75, min(1.75 / dot(rayDir, surfaceNormal), VoxelGridSize / 2));
	float dist = startDist;
	float maxDist = VoxelGridSize * 1.5;

	vec4 result = vec4(0);
	float level = 0;

	while (dist < maxDist && result.a < 0.9999)
	{
		vec4 value = SampleVoxelLod(voxelPos + rayDir * dist, rayDir, level);
		result += vec4(value.rgb * value.a, value.a) * (1.0 - result.a) * (1 - step(0, -value.a));

		level += 1.0;
		dist *= 2.0;
	}

	return result;
}

#endif
#endif
