#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <playerInput.h>
#include <iostream>
#include <modelloader.h>
#include <physicsHandler.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

const char *vertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
uniform mat4 MVP;
void main() {
    gl_Position = MVP * vec4(aPos, 1.0f);
    TexCoord = aTexCoord;
}
)";

const char *fragmentShaderSource = R"(
#version 460 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D texture0;
uniform bool hasTexture;
uniform vec4 baseColor;
void main() {
    if (hasTexture)
        FragColor = texture(texture0, TexCoord);
    else
        FragColor = baseColor; 

}
)";

float* cubevertex(float x, float y, float z)
{
    float h = 0.5f;

    float* vertices = new float[]
    {
        // Bottom face (z-)
        x - h, y - h, z - h,  // 0
        x + h, y - h, z - h,  // 1
        x + h, y + h, z - h,  // 2
        x - h, y + h, z - h,  // 3

        // Top face (z+)
        x - h, y - h, z + h,  // 4
        x + h, y - h, z + h,  // 5
        x + h, y + h, z + h,  // 6
        x - h, y + h, z + h   // 7
    };

    return vertices;
}



int main() {


glfwInit();
GLFWwindow* window = glfwCreateWindow(800, 600, "Krunker Test", nullptr, nullptr);
if (!window) {
    std::cerr << "Failed to create GLFW window!" << std::endl;
    glfwTerminate();
    return -1;
}
glfwMakeContextCurrent(window);
glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
if (glewInit() != GLEW_OK) {
std::cerr << "Failed to initialize GLEW!" << std::endl;
    glfwTerminate();
    return -1;
}
glEnable(GL_DEPTH_TEST);

PlayerInput input(window);


unsigned int vertexshader,fragmentshader,shaderprogram;
vertexshader = glCreateShader(GL_VERTEX_SHADER);
fragmentshader = glCreateShader(GL_FRAGMENT_SHADER);

glShaderSource(vertexshader,1,&vertexShaderSource,NULL);
glCompileShader(vertexshader);
glShaderSource(fragmentshader,1,&fragmentShaderSource,NULL);
glCompileShader(fragmentshader);

shaderprogram = glCreateProgram();
glAttachShader(shaderprogram,vertexshader);
glAttachShader(shaderprogram,fragmentshader);
glLinkProgram(shaderprogram);

glDeleteShader(vertexshader);
glDeleteShader(fragmentshader);


// after shader setup:
ModelLoader map;
if (!map.load("../assets/map3.glb")) {
    std::cerr << "Failed to load map!" << std::endl;
    return -1;
}

std::cout << "Textures found: " << map.glTextures.size() << std::endl;
std::cout << "Meshes found: " << map.meshes.size() << std::endl;


PhysicsHandler physics;
physics.buildWorld(map.meshes);



glUseProgram(shaderprogram);
unsigned int mvploc = glGetUniformLocation(shaderprogram,"MVP");
unsigned int colorLoc = glGetUniformLocation(shaderprogram, "baseColor");



float lastframe = 0.0f;

while (!glfwWindowShouldClose(window))
{
    float currentime = glfwGetTime();
    float deltatime = currentime - lastframe;
    lastframe = currentime;
    input.update(deltatime);

    MoveResult newpos = physics.resolveMovement(input.getPosition(),input.getVelocity());
    input.updatepos(newpos.position);
    input.updatevel(newpos.velocity);
    input.updateground(newpos.onGround); 

    glm::mat4 vp = input.getMVPMatrix(); // your camera VP

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderprogram);
    
    map.draw(shaderprogram, mvploc, vp,colorLoc);
    glfwSwapBuffers(window);
    glfwPollEvents();
}


glfwTerminate();
return 0;
}
