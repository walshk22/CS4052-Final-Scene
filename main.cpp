//
//  main.cpp
//  Final Project
//
//  Created by Kaitlyn Walsh on 10/30/18.
//  Copyright © 2018 Kaitlyn Walsh. All rights reserved.
//

#include <iostream>

#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>


#include <string>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

//Assimp Includes
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "maths_funcs.h"

//FOR TEXT
#include <GLUT/glut.h>


//SOUND
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>


//Texture
#include "SOIL2/SOIL2.h"
#include "stb_image.h"


//FOR HEIGHT MAP
//#include "imageloader.h"
//#include "vec3f.h"

//SNOW
//#include "Particle.h"

using namespace std;
GLFWwindow *window;

//CAMERA VARIABLES
vec3 cameraPos = vec3(0.0f,0.0f,10.0f);
vec3 cameraTarget = vec3(0.0f,0.0f,0.0f);
vec3 cameraDirection = normalise(vec3(cameraPos-cameraTarget));
vec3 cameraFront = vec3(0.0f,0.0f,-1.0f);
vec3 cameraUp = vec3(0.0f,1.0f,0.0f);
vec3 cameraRight = normalise(cross(cameraUp, cameraDirection));


//MOUSE VARIABLES
GLfloat rotate_angle = -45.0;
float x, y, z;
mat4 snowman;
mat4 trans;


int matrix_location, view_mat_location, proj_mat_location, matrix_location2;
GLuint VAO;
GLuint VAO2;
unsigned int texture1, texture2;

//BACKGROUND
GLuint BG_texture;
GLuint VBO_BG, VAO_BG, EBO_BG;


//MUSIC
Mix_Music *xmasMusic = NULL;


//ADD KEYBOARD TO CONTROL CAMERA VIEW
void keyPress(GLFWwindow *window, int key, int scancode, int action, int mods);


//ADD MOUSE CLICK
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);


#define MESH_NAME "lowtree.dae"
#define MESH_NAME2 "snwmnnn.dae"

#pragma region SimpleTypes
typedef struct
{
    size_t mPointCount = 0;
    std::vector<vec3> mVertices;
    std::vector<vec3> mNormals;
    std::vector<vec2> mTextureCoords;
} ModelData;

ModelData snowMan;
ModelData tree;
int width = 800;
int height = 600;
GLuint loc1, loc2, loc3;
mat4 view;
mat4 persp_proj;

#pragma endregion SimpleTypes

GLint shaderProgramID;
GLint shaderProgram2;
GLint backgroundShader;


#pragma region MESH LOADING
/*----------------------------------------------------------------------------
 MESH LOADING FUNCTION
 ----------------------------------------------------------------------------*/

ModelData load_mesh(const char* file_name) {
    ModelData modelData;
    
    /* Use assimp to read the model file, forcing it to be read as    */
    /* triangles. The second flag (aiProcess_PreTransformVertices) is */
    /* relevant if there are multiple meshes in the model file that   */
    /* are offset from the origin. This is pre-transform them so      */
    /* they're in the right position.                                 */
    const aiScene* scene = aiImportFile(
                                        file_name,
                                        aiProcess_Triangulate | aiProcess_PreTransformVertices
                                        );
    
    if (!scene) {
        fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
        return modelData;
    }
    
    printf("  %i materials\n", scene->mNumMaterials);
    printf("  %i meshes\n", scene->mNumMeshes);
    printf("  %i textures\n", scene->mNumTextures);
    
    for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
        const aiMesh* mesh = scene->mMeshes[m_i];
        printf("    %i vertices in mesh\n", mesh->mNumVertices);
        modelData.mPointCount += mesh->mNumVertices;
        for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
            if (mesh->HasPositions()) {
                const aiVector3D* vp = &(mesh->mVertices[v_i]);
                modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
            }
            if (mesh->HasNormals()) {
                const aiVector3D* vn = &(mesh->mNormals[v_i]);
                modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
            }
            if (mesh->HasTextureCoords(0)) {
                const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
                modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
            }
            if (mesh->HasTangentsAndBitangents()) {
                /* You can extract tangents and bitangents here              */
                /* Note that you might need to make Assimp generate this     */
                /* data for you. Take a look at the flags that aiImportFile  */
                /* can take.                                                 */
            }
        }
    }
    
    aiReleaseImport(scene);
    return modelData;
}

