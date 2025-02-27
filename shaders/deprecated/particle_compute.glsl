#version 460

#define CELL 32
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

uint particleLinearIdx(ivec2 idx2){
	return idx2.x * N + idx2.y; 
}

float random (vec2 uv)
{
    return (fract(sin(dot(uv,vec2(12.9898,78.233)))*43758.5453123 ));
}

float smoothKernel(float radius, float dst){
	float f = (max(radius - dst, 0) / radius);
	return 1.0-f*f;
}

void main(void){

	float gravity = -values[8] * values[6]; // 2.4
	float drag_f = values[13]; // 0.995;
	float mx_speed = values[6] * sqrt(2) ;

	ivec2 p_idx = ivec2(gl_GlobalInvocationID.xy);

	vec4 p = imageLoad(parts, p_idx);
	vec2 g = imageLoad(dens_grad, p_idx).yz;

	vec2 att_p = vec2(values[0], values[1]);
	float att_r = values[2];
	float att_f = values[3];
	float delta_step = values[4];

	vec2 acc = vec2(0.0,gravity);

	float dens = imageLoad(dens_grad, p_idx).x;

	acc += g / (dens + 0.00001);

	// attractor
	if(length(p.xy - att_p) < att_r && att_f > 0.00001){
		vec2 dir = (att_p - p.xy); 
		acc += dir * smoothKernel(att_r, length(p.xy - att_p)) * att_f * 10;
		// acc -= vec2(0.0,gravity);
		drag_f = 0.95;
	}

	vec2 newPos = p.xy + p.zw * delta_step;


	float drag = mix(0.9999, 0.995, dens * dens);

	vec2 newSpeed = p.zw * drag_f + acc * delta_step;
	
	newPos = clamp(newPos, vec2(0), vec2(values[5],values[6]));
	newSpeed = clamp(newSpeed, vec2(-mx_speed), vec2(mx_speed));
	
	// obstacle
	// float dst = length(p.xy - att_p);
	// if(dst < att_r){
	// 	vec2 normal = normalize(p.xy - att_p);
	// 	newPos = att_p + normal * dst * 1.1;
	// }
	
	cells[particleLinearIdx(p_idx)] = ivec2(hashCell(newPos), particleLinearIdx(p_idx));
	
	imageStore(parts, p_idx, vec4(newPos, newSpeed));
}