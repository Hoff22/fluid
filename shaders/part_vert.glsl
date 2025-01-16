#version 460 core

#define CELL 8
#define N 64

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uvs;
uniform sampler2D parts;
out vec2 UVs;
flat out int instanceID;
layout(std430, binding = 3) buffer attractor
{
	ivec2 cells[N*N];
	uint start_idx[CELL*CELL];
	float values[];
};
void main()
{
	float particle_size = values[7] / 10;
	ivec2 dims = textureSize(parts, 0);
	vec2 particle_uv_idx = vec2((2.0*(gl_InstanceID % dims.x)+1) / (2*dims.x) , (2.0*gl_InstanceID / dims.y+1) / (2*dims.y));
	
	vec2 particle_pos = texture(parts, particle_uv_idx).xy;

	particle_pos = vec2(particle_pos.x / values[5], particle_pos.y / values[6]);
	particle_pos = particle_pos * 2.0 - vec2(1.0);
	
	vec2 scr = vec2(values[5], values[6]);
	float f = scr.x / scr.y;

	gl_Position = vec4(pos * particle_size * vec3(1.0,f,1.0)  + vec3(particle_pos,0.0), 1.0);
	
	instanceID = gl_InstanceID;
	UVs = uvs;
}