#pragma endregion MESH LOADING

#pragma region SHADER_FUNCTIONS
std::string readShaderSource(const std::string& fileName)
{
    std::ifstream file(fileName.c_str());
    if(file.fail()) {
        cout << "error loading shader called " << fileName;
        exit (1);
    }
    
    std::stringstream stream;
    stream << file.rdbuf();
    file.close();
    
    return stream.str();
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
    // create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);
    
    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
    std::string outShader = readShaderSource(pShaderText);
    const char* pShaderSource = outShader.c_str();
    
    // Bind the source code to the shader, this happens before compilation
    glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
    // compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
    // check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
    // Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders()
{
    //Start the process of setting up our shaders by creating a program ID
    //Note: we will link all the shaders together into this ID
    shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        std::cerr << "Error creating shader program..." << std::endl;
        //std::cerr << "Press enter/return to exit..." << std::endl;
        //std::cin.get();
        exit(1);
    }
    
    // Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, "Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(shaderProgramID, "Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);
    
    GLint Success = 0;
    GLchar ErrorLog[1024] = { '0' };
    // After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
        //std::cerr << "Press enter/return to exit..." << std::endl;
        //std::cin.get();
        exit(1);
    }
    
    // program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
        //std::cerr << "Press enter/return to exit..." << std::endl;
        //std::cin.get();
        exit(1);
    }
    return shaderProgramID;
}

GLuint CompileShaders2()
{
    //Start the process of setting up our shaders by creating a program ID
    //Note: we will link all the shaders together into this ID
    shaderProgram2 = glCreateProgram();
    if (shaderProgram2 == 0) {
        std::cerr << "Error creating shader program..." << std::endl;
        //std::cerr << "Press enter/return to exit..." << std::endl;
        //std::cin.get();
        exit(1);
    }
    
    // Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgram2, "Shaders2/simpleVertexShader2.txt", GL_VERTEX_SHADER);
    AddShader(shaderProgram2, "Shaders2/simpleFragmentShader2.txt", GL_FRAGMENT_SHADER);
    
    GLint Success = 0;
    GLchar ErrorLog[1024] = { '0' };
    // After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgram2);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgram2, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(shaderProgram2, sizeof(ErrorLog), NULL, ErrorLog);
        std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
        //std::cerr << "Press enter/return to exit..." << std::endl;
        //std::cin.get();
        exit(1);
    }
    
    // program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgram2);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgram2, GL_LINK_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgram2, sizeof(ErrorLog), NULL, ErrorLog);
        std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
        //std::cerr << "Press enter/return to exit..." << std::endl;
        //std::cin.get();
        exit(1);
    }
    
    
    return shaderProgram2;
}

