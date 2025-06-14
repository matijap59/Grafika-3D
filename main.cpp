#define _CRT_SECURE_NO_WARNINGS
 
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>

#include <GL/glew.h> 
#include <GLFW/glfw3.h>

//GLM biblioteke
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


const int TARGET_FPS = 60;
const double FRAME_TIME = 1.0 / TARGET_FPS;

unsigned int compileShader(GLenum type, const char* source);
unsigned int createShader(const char* vsSource, const char* fsSource);
void updateCameraVectors(); 
static unsigned loadImageToTexture(const char* filePath);


float verticeTexture[] = {
    //  x       y       u     v

    -1.0f,  0.6f,    0.0f, 0.0f,   // top-left
    -0.6f,  0.6f,    1.0f, 0.0f,   // top-right
    -0.6f,  1.0f,    1.0f, 1.0f,   // bottom-right
    -1.0f,  1.0f,    0.0f, 1.0f    // bottom-left
};


unsigned int indicesTexture[] = {
        0, 1, 2,
        0, 2, 3
};

unsigned int textureStride = (2 + 2) * sizeof(float);


glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.5f);     // Pocetna pozicija kamere (unutar kocke)
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // Pravac u kome kamera gleda (prema negativnoj Z osi)
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);       // Gornji vektor kamere
float yaw = -90.0f;   // Horizontalna rotacija kamere (pocetno gleda po negativnoj Z osi)
float pitch = 0.0f;   // Vertikalna rotacija kamere

float fov = 75.0f;  // Field of View za zumiranje


float cubeSize = 4.0f;
float h = cubeSize / 2.0f; // Pola velicine

//Globalne promenljive za stanje vrata
bool doorIsOpen = false;
float doorOpenProgress = 0.0f; // 0.0 = zatvorena, 1.0 = potpuno otvorena
float doorAnimationSpeed = 1.5f; // Brzina animacije vrata
bool spacePressedLastFrame = false; // Za detekciju samo jednog pritiska Space tastera

// Definisanje promenljivih za VRATA
float doorWidth = 1.4f;
float doorHeight = 2.5f;
float doorXOffset = 0.0f; // Centrirano po x osi
float doorYBase = -h;     // Donja ivica vrata na podu

float doorXOffset2 = 0.0f; // Centrirano po x osi
float newDoorGap = 0.05f;

// Definisanje promenljivih za slike
float pictureWidth = 1.0f;
float pictureHeight = 1.5f;
float pictureXOffset = 0.0f; // Centrirano po X osi (za sliku na zadnjem zidu)
float pictureYBase = -h + 0.8f; // Donja ivica slike na 0.8m od poda

// Za slike na bocnim zidovima, Z-offset ce biti 0.0f za centriranje
float picture2ZOffset = 0.0f; // Za sliku 2 na levom zidu
float picture3ZOffset = 0.0f; // Za sliku 3 na desnom zidu
float doorThickness = 0.02f;


float outsidePlaneSize = h * 10.0f; //10 puta veæa od galerije
glm::vec4 outsidePlaneColor = glm::vec4(0.5f, 0.7f, 0.5f, 1.0f);


