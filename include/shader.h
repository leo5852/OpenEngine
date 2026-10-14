#ifndef SHADER_HPP
#define SHADER_HPP

#include <GL/glew.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <glm/glm.hpp>

class Shader 
{
public:
    GLuint programID;
    Shader(){}
    Shader(const char* vertexPath, const char* fragmentPath){this->setup(vertexPath, fragmentPath);}
    
    // "setup() 전에" 호출해야 셰이더 소스의 #version 다음 줄에 #define으로 끼워 넣음.
    // C++ 상수 하나를 GLSL과 공유하고 싶을 때 사용(ex. MAX_LIGHTS)
    void addToDefine(const char* name, int value);

    void setup(const char* vertexPath, const char* fragmentPath);

    // Use the linked program
    void use(){glUseProgram(programID);}
    
    void setUniform(const char *name, float x, float y, float z);
    void setUniform(const char *name, const glm::vec2 &v);
    void setUniform(const char *name, const glm::vec3 &v);
    void setUniform(const char *name, const glm::vec4 &v);
    void setUniform(const char *name, const glm::mat4 &m);
    void setUniform(const char *name, const glm::mat3 &m);
    void setUniform(const char *name, float val);
    void setUniform(const char *name, int val);
    void setUniform(const char *name, bool val);
    void setUniform(const char *name, GLuint val);

private:
    std::string defines; // addDefine()으로 쌓아둔 #define 줄들
    std::map<std::string, int> uniformLocations;

    inline GLint getUniformLocation(const char *name) 
    {
	auto pos = uniformLocations.find(name);
	if (pos == uniformLocations.end()) {
		GLint loc = glGetUniformLocation(programID, name);
        if (loc == -1)
            std::cout << "WARNING: couldn't find uniform \"" << name << "\" . Name typo OR not used in shader.\n";
		uniformLocations[name] = loc;
		return loc;
    }
	return pos->second;
    }
};

#endif
