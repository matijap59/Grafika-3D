#version 330 core

//in vec2 chTex; //koordinate teksture
//out vec4 outCol;

//uniform sampler2D uTex; //teksturna jedinica

//void main()
//{
//	outCol = texture(uTex, chTex); 
//}

in vec2 chTex; // Koordinate teksture
out vec4 outCol;

uniform sampler2D uTex;      // Tekstura
uniform vec4 overlayColor;   // Zelena boja sa prozirnošæu (RGBA)

void main()
{
    // Uèitaj boju iz teksture
    vec4 texColor = texture(uTex, chTex);

    // Kombinuj boju teksture sa poluprovidnom zelenom
    outCol = mix(texColor, overlayColor, overlayColor.a);
}