int main(void)
{

   
    if (!glfwInit())
    {
        std::cout<<"GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    unsigned int wWidth = 1200;
    unsigned int wHeight = 800;
    const char wTitle[] = "[Galerija]";
    window = glfwCreateWindow(wWidth, wHeight, wTitle, NULL, NULL);
    
    if (window == NULL)
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate();
        return 2;
    }
    
    glfwMakeContextCurrent(window);

    
    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        return 3;
    }


    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // Fragment je bliži ako ima manju dubinu

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK); // Eliminiše zadnje strane

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ PROMJENLJIVE I BAFERI +++++++++++++++++++++++++++++++++++++++++++++++++

    unsigned int galleryShader = createShader("basic.vert", "basic.frag");

    float vertices[] =
    {
        // POD (Svetlo siva) - normala gleda gore (pozitivna Y)
        // Trougao 1
        -h, -h,  h,  0.7f, 0.7f, 0.7f, 1.0f, // 0 - DLT (Donji Levi Prednji)
         h, -h,  h,  0.7f, 0.7f, 0.7f, 1.0f, // 1 - DRT (Donji Desni Prednji)
         h, -h, -h,  0.7f, 0.7f, 0.7f, 1.0f, // 2 - DRZ (Donji Desni Zadnji)
         // Trougao 2
         -h, -h,  h,  0.7f, 0.7f, 0.7f, 1.0f, // 0 - DLT
          h, -h, -h,  0.7f, 0.7f, 0.7f, 1.0f, // 2 - DRZ
         -h, -h, -h,  0.7f, 0.7f, 0.7f, 1.0f, // 3 - DLZ (Donji Levi Zadnji)

         // PLAFON (Tamno siva) - normala gleda dole (negativna Y) - CW za unutrašnjost
         // Trougao 1
         -h,  h, -h,  0.5f, 0.5f, 0.5f, 1.0f, // 4 - GLZ (Gornji Levi Zadnji)
          h,  h, -h,  0.5f, 0.5f, 0.5f, 1.0f, // 5 - GRZ (Gornji Desni Zadnji)
          h,  h,  h,  0.5f, 0.5f, 0.5f, 1.0f, // 6 - GRT (Gornji Desni Prednji)
          // Trougao 2
          -h,  h, -h,  0.5f, 0.5f, 0.5f, 1.0f, // 4 - GLZ
           h,  h,  h,  0.5f, 0.5f, 0.5f, 1.0f, // 6 - GRT
          -h,  h,  h,  0.5f, 0.5f, 0.5f, 1.0f, // 7 - GLT (Gornji Levi Prednji)

          // ZADNJI ZID (Svetlo plava) - normala gleda napred (pozitivna Z) - CW za unutrašnjost
          // ZADNJI ZID (Svetlo plava) - normala gleda napred (pozitivna Z) - CW za unutrašnjost
          // Trougao 1
          -h, -h, -h, 1.0f, 0.992f, 0.902f, 1.0f, // 3 - DLZ
           h, -h, -h,  1.0f, 0.992f, 0.902f, 1.0f, // 2 - DRZ
           h,  h, -h,  1.0f, 0.992f, 0.902f, 1.0f, // 5 - GRZ
           // Trougao 2
           -h, -h, -h,  1.0f, 0.992f, 0.902f, 1.0f, // 3 - DLZ
            h,  h, -h,  1.0f, 0.992f, 0.902f, 1.0f, // 5 - GRZ
           -h,  h, -h,  1.0f, 0.992f, 0.902f, 1.0f, // 4 - GLZ

           // PREDNJI ZID (Svetlo zelena) - normala gleda nazad (negativna Z) - CCW za unutrašnjost
           // Trougao 1
           -h,  h,  h,  1.0f, 0.992f, 0.902f, 1.0f, // 7 - GLT
            h,  h,  h, 1.0f, 0.992f, 0.902f, 1.0f, // 6 - GRT
            h, -h,  h,  1.0f, 0.992f, 0.902f, 1.0f, // 1 - DRT
            // Trougao 2
            -h,  h,  h,  1.0f, 0.992f, 0.902f, 1.0f, // 7 - GLT
             h, -h,  h,  1.0f, 0.992f, 0.902f, 1.0f, // 1 - DRT
            -h, -h,  h,  1.0f, 0.992f, 0.902f, 1.0f, // 0 - DLT

            // DESNI ZID (Narandžasta) - normala gleda levo (negativna X) - CW za unutrašnjost
            // Trougao 1
             h, -h,  h,   1.0f, 0.992f, 0.902f, 1.0f, // 1 - DRT
             h,  h,  h,   1.0f, 0.992f, 0.902f, 1.0f, // 6 - GRT
             h,  h, -h,   1.0f, 0.992f, 0.902f, 1.0f, // 5 - GRZ
             // Trougao 2
              h, -h,  h,   1.0f, 0.992f, 0.902f, 1.0f, // 1 - DRT
              h,  h, -h,   1.0f, 0.992f, 0.902f, 1.0f, // 5 - GRZ
              h, -h, -h,  1.0f, 0.992f, 0.902f, 1.0f, // 2 - DRZ

              // LEVI ZID (Magenta) - normala gleda desno (pozitivna X) - CCW za unutrašnjost
              // Trougao 1
              -h, -h, -h,  1.0f, 0.992f, 0.902f, 1.0f, // 3 - DLZ
              -h,  h, -h,  1.0f, 0.992f, 0.902f, 1.0f, // 4 - GLZ
              -h,  h,  h,  1.0f, 0.992f, 0.902f, 1.0f, // 7 - GLT
              // Trougao 2
              -h, -h, -h,  1.0f, 0.992f, 0.902f, 1.0f, // 3 - DLZ
              -h,  h,  h,  1.0f, 0.992f, 0.902f, 1.0f, // 7 - GLT
              -h, -h,  h, 1.0f, 0.992f, 0.902f, 1.0f,  // 0 - DLT

                      //// VRATA (Braon) - UNUTRAŠNJA STRANA I SPOLJAŠNJA STRANA (formiraju debljinu vrata)
            //// Unutrašnja strana vrata (dalja od tebe, dublje u zidu) - normala gleda ka +Z
            // Redosled verteksa treba da bude CCW kada se gleda sa Z+ strane (od zida, ka kameri unutar galerije)
            doorXOffset - doorWidth / 2.0f, doorYBase + doorHeight,  h - 0.05f - doorThickness, 0.6f, 0.4f, 0.2f, 1.0f, // Gornji levi
            doorXOffset + doorWidth / 2.0f, doorYBase + doorHeight,  h - 0.05f - doorThickness, 0.6f, 0.4f, 0.2f, 1.0f, // Gornji desni
            doorXOffset + doorWidth / 2.0f, doorYBase,               h - 0.05f - doorThickness, 0.6f, 0.4f, 0.2f, 1.0f, // Donji desni
            doorXOffset - doorWidth / 2.0f, doorYBase + doorHeight,  h - 0.05f - doorThickness, 0.6f, 0.4f, 0.2f, 1.0f, // Gornji levi
            doorXOffset + doorWidth / 2.0f, doorYBase,               h - 0.05f - doorThickness, 0.6f, 0.4f, 0.2f, 1.0f, // Donji desni
            doorXOffset - doorWidth / 2.0f, doorYBase,               h - 0.05f - doorThickness, 0.6f, 0.4f, 0.2f, 1.0f, // Donji levi

            // Spoljašnja strana braon vrata (bliža tebi, ali unutar debljine zida) - normala gleda ka -Z
            // Redosled verteksa treba da bude CCW kada se gleda sa Z- strane (od kamere, ka zidu)
            doorXOffset - doorWidth / 2.0f, doorYBase + doorHeight,  h - 0.05f, 0.6f, 0.4f, 0.2f, 1.0f, // Gornji levi
            doorXOffset + doorWidth / 2.0f, doorYBase,               h - 0.05f, 0.6f, 0.4f, 0.2f, 1.0f, // Donji desni
            doorXOffset + doorWidth / 2.0f, doorYBase + doorHeight,  h - 0.05f, 0.6f, 0.4f, 0.2f, 1.0f, // Gornji desni
            doorXOffset - doorWidth / 2.0f, doorYBase + doorHeight,  h - 0.05f, 0.6f, 0.4f, 0.2f, 1.0f, // Gornji levi
            doorXOffset - doorWidth / 2.0f, doorYBase,               h - 0.05f, 0.6f, 0.4f, 0.2f, 1.0f, // Donji levi
            doorXOffset + doorWidth / 2.0f, doorYBase,               h - 0.05f, 0.6f, 0.4f, 0.2f, 1.0f, // Donji desni

            doorXOffset - doorWidth / 2.0f, doorYBase + doorHeight,  h - 0.05f,  0.0f, 0.0f, 1.0f, 1.0f, // Gornji levi
            doorXOffset + doorWidth / 2.0f, doorYBase + doorHeight,  h - 0.05f,  0.0f, 0.0f, 1.0f, 1.0f, // Gornji desni
            doorXOffset + doorWidth / 2.0f, doorYBase,               h - 0.05f,  0.0f, 0.0f, 1.0f, 1.0f, // Donji desni
            doorXOffset - doorWidth / 2.0f, doorYBase + doorHeight,  h - 0.05f,  0.0f, 0.0f, 1.0f, 1.0f, // Gornji levi
            doorXOffset + doorWidth / 2.0f, doorYBase,               h - 0.05f, 0.0f, 0.0f, 1.0f, 1.0f, // Donji desni
            doorXOffset - doorWidth / 2.0f, doorYBase,               h - 0.05f,  0.0f, 0.0f, 1.0f, 1.0f, // Donji levi

              // SLIKA 1 (Crna, na ZADNJEM zidu) - normala gleda napred (pozitivna Z)
        // Trougao 1 (Gornji levi, Donji desni, Gornji desni)
        pictureXOffset - pictureWidth / 2.0f, pictureYBase + pictureHeight, -h + 0.05f, 0.3f, 0.3f, 1.0f, 1.0f, // Gornji levi (1)
        pictureXOffset + pictureWidth / 2.0f, pictureYBase,               -h + 0.05f, 0.3f, 0.3f, 1.0f, 1.0f, // Donji desni (3)
        pictureXOffset + pictureWidth / 2.0f, pictureYBase + pictureHeight, -h + 0.05f, 0.3f, 0.3f, 1.0f, 1.0f, // Gornji desni (2)
        // Trougao 2 (Gornji levi, Donji levi, Donji desni)
        pictureXOffset - pictureWidth / 2.0f, pictureYBase + pictureHeight, -h + 0.05f, 0.3f, 0.3f, 1.0f, 1.0f, // Gornji levi (1)
        pictureXOffset - pictureWidth / 2.0f, pictureYBase,               -h + 0.05f, 0.3f, 0.3f, 1.0f, 1.0f, // Donji levi (4)
        pictureXOffset + pictureWidth / 2.0f, pictureYBase,               -h + 0.05f, 0.3f, 0.3f, 1.0f, 1.0f, // Donji desni (3)

        // SLIKA 2 (Crna, na LEVOM zidu) - normala gleda desno (pozitivna X)
        // Trougao 1 (Gornji prednji, Donji zadnji, Gornji zadnji)
        -h + 0.05f, pictureYBase + pictureHeight, picture2ZOffset + pictureWidth / 2.0f,  1.0f, 0.6f, 0.0f, 1.0f, // Gornji prednji (1)
        -h + 0.05f, pictureYBase,               picture2ZOffset - pictureWidth / 2.0f, 1.0f, 0.6f, 0.0f, 1.0f, // Donji zadnji (3)
        -h + 0.05f, pictureYBase + pictureHeight, picture2ZOffset - pictureWidth / 2.0f, 1.0f, 0.6f, 0.0f, 1.0f, // Gornji zadnji (2)
        // Trougao 2 (Gornji prednji, Donji prednji, Donji zadnji)
        -h + 0.05f, pictureYBase + pictureHeight, picture2ZOffset + pictureWidth / 2.0f, 1.0f, 0.6f, 0.0f, 1.0f, // Gornji prednji (1)
        -h + 0.05f, pictureYBase,               picture2ZOffset + pictureWidth / 2.0f, 1.0f, 0.6f, 0.0f, 1.0f, // Donji prednji (4)
        -h + 0.05f, pictureYBase,               picture2ZOffset - pictureWidth / 2.0f, 1.0f, 0.6f, 0.0f, 1.0f, // Donji zadnji (3)

        // SLIKA 3 (Crna, na DESNOM zidu) - normala gleda levo (negativna X)
        // Trougao 1 (Gornji zadnji, Donji prednji, Gornji prednji)
         h - 0.05f, pictureYBase + pictureHeight, picture3ZOffset - pictureWidth / 2.0f, 1.0f, 0.0f, 1.0f, 1.0f, // Gornji zadnji (1)
         h - 0.05f, pictureYBase,               picture3ZOffset + pictureWidth / 2.0f, 1.0f, 0.0f, 1.0f, 1.0f, // Donji prednji (3)
         h - 0.05f, pictureYBase + pictureHeight, picture3ZOffset + pictureWidth / 2.0f, 1.0f, 0.0f, 1.0f, 1.0f, // Gornji prednji (2)
         // Trougao 2 (Gornji zadnji, Donji zadnji, Donji prednji)
          h - 0.05f, pictureYBase + pictureHeight, picture3ZOffset - pictureWidth / 2.0f, 1.0f, 0.0f, 1.0f, 1.0f, // Gornji zadnji (1)
          h - 0.05f, pictureYBase,               picture3ZOffset - pictureWidth / 2.0f, 1.0f, 0.0f, 1.0f, 1.0f, // Donji zadnji (4)
          h - 0.05f, pictureYBase,               picture3ZOffset + pictureWidth / 2.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // Donji prednji

                  -outsidePlaneSize, -h-0.05f, -outsidePlaneSize, outsidePlaneColor.r, outsidePlaneColor.g, outsidePlaneColor.b, outsidePlaneColor.a, // Donji Levi Zadnji
                  outsidePlaneSize, -h-0.05f, outsidePlaneSize, outsidePlaneColor.r, outsidePlaneColor.g, outsidePlaneColor.b, outsidePlaneColor.a, // Donji Desni Zadnji
                  outsidePlaneSize, -h-0.05f, -outsidePlaneSize, outsidePlaneColor.r, outsidePlaneColor.g, outsidePlaneColor.b, outsidePlaneColor.a, // Donji Desni Prednji

                  -outsidePlaneSize, -h-0.05f, -outsidePlaneSize, outsidePlaneColor.r, outsidePlaneColor.g, outsidePlaneColor.b, outsidePlaneColor.a, // Donji Levi Zadnji
                  -outsidePlaneSize, -h-0.05f, outsidePlaneSize, outsidePlaneColor.r, outsidePlaneColor.g, outsidePlaneColor.b, outsidePlaneColor.a, // Donji Desni Prednji
                  outsidePlaneSize, -h-0.05f, outsidePlaneSize, outsidePlaneColor.r, outsidePlaneColor.g, outsidePlaneColor.b, outsidePlaneColor.a,  // Donji Levi Prednji

               // NOVI TROUGLOVI ZA SPOLJAŠNJU STRANU GALERIJE
        // SPOLJNI PLAFON (Tamno siva) - normala gleda gore (pozitivna Y) - CCW za spoljašnjost (isti kao unutrašnji pod)
        // Trougao 1
               -h, h, h, 0.5f, 0.5f, 0.5f, 1.0f, // Gornji levi (unutrašnji: 7 GLT)
               h, h, h, 0.5f, 0.5f, 0.5f, 1.0f, // Gornji desni (unutrašnji: 6 GRT)
               h, h, -h, 0.5f, 0.5f, 0.5f, 1.0f, // Donji desni (unutrašnji: 5 GRZ)
               // Trougao 2
               -h, h, h, 0.5f, 0.5f, 0.5f, 1.0f, // Gornji levi (unutrašnji: 7 GLT)
               h, h, -h, 0.5f, 0.5f, 0.5f, 1.0f, // Donji desni (unutrašnji: 5 GRZ)
               -h, h, -h, 0.5f, 0.5f, 0.5f, 1.0f, // Donji levi (unutrašnji: 4 GLZ)

               // SPOLJNI POD (Svetlo siva) - normala gleda dole (negativna Y) - CW za spoljašnjost (isti kao unutrašnji plafon)
               // Trougao 1
               -h, -h, -h, 0.7f, 0.7f, 0.7f, 1.0f, // DLZ (unutrašnji: 3 DLZ)
               h, -h, -h, 0.7f, 0.7f, 0.7f, 1.0f, // DRZ (unutrašnji: 2 DRZ)
               h, -h, h, 0.7f, 0.7f, 0.7f, 1.0f, // DRT (unutrašnji: 1 DRT)
               // Trougao 2
               -h, -h, -h, 0.7f, 0.7f, 0.7f, 1.0f, // DLZ (unutrašnji: 3 DLZ)
               h, -h, h, 0.7f, 0.7f, 0.7f, 1.0f, // DRT (unutrašnji: 1 DRT)
               -h, -h, h, 0.7f, 0.7f, 0.7f, 1.0f, // DLT (unutrašnji: 0 DLT)

               // SPOLJNI ZADNJI ZID (Svetlo plava) - normala gleda nazad (negativna Z) - CCW za spoljašnjost
               // Trougao 1
               -h, h, -h, 1.0f, 0.992f, 0.902f, 1.0f, // GLZ (unutrašnji: 4 GLZ)
               h, h, -h, 1.0f, 0.992f, 0.902f, 1.0f, // GRZ (unutrašnji: 5 GRZ)
               h, -h, -h, 1.0f, 0.992f, 0.902f, 1.0f, // DRZ (unutrašnji: 2 DRZ)
               // Trougao 2
               -h, h, -h, 1.0f, 0.992f, 0.902f, 1.0f, // GLZ (unutrašnji: 4 GLZ)
               h, -h, -h, 1.0f, 0.992f, 0.902f, 1.0f, // DRZ (unutrašnji: 2 DRZ)
               -h, -h, -h, 1.0f, 0.992f, 0.902f, 1.0f, // DLZ (unutrašnji: 3 DLZ)

               //// SPOLJNI PREDNJI ZID (Svetlo zelena) - normala gleda napred (pozitivna Z) - CW za spoljašnjost
               //// Trougao 1
               -h, -h, h, 1.0f, 0.992f, 0.902f, 1.0f, // DLT (unutrašnji: 0 DLT)
               h, -h, h, 1.0f, 0.992f, 0.902f, 1.0f, // DRT (unutrašnji: 1 DRT)
               h, h, h, 1.0f, 0.992f, 0.902f, 1.0f, // GRT (unutrašnji: 6 GRT)
               // Trougao 2
               -h, -h, h, 1.0f, 0.992f, 0.902f, 1.0f, // DLT (unutrašnji: 0 DLT)
               h, h, h, 1.0f, 0.992f, 0.902f, 1.0f, // GRT (unutrašnji: 6 GRT)
               -h, h, h, 1.0f, 0.992f, 0.902f, 1.0f, // GLT (unutrašnji: 7 GLT)

               // SPOLJNI DESNI ZID (Narandžasta) - normala gleda desno (pozitivna X) - CCW za spoljašnjost
               // Trougao 1
               h, -h, -h, 1.0f, 0.992f, 0.902f, 1.0f, // DRZ (unutrašnji: 2 DRZ)
               h, h, -h, 1.0f, 0.992f, 0.902f, 1.0f, // GRZ (unutrašnji: 5 GRZ)
               h, h, h, 1.0f, 0.992f, 0.902f, 1.0f, // GRT (unutrašnji: 6 GRT)
               // Trougao 2
               h, -h, -h, 1.0f, 0.992f, 0.902f, 1.0f, // DRZ (unutrašnji: 2 DRZ)
               h, h, h, 1.0f, 0.992f, 0.902f, 1.0f, // GRT (unutrašnji: 6 GRT)
               h, -h, h, 1.0f, 0.992f, 0.902f, 1.0f, // DRT (unutrašnji: 1 DRT)

               // SPOLJNI LEVI ZID (Magenta) - normala gleda levo (negativna X) - CW za spoljašnjost
               // Trougao 1
               -h, -h, h, 1.0f, 0.992f, 0.902f, 1.0f, // DLT (unutrašnji: 0 DLT)
               -h, h, h, 1.0f, 0.992f, 0.902f, 1.0f, // GLT (unutrašnji: 7 GLT)
               -h, h, -h, 1.0f, 0.992f, 0.902f, 1.0f, // GLZ (unutrašnji: 4 GLZ)
               // Trougao 2
               -h, -h, h, 1.0f, 0.992f, 0.902f, 1.0f, // DLT (unutrašnji: 0 DLT)
               -h, h, -h, 1.0f, 0.992f, 0.902f, 1.0f, // GLZ (unutrašnji: 4 GLZ)
               -h, -h, -h, 1.0f, 0.992f, 0.902f, 1.0f,  // DLZ (unutrašnji: 3 DLZ)

    };


    unsigned int numGalleryVertices = 6 * 6; // Zidovi, pod, plafon
    unsigned int numDoorVertices = 2 * 6;
    unsigned int numPictureVertices = 4 * 6; // Tri slike

    unsigned int numOutsidePlaneVertices = 2 * 3; // Dva trougla, svaki po 3 verteksa = 6 verteksa

    unsigned int numGalleryExteriorVertices = 6 * 6; // Dodati trouglovi za spoljašnju stranu zidova, poda, plafona

    unsigned int totalVertices = numGalleryVertices + numDoorVertices + numPictureVertices + numOutsidePlaneVertices + numGalleryExteriorVertices;

    unsigned int stride = (3 + 4) * sizeof(float); 
    
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    unsigned int textureVAO;
    glGenVertexArrays(1, &textureVAO);
    glBindVertexArray(textureVAO);
    unsigned int textureVBO;
    glGenBuffers(1, &textureVBO);
    glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticeTexture), verticeTexture, GL_STATIC_DRAW);

    // da bude cetvorougao moramo spojiti 2 trougla
    unsigned int textureEBO;
    glGenBuffers(1, &textureEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textureEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesTexture), indicesTexture, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, textureStride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, textureStride, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    unsigned int textureShader = createShader("texture.vert", "texture.frag");

    unsigned checkerTexture = loadImageToTexture("ime.png");
    if (checkerTexture == 0) {
        std::cout << "Failed to load texture!" << std::endl;
        return -1;
    }
    glBindTexture(GL_TEXTURE_2D, checkerTexture); 
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);   
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(textureShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, checkerTexture);
    glUniform1i(glGetUniformLocation(textureShader, "uTex"), 0);


    glBindVertexArray(textureVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++            UNIFORME            +++++++++++++++++++++++++++++++++++++++++++++++++

    glm::mat4 model = glm::mat4(1.0f); //Matrica transformacija - mat4(1.0f) generise jedinicnu matricu (ne pomeramo je)
    unsigned int modelLoc = glGetUniformLocation(galleryShader, "uM");

    glm::mat4 view = glm::mat4(1.0f); // View matrica (pozicija i orijentacija kamere)
    unsigned int viewLoc = glGetUniformLocation(galleryShader, "uV");
    
    // Projekciona matrica (perspektivna projekcija)
    glm::mat4 projection = glm::perspective(glm::radians(fov), (float)wWidth / (float)wHeight, 0.0001f, 100.0f);
    unsigned int projectionLoc = glGetUniformLocation(galleryShader, "uP");


    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ RENDER LOOP - PETLJA ZA CRTANJE +++++++++++++++++++++++++++++++++++++++++++++++++
    glUseProgram(galleryShader); //Slanje default vrijednosti uniformi
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //(Adresa matrice, broj matrica koje saljemo, da li treba da se transponuju, pokazivac do matrica)
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(VAO);

    updateCameraVectors();

    double lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        auto fpsStartTime = std::chrono::high_resolution_clock::now();
        double currentFrameTime = glfwGetTime();
        double deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;


        float cameraMoveSpeed = 2.5f * deltaTime; // Brzina kretanja (m/s)
        float cameraRotationSpeed = 40.0f * deltaTime; // Brzina rotacije (stepeni/s)
        float zoomFovSpeed = 50.0f * deltaTime;

        glm::vec3 newCameraPos = cameraPos; // Izraèunavamo novu poziciju ovde

        // Kretanje napred/nazad (W/S)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            newCameraPos += cameraMoveSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            newCameraPos -= cameraMoveSpeed * cameraFront;

        // Kretanje levo/desno (strafe A/D)
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            newCameraPos -= cameraRight * cameraMoveSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            newCameraPos += cameraRight * cameraMoveSpeed;

        // Rotacija kamere levo/desno (Q/E) - menja YAW ugao
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            yaw -= cameraRotationSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            yaw += cameraRotationSpeed;
        }

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            fov -= zoomFovSpeed;
            if (fov < 1.0f) {
                fov = 1.0f;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            fov += zoomFovSpeed;
            if (fov > 90.0f) {
                fov = 90.0f;
            }
        }

        updateCameraVectors();

        // Logika za otvaranje/zatvaranje vrata
        bool currentSpacePressed = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
        if (currentSpacePressed && !spacePressedLastFrame) {
            doorIsOpen = !doorIsOpen;
        }
        spacePressedLastFrame = currentSpacePressed;

        // Izlaz iz aplikacije
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);

        // ----- DETEKCIJA KOLIZIJA -----
        float cameraRadius = 0.1f; // Radijus igraèa/kamere

        // Granice UNUTRAŠNJOSTI galerije (bez prednjeg zida gde su vrata)
        float interiorMinX = -h + cameraRadius;
        float interiorMaxX = h - cameraRadius;
        float interiorMinY = -h + cameraRadius;
        float interiorMaxY = h - cameraRadius;
        float interiorMinZ = -h + cameraRadius; // Zadnji zid
        float interiorMaxZ = h - cameraRadius;  // Prednji zid (unutrašnja strana)

        float planeMinX = -outsidePlaneSize + cameraRadius;
        float planeMaxX = outsidePlaneSize - cameraRadius;
        float planeMinZ = h - cameraRadius; // Malo unutar prednjeg zida (da se može proæi)
        float planeMaxZ = h + outsidePlaneSize * 2.0f - cameraRadius; // Kraj ravni
        float planeY = -h + cameraRadius; // Y pozicija ravni (visina poda)

        // Detekcija da li je kamera u "zoni vrata"
        // Zona vrata je širi pravougaonik oko samih vrata, po X i Y
        bool isCameraInDoorwayX = (newCameraPos.x >= (doorXOffset - doorWidth / 2.0f - cameraRadius)) &&
            (newCameraPos.x <= (doorXOffset + doorWidth / 2.0f + cameraRadius));
        bool isCameraInDoorwayY = (newCameraPos.y >= (doorYBase - cameraRadius)) &&
            (newCameraPos.y <= (doorYBase + doorHeight + cameraRadius));

        bool isCameraInDoorwayArea = isCameraInDoorwayX && isCameraInDoorwayY;

        // Progres otvaranja vrata: 1.0 je potpuno otvoreno
        bool doorIsSufficientlyOpen = (doorOpenProgress >= 0.95f); // 95% otvoreno smatramo otvorenim

        // Primeni kolizije
        // Prioritet: ako su vrata otvorena i u zoni smo vrata, dozvoli prolaz
        if (isCameraInDoorwayArea && doorIsSufficientlyOpen)
        {
            newCameraPos.y = glm::clamp(newCameraPos.y, planeY, interiorMaxY); // Ne dozvoli da propadne ispod ravni/poda, ali i da ne ode previsoko iznad plafona galerije

            // Z-kolizija: Omoguæi prolaz kroz Z=h zid.
            // X-kolizija: Može da ide preko X granica galerije, ali ne preko granica spoljašnje ravni
            newCameraPos.x = glm::clamp(newCameraPos.x, planeMinX, planeMaxX);

            // Z-kolizija: Može da ide preko Z granice galerije (prednjeg zida), ali ne preko granica spoljašnje ravni
            newCameraPos.z = glm::clamp(newCameraPos.z, interiorMinZ, planeMaxZ);
        }
        else // Vrata su zatvorena ILI kamera nije u zoni vrata
        {
            // Standardne kolizije unutar galerije
            newCameraPos.x = glm::clamp(newCameraPos.x, interiorMinX, interiorMaxX);
            newCameraPos.y = glm::clamp(newCameraPos.y, interiorMinY, interiorMaxY);
            newCameraPos.z = glm::clamp(newCameraPos.z, interiorMinZ, interiorMaxZ);
        }

        // Postavi poziciju kamere na izraèunatu i proverenu poziciju
        cameraPos = newCameraPos;


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Osvježavamo i Z bafer i bafer boje
        glEnable(GL_DEPTH_TEST);
        glUseProgram(galleryShader);
        glBindVertexArray(VAO);

        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        projection = glm::perspective(glm::radians(fov), (float)wWidth / (float)wHeight, 0.0001f, 100.0f);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));


        model = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, numGalleryVertices); // Crtaj zidove, pod, plafon (prvih 6*6=36 verteksa)

        glDrawArrays(GL_TRIANGLES, numGalleryVertices + numDoorVertices, numPictureVertices);


        glm::mat4 doorModel = glm::mat4(1.0f);

        if (doorIsOpen) {
            doorOpenProgress += doorAnimationSpeed * deltaTime;
            if (doorOpenProgress > 1.0f) doorOpenProgress = 1.0f; // Osiguraj da ne ode preko 1.0
        }
        else {
            doorOpenProgress -= doorAnimationSpeed * deltaTime;
            if (doorOpenProgress < 0.0f) doorOpenProgress = 0.0f; // Osiguraj da ne ode ispod 0.0
        }

        doorModel = glm::translate(doorModel, glm::vec3(doorXOffset - doorWidth / 2.0f, doorYBase, h));

        float rotationAngle = glm::mix(0.0f, 90.0f, doorOpenProgress); // Rotacija od 0 do -90 stepeni (otvara se ulevo)
        doorModel = glm::rotate(doorModel, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
     
        doorModel = glm::translate(doorModel, glm::vec3(-(doorXOffset - doorWidth / 2.0f), -doorYBase, -h));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(doorModel));
        glDrawArrays(GL_TRIANGLES, numGalleryVertices, numDoorVertices); // Crtaj vrata

        // --- Crtanje SPOLJAŠNJE RAVNI ---
    // Resetuj model matricu na jediniènu jer je doorModel transformisala matricu za vrata
        model = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glDrawArrays(GL_TRIANGLES, numGalleryVertices + numDoorVertices + numPictureVertices, numOutsidePlaneVertices); // Crtaj ravan

        glDrawArrays(GL_TRIANGLES, numGalleryVertices + numDoorVertices + numPictureVertices + numOutsidePlaneVertices, numGalleryExteriorVertices);



        glBindVertexArray(0); // Odveži VAO

        glUseProgram(textureShader); // Use your 2D texture shader

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, checkerTexture);
        glUniform1i(glGetUniformLocation(textureShader, "uTex"), 0);


        glBindVertexArray(textureVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        auto fpsEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = fpsEndTime - fpsStartTime;

        double sleepTime = FRAME_TIME - elapsed.count();
        if (sleepTime > 0) {
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ POSPREMANJE +++++++++++++++++++++++++++++++++++++++++++++++++


    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(galleryShader);


    glDeleteBuffers(1, &textureVBO);
    glDeleteVertexArrays(1, &textureVAO);
    glDeleteProgram(textureShader);



    glfwTerminate();
    return 0;
}

unsigned int compileShader(GLenum type, const char* source)
{
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
     std::string temp = ss.str();
     const char* sourceCode = temp.c_str();

    int shader = glCreateShader(type);
    
    int success;
    char infoLog[512];
    glShaderSource(shader, 1, &sourceCode, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    unsigned int program;
    unsigned int vertexShader;
    unsigned int fragmentShader;

    program = glCreateProgram();

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    glValidateProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}


// Funkcija za ažuriranje vektora cameraFront na osnovu yaw i pitch uglova
void updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

static unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 2: InternalFormat = GL_RG; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}