GLuint CompileShaders3()
{
    //Start the process of setting up our shaders by creating a program ID
    //Note: we will link all the shaders together into this ID
    backgroundShader = glCreateProgram();
    if (backgroundShader == 0) {
        std::cerr << "Error creating shader program..." << std::endl;
        //std::cerr << "Press enter/return to exit..." << std::endl;
        //std::cin.get();
        exit(1);
    }
    
    // Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(backgroundShader, "BGShaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
    AddShader(backgroundShader, "BGShaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);
    
    GLint Success = 0;
    GLchar ErrorLog[1024] = { '0' };
    // After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(backgroundShader);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(backgroundShader, GL_LINK_STATUS, &Success);
    if (Success == 0) {
        glGetProgramInfoLog(backgroundShader, sizeof(ErrorLog), NULL, ErrorLog);
        std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
        //std::cerr << "Press enter/return to exit..." << std::endl;
        //std::cin.get();
        exit(1);
    }
    
    // program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(backgroundShader);
    // check for program related errors using glGetProgramiv
    glGetProgramiv(backgroundShader, GL_LINK_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(backgroundShader, sizeof(ErrorLog), NULL, ErrorLog);
        std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
        //std::cerr << "Press enter/return to exit..." << std::endl;
        //std::cin.get();
        exit(1);
    }
    
    
    return backgroundShader;
}

#pragma endregion SHADER_FUNCTIONS

#pragma region VBO_FUNCTIONS
void generateObjectBufferMesh() {
    /*----------------------------------------------------------------------------
     LOAD MESH HERE AND COPY INTO BUFFERS
     ----------------------------------------------------------------------------*/
    
    //Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
    //Might be an idea to do a check for that before generating and binding the buffer.
    
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, texture1);
    //glUniform1f(glGetUniformLocation(shaderProgramID, "TreeText.jpg"));
    
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    
    tree = load_mesh(MESH_NAME);
    
    unsigned int vp_vbo1 = 0;
    loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
    loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
    loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");
    
    glGenBuffers(1, &vp_vbo1);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo1);
    glBufferData(GL_ARRAY_BUFFER, tree.mPointCount * sizeof(vec3), &tree.mVertices[0], GL_STATIC_DRAW);
    
    
    unsigned int vn_vbo1 = 0;
    glGenBuffers(1, &vn_vbo1);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo1);
    glBufferData(GL_ARRAY_BUFFER, tree.mPointCount * sizeof(vec3), &tree.mNormals[0], GL_STATIC_DRAW);
    
    // This is for texture coordinates which you don't currently need, so I have commented it out
    GLuint vt_vbo = 0;
    glGenBuffers (1, &vt_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
    glBufferData (GL_ARRAY_BUFFER,  sizeof(vec2), &tree.mTextureCoords[0], GL_STATIC_DRAW);
    
    
    
    glEnableVertexAttribArray(loc1);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo1);
    glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    
    glEnableVertexAttribArray(loc2);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo1);
    glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    
    // This is for texture coordinates which you don't currently need, so I have commented it out
    glEnableVertexAttribArray (loc3);
    glBindBuffer (GL_ARRAY_BUFFER, vt_vbo);
    glVertexAttribPointer (loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    
    //MESH 2 DATA---- SNOWMAN
    glBindTexture(GL_TEXTURE_2D, texture2);
    glGenVertexArrays(1, &VAO2);
    glBindVertexArray(VAO2);
    snowMan = load_mesh(MESH_NAME2);
    
    unsigned int vp_vbo2 = 0;
    int location1 = glGetAttribLocation(shaderProgram2, "vertex_position");
    int location2 = glGetAttribLocation(shaderProgram2, "vertex_normal");
    int location3 = glGetAttribLocation(shaderProgram2, "vertex_texture");
    
    glGenBuffers(1, &vp_vbo2);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo2);
    glBufferData(GL_ARRAY_BUFFER, snowMan.mPointCount * sizeof(vec3), &snowMan.mVertices[0], GL_STATIC_DRAW);
    
    
    unsigned int vn_vbo2 = 0;
    glGenBuffers(1, &vn_vbo2);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo2);
    glBufferData(GL_ARRAY_BUFFER, snowMan.mPointCount * sizeof(vec3), &snowMan.mNormals[0], GL_STATIC_DRAW);
    
    //    This is for texture coordinates which you don't currently need, so I have commented it out
    unsigned int vt_vbo2 = 0;
    glGenBuffers (1, &vt_vbo2);
    glBindBuffer (GL_ARRAY_BUFFER, vt_vbo2);
    glBufferData (GL_ARRAY_BUFFER, sizeof(vec2), &snowMan.mTextureCoords[0], GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(location1);
    glBindBuffer(GL_ARRAY_BUFFER, vp_vbo2);
    glVertexAttribPointer(location1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    
    glEnableVertexAttribArray(location2);
    glBindBuffer(GL_ARRAY_BUFFER, vn_vbo2);
    glVertexAttribPointer(location2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    
    
    // This is for texture coordinates which you don't currently need, so I have commented it out
    glEnableVertexAttribArray (location3);
    glBindBuffer (GL_ARRAY_BUFFER, vt_vbo2);
    glVertexAttribPointer (location3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    
    
}

#pragma endregion VBO_FUNCTIONS


void updateScene() {
    
    static float last_time = 0;
    float curr_time = glfwGetTime();
    if (last_time == 0)
        last_time = curr_time;
    float delta = (curr_time - last_time) * 0.1f;
    last_time = curr_time;
    
    rotate_angle += 20.0f * 45.0;
    rotate_angle = fmodf(rotate_angle, 360.0f);
}

void init()
{
    //CREATING SHADER 1
    shaderProgramID = CompileShaders();
    
    //CREATING SHADER 2
    shaderProgram2 = CompileShaders2();
    
    backgroundShader = CompileShaders3();
    
    //GENERATING BACKGROUND
    float vertices[] = {
        0.5f, 0.5f, 0.75f,      1.0f, 1.0f, 1.0f,           1.0f, 1.0f,     //Top Right
        0.5f, -0.5f, 0.75f,     1.0f, 1.0f, 1.0f,           1.0f, 0.0f,     //Bottom Right
        -0.5f, -0.5f, 0.75f,    1.0f, 1.0f, 1.0f,           0.0f, 0.0f,     //Bottom Left
        -0.5f, 0.5f, 0.75f,     1.0f, 1.0f, 1.0f,           0.0f, 1.0f
    };
    
    int indices[] = {
        0, 1, 3,            //Triangle 1
        1, 2, 3             //Triangle 2
    };
    
    glGenVertexArrays(1, &VAO_BG);
    glGenBuffers(1, &VBO_BG);
    glGenBuffers(1, &EBO_BG);
    
    glBindVertexArray(VAO_BG);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO_BG);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_BG);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    //Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 *sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);
    //Color
    glVertexAttribPointer(1,3,GL_FLOAT, GL_FALSE, 8 *sizeof(float), (void *)(3 *sizeof(float)));
    glEnableVertexAttribArray(1);
    //Texture Coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 *sizeof(float), (void *) (6 *sizeof(float) ));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    //Load Texture
    int BG_width, BG_height, nrChannels;
    glGenTextures(1, &BG_texture);
    glBindTexture(GL_TEXTURE_2D, BG_texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_set_flip_vertically_on_load(true);
    
    unsigned char *image_BG = stbi_load("background.jpg", &BG_width, &BG_height, &nrChannels, 0);
    if(image_BG)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BG_width, BG_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_BG);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else{
        cout << "ERROR:: FAILED TO LOAD TEXTURE" << endl;
    }
    stbi_image_free(image_BG);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    
    //GENERATING OBJECTS
    generateObjectBufferMesh();
}



//Loading Music using SDL_MIXER
bool loadMusic()
{
    bool success = true;
    
    xmasMusic = Mix_LoadMUS("Winter-Wonderland.wav");
    
    if(xmasMusic == NULL)
    {
        cout << "FAILED TO LOAD MUSIC :: SDL_MIXER ERROR: %s\n" << Mix_GetError() << endl;
        success = false;
    }
    
    return success;
}

void createBackground()
{
    
    glUseProgram(backgroundShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, BG_texture);
    glUniform1i(glGetUniformLocation(backgroundShader, "BG_Texture"), 0);
    
    glBindVertexArray(VAO_BG);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
}


void display()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.0f,0.0f,0.4f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    
    //KEYBOARD FOR CAMERA POSITION
    glfwSetKeyCallback(window, keyPress);
    //MOUSE
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    
    //SCENE
    createBackground();
    
    glUseProgram(shaderProgramID);
    matrix_location = glGetUniformLocation(shaderProgramID, "model");
    view_mat_location = glGetUniformLocation(shaderProgramID, "view");
    proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");
    
    
    view = identity_mat4();
    persp_proj = identity_mat4();
    mat4 tree1 = identity_mat4();
    tree1 = translate(tree1, vec3(0.0f,0.0f,0.0f));
    tree1 = scale(tree1, vec3(1.5f,1.5f,1.5f));
    tree1 = translate(tree1, vec3(3.5, -3.5, 0.0f));
    
    view = look_at(cameraPos, cameraPos + cameraFront, cameraUp);
    persp_proj = perspective(45.0, (float)width/ (float)height, 0.1f, 1000.0f);
    
    glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
    glUniformMatrix4fv(view_mat_location,1, GL_FALSE, view.m);
    glUniformMatrix4fv(matrix_location,1, GL_FALSE, tree1.m);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, tree.mPointCount);
    
    
    mat4 view2 = identity_mat4();
    mat4 persp_proj2 = identity_mat4();
    mat4 tree2 = identity_mat4();
    tree2 = translate(tree2, vec3(0.0f, 0.0f, 0.0f));
    tree2 = scale(tree2, vec3(1.5f,1.5f,1.5f));
    tree2 = translate(tree2, vec3(-3.5f, -3.5f, 0.0f));
    
    glUniformMatrix4fv(matrix_location,1, GL_FALSE, tree2.m);
    glDrawArrays(GL_TRIANGLES, 0, tree.mPointCount);
    
    
    glUseProgram(shaderProgram2);
    matrix_location2 = glGetUniformLocation(shaderProgram2, "model");
    int view_mat_location2 = glGetUniformLocation(shaderProgram2, "view");
    int proj_mat_location2 = glGetUniformLocation(shaderProgram2, "proj");
    
    x = 0;
    y = -0.9;
    z = -0.5;
    
    mat4 snowmanView = identity_mat4();
    mat4 snowmanProj = identity_mat4();
    snowmanView = look_at(cameraPos, cameraPos + cameraFront, cameraUp);
    snowmanProj = perspective(45.0, (float)width/ (float)height, 0.1f, 1000.0f);
    
    snowman = identity_mat4();
    snowman = translate(snowman, vec3(0.0f,0.0f,0.0f));
    snowman = rotate_y_deg(snowman,rotate_angle);
    snowman = scale(snowman, vec3(0.35, 0.35, 0.35));
    snowman = translate(snowman, vec3(0.0f, -0.95f, -0.5f));
    
    glUniformMatrix4fv(matrix_location2, 1, GL_FALSE, snowman.m);
    glUniformMatrix4fv(view_mat_location2, 1, GL_FALSE, snowmanView.m);
    glUniformMatrix4fv(proj_mat_location2, 1, GL_FALSE, snowmanProj.m);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glBindVertexArray(VAO2);
    glDrawArrays(GL_TRIANGLES, 0, snowMan.mPointCount);
    
    
    mat4 smallSnowman1 = identity_mat4();
    smallSnowman1 = translate(smallSnowman1, vec3(0.0f,0.0f,0.0f));
    smallSnowman1 = scale(smallSnowman1, vec3(0.75f, 0.75f, 0.75f));
    smallSnowman1 = translate(smallSnowman1, vec3(-5.0, -1.0f, 0.75f));
    
    smallSnowman1 =  snowman * smallSnowman1;
    
    glUniformMatrix4fv(matrix_location2, 1, GL_FALSE, smallSnowman1.m);
    glDrawArrays(GL_TRIANGLES, 0, snowMan.mPointCount);
    
    mat4 smallSnowman2 = identity_mat4();
    smallSnowman2 = translate(smallSnowman2, vec3(0.0f,0.0f,0.0f));
    smallSnowman2 = scale(smallSnowman2, vec3(0.75f, 0.75f, 0.75f));
    smallSnowman2 = translate(smallSnowman2, vec3(5.0, -1.0f, 0.75f));
    
    smallSnowman2 = snowman * smallSnowman2;
    
    glUniformMatrix4fv(matrix_location2, 1, GL_FALSE, smallSnowman2.m);
    glDrawArrays(GL_TRIANGLES, 0, snowMan.mPointCount);
    
}



