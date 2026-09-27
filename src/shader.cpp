#include "shader.h"

void Shader::setup(const char* vertexPath, const char* fragmentPath){
        // 1. Create shader object
        GLuint vShaderID = glCreateShader(GL_VERTEX_SHADER);
        GLuint fShaderID = glCreateShader(GL_FRAGMENT_SHADER);
        // 2. read shader source codes from file
        std::string vShaderCode;
        std::ifstream vStream(vertexPath);
        if(vStream.is_open()){
            std::stringstream sstr;
            sstr << vStream.rdbuf();
            vShaderCode = sstr.str();
            vStream.close();
        }
        else{
            std::cout << "ERROR: Can't open file name \"" << vertexPath << "\"";
            return;
        }
        char const* vShaderCodePtr = vShaderCode.c_str();
        //cout << "-- vertex shader code --\n" << vShaderCode << endl;

        std::string fShaderCode;
        std::ifstream fStream(fragmentPath);
        if(fStream.is_open()){
            std::stringstream sstr;
            sstr << fStream.rdbuf();
            fShaderCode = sstr.str();
            fStream.close();
        }
        else{
            std::cout << "ERROR: Can't open file name \"" << fragmentPath << "\"";
            return;
        }
        char const* fShaderCodePtr = fShaderCode.c_str();
        //cout << "-- fragment shader code --\n" << fShaderCode << endl;

        // 3. vertex shader compile & error check
        GLint compileResult = GL_FALSE;
        int infoLogLength = 0;

        printf("\nCompiling Vertex Shader from \"%s\"\n", vertexPath);
        glShaderSource(vShaderID, 1, &vShaderCodePtr, NULL);
        glCompileShader(vShaderID);
        glGetShaderiv(vShaderID, GL_COMPILE_STATUS, &compileResult);
        if(compileResult == GL_FALSE){
            glGetShaderiv(vShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
            std::vector<char> vShaderErrorMsg(infoLogLength+1);
            glGetShaderInfoLog(vShaderID, infoLogLength, NULL, &vShaderErrorMsg[0]);
            std::cout << "Shader Compile Error:\n" << &vShaderErrorMsg[0] << std::endl;
        }
        else
            printf("Compile success\n");

        // 4. fragment shader compile & error check
        compileResult = GL_FALSE;
        infoLogLength = 0;

        printf("\nCompiling Fragment Shader from \"%s\"\n", fragmentPath);
        glShaderSource(fShaderID, 1, &fShaderCodePtr, NULL);
        glCompileShader(fShaderID);
        glGetShaderiv(fShaderID, GL_COMPILE_STATUS, &compileResult);
        if(compileResult == GL_FALSE){
            glGetShaderiv(fShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
            std::vector<char> fShaderErrorMsg(infoLogLength+1);
            glGetShaderInfoLog(fShaderID, infoLogLength, NULL, &fShaderErrorMsg[0]);
            std::cout << "Shader Compile Error:\n" << &fShaderErrorMsg[0] << std::endl;
        }
        else
            printf("Compile success\n");

        // 5. Link & use program
        programID = glCreateProgram();
        glAttachShader(programID, vShaderID);
        glAttachShader(programID, fShaderID);
        glLinkProgram(programID);

        glGetProgramiv(programID, GL_LINK_STATUS, &compileResult);
        if(compileResult == GL_FALSE){
            glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
            if (infoLogLength > 0){
                std::vector<char> ProgramErrorMessage(infoLogLength+1);
                glGetProgramInfoLog(programID, infoLogLength, NULL, &ProgramErrorMessage[0]);
                std::cout << "\nLink Error: " << &ProgramErrorMessage[0] << std::endl;
            }
        }

        glDeleteShader(vShaderID);
        glDeleteShader(fShaderID);
}

void Shader::setUniform(const char *name, float x, float y, float z) {
    GLint loc = getUniformLocation(name);
    glUniform3f(loc, x, y, z);
}

void Shader::setUniform(const char *name, const glm::vec3 &v) {
    this->setUniform(name, v.x, v.y, v.z);
}

void Shader::setUniform(const char *name, const glm::vec4 &v) {
    GLint loc = getUniformLocation(name);
    glUniform4f(loc, v.x, v.y, v.z, v.w);
}

void Shader::setUniform(const char *name, const glm::vec2 &v) {
    GLint loc = getUniformLocation(name);
    glUniform2f(loc, v.x, v.y);
}

void Shader::setUniform(const char *name, const glm::mat4 &m) {
    GLint loc = getUniformLocation(name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, &m[0][0]);
}

void Shader::setUniform(const char *name, const glm::mat3 &m) {
    GLint loc = getUniformLocation(name);
    glUniformMatrix3fv(loc, 1, GL_FALSE, &m[0][0]);
}

void Shader::setUniform(const char *name, float val) {
    GLint loc = getUniformLocation(name);
    glUniform1f(loc, val);
}

void Shader::setUniform(const char *name, int val) {
    GLint loc = getUniformLocation(name);
    glUniform1i(loc, val);
}

void Shader::setUniform(const char *name, GLuint val) {
    GLint loc = getUniformLocation(name);
    glUniform1ui(loc, val);
}

void Shader::setUniform(const char *name, bool val) {
    int loc = getUniformLocation(name);
    glUniform1i(loc, val);
}
    