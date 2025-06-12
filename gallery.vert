#version 330 core

layout (location = 0) in vec3 aPos;  // Pozicija verteksa
layout (location = 1) in vec4 aColor; // Boja verteksa

out vec4 vColor; // Izlazna boja za fragment šejder

uniform mat4 uM; // Model matrica
uniform mat4 uV; // View matrica
uniform mat4 uP; // Projection matrica

void main()
{
    gl_Position = uP * uV * uM * vec4(aPos, 1.0); // Konaèna pozicija
    vColor = aColor; // Prosleðivanje boje
}