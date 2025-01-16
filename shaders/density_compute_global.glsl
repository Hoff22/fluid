#version 460

#define CELL 8
#define N 64

layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D parts;
layout(rgba32f, binding = 4) uniform image2D density;
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

void main(void){
	ivec2 dims = imageSize(parts);
	ivec2 p_idx = ivec2(gl_GlobalInvocationID.xy);

	vec2 scr = vec2(values[5], values[6]);
	vec2 scr_px = imageSize(density);
	
	vec2 f = scr / scr_px;

	

	vec2 g = vec2(0.0);
	float d = 0.0;
	float mass = 1;
	float current_dens = imageLoad(density, p_idx).z;

	vec2 pixel_pos = vec2(p_idx.x, p_idx.y) * f; 
	vec2 cell_sz = scr / (CELL);
	ivec2 cell_pos = ivec2(pixel_pos / cell_sz);

	for(int i = -1; i <= 1; i++){
		for(int j = -1; j<=1; j++){
			if(i + cell_pos.x < 0 || j + cell_pos.y < 0 || j + cell_pos.y >= CELL ||  i + cell_pos.x >= CELL) continue;

			ivec2 cur_cell = cell_pos + ivec2(i,j);

			uint str = start_idx[hashCell(cur_cell)];

			for(uint k = str; k < N*N; k++){
				if(k > str && cells[k-1].x != cells[k].x) break;
				ivec2 other_idx = particleCoordinateIdx(cells[k].y);
				vec4 p_cur = imageLoad(parts, other_idx);
				float dst = length(pixel_pos.xy / scr  - p_cur.xy / scr);
				
				// density calc
				d += smoothKernel(values[11], dst) * mass ;
				//                0.2

				// gradient calc
				float dens = imageLoad(density, ivec2(p_cur.xy / f)).z;
				vec2 dir = dst > 0.000001 ? normalize(pixel_pos.xy - p_cur.xy) : randomDir(p_cur.xy);
				// g += dir * 0.00001;
				g += -sharedPressure(current_dens, dens) * dir * smoothKernelDeriv(values[12],dst) * mass / (dens + 0.0000001);
				// g += dir;
			}
		}
	}

	// for(int i = 0; i < dims.y; i++){
	// 	for(int j = 0; j < dims.x; j++){
	// 		vec4 p_cur = imageLoad(parts, ivec2(j,i));
	// 		float dst = length(pixel_pos.xy / scr  - p_cur.xy / scr);
			
	// 		// density calc
	// 		d += smoothKernel(values[11], dst) * mass ;
	// 		//                0.2

	// 		// gradient calc
	// 		float dens = imageLoad(density, ivec2(p_cur.xy / f)).z;
	// 		vec2 dir = dst > 0.000001 ? normalize(pixel_pos.xy - p_cur.xy) : randomDir(p_cur.xy);
	// 		// g += dir * 0.00001;
	// 		g += -sharedPressure(current_dens, dens) * dir * smoothKernelDeriv(values[12],dst) * mass / (dens + 0.0000001);
	// 		// g += dir;

	// 	}
	// }

	float a = ((current_dens - values[9]) / values[9]) * 0.5 + 0.5;

	vec3 pixel_color = vec3(a, 1.0-a, d / (dims.x*dims.y));
	pixel_color = vec3(g, d / (dims.x*dims.y));
	// pixel_color = vec3(0.0,0.0, d / (dims.x*dims.y));
	// pixel_color = vec3(vec2(start_idx[hashCell(cell_pos)] == 1012 ? : 1.0, 0.0), 0.0);
	// if(start_idx[hashCell(cell_pos)] == 1012) pixel_color = vec3(1.0);
	// else pixel_color = vec3(0.0);

	imageStore(density, p_idx, vec4(pixel_color, 1.0) );
}