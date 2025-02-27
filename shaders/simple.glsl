#version 460

#ifdef H_VERT
    layout (location = 0) in vec2 vPos; 
    void main(void)
    {
        gl_Position = vec4(vPos.x, vPos.y,0.0,1.0);
    }
#endif

#ifdef H_FRAG
    out vec4 color;
    void main(void)
    {

        vec3 c1 = vec3(50.0/255.0,40.0/255.0,30.0/255.0);
        vec3 c2 = vec3(255.0/255, 240.0/255, 235.0/255);

        vec3 c = mix(c1, c2, 0.6);

        color = vec4(c,0.0);
    }
#endif

