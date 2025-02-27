#version 460

#ifdef H_VERT
	layout (location = 0) in vec3 pos;
	layout (location = 1) in vec2 uvs;
	out vec2 UVs;
	void main()
	{
		gl_Position = vec4(pos, 1.0);
		UVs = uvs;
	}
#endif

#ifdef H_FRAG
	out vec4 FragColor;
	uniform sampler2D tex;
	in vec2 UVs;
	void main()
	{
		FragColor = vec4(texture(tex, UVs).xyz, 1.0);
	}
#endif
