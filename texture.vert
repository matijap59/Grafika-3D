#version 330 core 

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTex; //Koordinate texture, propustamo ih u FS kao boje
out vec2 chTex;

layout(location = 2) in vec3 inCol;
out vec3 chCol;

void main()
{
	gl_Position = vec4(inPos.x, inPos.y, 0.0, 1.0);
	chTex = inTex;
	chCol = inCol;
}