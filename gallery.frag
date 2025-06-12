#version 330 core

in vec4 vColor; // Ulazna boja iz verteks šejdera

out vec4 FragColor; // Izlazna boja piksela

void main()
{
    FragColor = vColor; // Koristi prosleðenu boju
}