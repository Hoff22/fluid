#version 460

#define CELL 8
#define N 64

layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D parts;
layout(rgba32f, binding = 1) uniform image2D dens_grad;
layout(std430, binding = 3) buffer attractor
{
	ivec2 cells[N*N];
	uint start_idx[CELL*CELL];
	float values[];
};

#define PI 3.141592653589793

uint hashCell(vec2 p){
	vec2 cell_sz = vec2(values[5], values[6]) / CELL;
	ivec2 ip = ivec2(p / cell_sz);
	return (ip.x * 1663 + ip.y * 7759) % (CELL*CELL);
}

uint hashCell(ivec2 ip){
	return (ip.x * 1663 + ip.y * 7759) % (CELL*CELL);
}

uint particleLinearIdx(ivec2 idx2){
	return idx2.x * N + idx2.y; 
}
ivec2 particleCoordinateIdx(uint idx){
	return ivec2(idx / N, idx % N); 
}

float smoothKernel(float radius, float dst){
	if(dst >= radius) return 0;
	float volume = PI * pow(radius, 4) / 6;
	float value = max(0, (radius - dst) * (radius - dst));
	return value / volume;
}

float smoothKernelDeriv(float radius, float dst){
	if(dst >= radius) return 0;
	float f = dst - radius;
	float scale = 12 / (PI * pow(radius, 4));
	return scale * f;
}

float pressure(float dens){
	float densityError = dens - values[9]; // 1.0
	return densityError * values[10]; // 1.2
}

float sharedPressure(float dens1, float dens2){
	return (pressure(dens1) + pressure(dens2)) / 2;
}

float random (vec2 uv)
{
    return (fract(sin(dot(uv,vec2(12.9898,78.233)))*43758.5453123) * 2.0 - 1.0);
}

vec2 randomDir(vec2 uv){
	return normalize(vec2(random(uv.xy), random(uv.yx)));
}

float compute_density(vec2 pos, ivec2 idx){
	vec2 scr = vec2(values[5], values[6]);
	vec2 cell_sz = scr / CELL;
	ivec2 cell_pos = ivec2(pos / cell_sz);
	float d = 0.0;
	float mass = 1;
	for(int i = -1; i <= 1; i++){
		for(int j = -1; j<=1; j++){
			if(i + cell_pos.x < 0 || j + cell_pos.y < 0 || j + cell_pos.y >= CELL ||  i + cell_pos.x >= CELL) continue;

			ivec2 cur_cell = cell_pos + ivec2(i,j);

			uint str = start_idx[hashCell(cur_cell)];

			for(uint k = str; k < N*N; k++){
				if(k > str && cells[k-1].x != cells[k].x) break;
				ivec2 other_idx = particleCoordinateIdx(cells[k].y);
				if(other_idx == idx) continue;
				vec4 p_cur = imageLoad(parts, other_idx);
				vec2 other_pos = p_cur.xy + p_cur.zw * 1 / 60.0;
				float dst = length(pos / scr  - other_pos / scr);
				d += smoothKernel(values[11], dst) * mass ;
			}
		}
	}
	return d;
}

vec2 compute_gradient(vec2 pos, float current_dens, ivec2 idx){
	vec2 scr = vec2(values[5], values[6]);
	vec2 cell_sz = scr / CELL;
	ivec2 cell_pos = ivec2(pos / cell_sz);
	vec2 g = vec2(0.0);
	float mass = 1;
	for(int i = -1; i <= 1; i++){
		for(int j = -1; j<=1; j++){
			if(i + cell_pos.x < 0 || j + cell_pos.y < 0 || j + cell_pos.y >= CELL ||  i + cell_pos.x >= CELL) continue;

			ivec2 cur_cell = cell_pos + ivec2(i,j);

			uint str = start_idx[hashCell(cur_cell)];

			for(uint k = str; k < N*N; k++){
				if(k > str && cells[k-1].x != cells[k].x) break;
				ivec2 other_idx = particleCoordinateIdx(cells[k].y);
				if(other_idx == idx) continue;
				vec4 p_cur = imageLoad(parts, other_idx);
				vec2 other_pos = p_cur.xy + p_cur.zw * 1/60.0;
				float dst = length(pos / scr  - other_pos / scr);
				float dens = imageLoad(dens_grad, other_idx).x;
				vec2 dir = dst > 0.000001 ? normalize(pos - other_pos) : randomDir(other_pos);
				g += -sharedPressure(current_dens, dens) * dir * smoothKernelDeriv(values[11],dst) * mass / (dens + 0.000001);
			}
		}
	}

	vec2 att_p = vec2(values[0], values[1]);
	float att_r = values[2];
	
	vec2 dir = normalize(pos-att_p);
	vec2 o_p = att_p + dir * att_r;
	float dst = length(pos / scr  - o_p / scr);
	g += -sharedPressure(current_dens, 2.0) * dir * smoothKernelDeriv(values[11],dst) * mass;

	return g;
}

void main(void){
	ivec2 dims = imageSize(parts);
	ivec2 p_idx = ivec2(gl_GlobalInvocationID.xy);
	float delta_step = values[4];

	vec4 p = imageLoad(parts, p_idx);

	vec2 p_pos = p.xy;

	float current_dens = imageLoad(dens_grad, p_idx).x;

	float d = compute_density(p_pos + p.zw * 1/60.0, p_idx);
	vec2 g = compute_gradient(p_pos + p.zw * 1/60.0, current_dens, p_idx);

	imageStore(dens_grad, p_idx, vec4(d / (dims.x*dims.y), g, 1.0) );
}