void displayText()
{
    glColor3d(1.0, 0.0, 0.1);
    glPushMatrix();
    glLoadIdentity();
    glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, 'H');
    glPopMatrix();
}


int main(int argc, const char * argv[]) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    
    if(!glfwInit())
    {
        return -1;
    }
    
    window = glfwCreateWindow(width, height, "Chirstmas ", NULL, NULL);
    int screenWidth, screenHeight;
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    
    
    if(!window)
    {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    
    if( GLEW_OK != glewInit())
    {
        cout << "FAILED TO INITIALIZE GLEW" << endl;
        return EXIT_FAILURE;
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    
    bool SDLSuccess = true;
    
    //Initializing SDL Audio and IMAGE
    SDL_Init(SDL_INIT_AUDIO);
    
    if(SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        cout << "ERROR FAILED TO INITIALIZE SDL :: ERROR:" << SDL_GetError() << endl;
        SDLSuccess = false;
    }
    
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        cout << "ERROR:: SDL_MIXER NOT INITALIZED ::: SDL ERROR %s \n" << Mix_GetError() << endl;
        SDLSuccess = false;
        
    }
    
    bool musicLoad = loadMusic();
    if(!musicLoad)
    {
        cout << "ERROR LOADING MUSIC" << endl;
    }
    
    init();
    
    while(!glfwWindowShouldClose(window))
    {
        display();
        //updateScene();
        
        
        glDisableClientState(GL_VERTEX_ARRAY);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    
    Mix_FreeMusic(xmasMusic);
    Mix_Quit();
    SDL_Quit();
    
    glfwTerminate();
    return 0;
}



void keyPress(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if(key == GLFW_KEY_UP)
    {
        //camera perspective zoom out
        cameraPos= vec3(0.0f,0.0f,100.0f);
        persp_proj = perspective(30.0, (float)(width/2)/(float)height, 10.0, 1000.0);
        
    }
    
    else if(key == GLFW_KEY_DOWN)
    {
        //camera perspective zoom in
        cameraPos = vec3(0.0f,0.0f,4.0f);
        persp_proj = perspective (15.0, (float)width/(float)height, 100.0,1000.0);
        
    }
    
    else if(key == GLFW_KEY_RIGHT)
    {
        
        //camera perspective goes right
        cameraPos = vec3(0.0f, -2.0f, 20.0f);
        persp_proj = perspective(90.0, (float)width/(float)height, 10.0, 100.0);
    }
    
    else if(key == GLFW_KEY_LEFT)
    {
        //camera perspective goes left
        cameraPos = vec3(-2.0f,0.0f,15.0);
        cameraTarget = vec3(0.5f, 0.5f, 0.5f);
        cameraDirection = vec3(normalise(cameraPos - cameraTarget));
    }
    
    else if(key == GLFW_KEY_SPACE)
    {
        cameraPos = vec3(0.0f,0.0f,10.0f);
    }
    
    else if(key == GLFW_KEY_R)
    {
        
        cameraPos = vec3(-5.0f, -1.0f, 20.0f);
        cameraDirection = vec3(cameraPos - cameraTarget);
        
    }
    
    //MUSIC CONTROLS
    
    else if(key == GLFW_KEY_M)
    {
        Mix_Volume(-1, MIX_MAX_VOLUME/2);
        
        if(Mix_PlayingMusic() == 0)
        {
            Mix_PlayMusic(xmasMusic, -1);
        }
    }
    
    else if (key == GLFW_KEY_N)
    {
        Mix_PauseMusic();
        
    }
    
    else if(key == GLFW_KEY_B)
    {
        Mix_ResumeMusic();
    }
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS)
    {
        
        cout << "MOUSE PRESSED" << endl;
        
        //rotate_angle = rotate_angle +15.0f;
        
        static float last_time = 0;
        float curr_time = glfwGetTime();
        if (last_time == 0)
            last_time = curr_time;
        float delta = (curr_time - last_time) * 1.0f;
        
        rotate_angle = rotate_angle + 15.0 * delta;
        
    }
}
