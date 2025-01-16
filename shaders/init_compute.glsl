#version 460

#define CELL 8
#define N 64

layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D parts;
layout(std430, binding = 3) buffer attractor
{
	ivec2 cells[N*N];
	uint start_idx[CELL*CELL];
	float values[];
};

#define PI 3.141592653589793

float random (vec2 uv)
{
    return (fract(sin(dot(uv,vec2(12.9898,78.233)))*43758.5453123 ));
}

uint hashCell(vec2 p){
	vec2 cell_sz = vec2(values[5], values[6]) / CELL;
	ivec2 ip = ivec2(p / cell_sz);
	return (ip.x * 1663 + ip.y * 7759) % (CELL*CELL);
}

uint particleLinearIdx(ivec2 idx2){
	return idx2.x * N + idx2.y; 
}

void main(void){
	ivec2 p_idx = ivec2(gl_GlobalInvocationID.xy);
	vec2 uv = vec2((1.0 * p_idx.x) / imageSize(parts).x, (1.0 * p_idx.y) / imageSize(parts).y);

	
	vec2 randomPos = vec2(random(uv.xy) * values[5],random(uv.yx) * values[6]);
	imageStore(parts, p_idx, vec4(randomPos, 0.0,0.0));

	cells[particleLinearIdx(p_idx)] = ivec2(hashCell(randomPos), particleLinearIdx(p_idx));
}