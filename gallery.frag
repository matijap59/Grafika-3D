#version 330 core

in vec4 vColor; // Ulazna boja iz verteks �ejdera

out vec4 FragColor; // Izlazna boja piksela

void main()
{
    FragColor = vColor; // Koristi prosle�enu boju
}