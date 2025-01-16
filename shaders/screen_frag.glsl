#version 460 core
out vec4 FragColor;
uniform sampler2D tex;
in vec2 UVs;
void main()
{
	FragColor = vec4(texture(tex, UVs).xyz, 1.0);
}