#version 460 core

#define CELL 32
#define N 64

out vec4 FragColor;
uniform sampler2D parts;
uniform sampler2D dens_grad;
layout(std430, binding = 3) buffer attractor
{
	ivec2 cells[N*N];
	uint start_idx[CELL*CELL];
	float values[];
};
in vec2 UVs;
flat in int instanceID;

float smoothstep (float edge0, float edge1, float x) {
   // Scale, and clamp x to 0..1 range
   x = clamp((x - edge0) / (edge1 - edge0), 0, 1);

   return x * x * (3.0f - 2.0f * x);
}

uint hashCell(vec2 p){
	vec2 cell_sz = vec2(values[5], values[6]) / CELL;
	ivec2 ip = ivec2(p / cell_sz);
	return (ip.x * 1663 + ip.y * 7759) % (CELL*CELL);
}

uint particleLinearIdx(ivec2 idx2){
	return idx2.x * N + idx2.y; 
}

void main()
{
	// FragColor = texture(parts, UVs);
	ivec2 dims = textureSize(parts, 0);
	vec2 particle_uv_idx = vec2((2.0*(instanceID % dims.x)+1) / (2*dims.x) , (2.0*instanceID / dims.y+1) / (2*dims.y));
	
	float dst = length(UVs*2.0 - vec2(1.0));
	
	float d = texture(dens_grad, particle_uv_idx).x;
	
	vec4 p = texture(parts, particle_uv_idx);

	float distFromEdge = (1.0 - dst);
	
	float derivX = dFdx(distFromEdge);
	float derivY = dFdy(distFromEdge);
	float gradientLength = length(vec2(derivX, derivY));
	float thresholdWidth = 2.0 * gradientLength;
	float antialiasedCircle = clamp((distFromEdge / thresholdWidth) + 0.5, 0, 1);


	vec3 p_col = vec3(5*length(p.zw) / values[6], 0.0, d);

	vec2 att_p = vec2(values[0], values[1]);
	uint att_cell = hashCell(att_p);
	float att_r = values[2];
	float att_f = values[3];
	// if(hashCell(p.xy) == att_cell){
	// 	p_col = vec3(1.0,0.0,0.0);	
	// }

	// FragColor = vec4(length(p.zw) * 100, 0.0 , d,(1.0-factor) > 0.5 ? 1.0 : 0.0); 
	FragColor = vec4(p_col, mix(0.0, 1.0, antialiasedCircle) ); 
	// FragColor = vec4(p_col, mix(0.0, 1.0, 1.0-dst) ); 
}