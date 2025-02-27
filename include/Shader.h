#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// essa classe foi inteiramente retirada do site learnopengl.com
// FONTE
// https://learnopengl.com/Getting-started/Shaders

class Shader
{
    inline bool isBlankCharacter(char c) {return c <= 0x20;} 
    std::string loadStringFile(const char* path){
        // 1. retrieve the vertex/fragment source code from filePath
        std::string str;
        std::ifstream f;
        // ensure ifstream objects can throw exceptions:
        f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            f.open(path);
            std::stringstream cShaderStream;
            // read file's buffer contents into streams
            cShaderStream << f.rdbuf();
            // close file handlers
            f.close();
            // convert stream into string
            str = cShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }
        // sanitizing.
        while(isBlankCharacter(str.back())) str.pop_back();
        for(int i = 0; i < str.size(); i++){
            if(!isBlankCharacter(str[i])){
                str = str.substr(i, (int)str.size() - i);
                break;
            }
        }
        return str;
    }
    void compileAndLinkProgram(const char* vShaderCode, const char* fShaderCode){
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
	Shader(){}

    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        
        compileAndLinkProgram(vShaderCode, fShaderCode);
    }
    Shader(const char* shaderPath, const std::string& vertexEntryPoint, const std::string& fragmentEntryPoint){
        std::string shaderCode = loadStringFile(shaderPath);
        // pre-pending the new entry point
        std::string versionDirective = shaderCode.substr(0, 12);
        shaderCode = shaderCode.substr(12, (int)shaderCode.size()-12);
        
        std::string vertexShaderCode = versionDirective + "\n#define " + vertexEntryPoint + " main\n" + shaderCode;
        std::string fragmentShaderCode = versionDirective + "\n#define " + fragmentEntryPoint + " main\n" + shaderCode;
        
        compileAndLinkProgram(vertexShaderCode.c_str(), fragmentShaderCode.c_str());
    }
    Shader(const char* shaderPath){
        std::string shaderCode = loadStringFile(shaderPath);
        // pre-pending the new entry point
        std::string versionDirective = shaderCode.substr(0, 12);
        shaderCode = shaderCode.substr(12, (int)shaderCode.size()-12);
        
        std::string vertexShaderCode = versionDirective + "\n#define " + "H_VERT" + " H_VERT\n" + shaderCode;
        std::string fragmentShaderCode = versionDirective + "\n#define " + "H_FRAG" + " H_FRAG\n" + shaderCode;
        
        compileAndLinkProgram(vertexShaderCode.c_str(), fragmentShaderCode.c_str());
    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use()
    {
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setInt2(const std::string& name, glm::vec2 value) const
    {
        glUniform2i(glGetUniformLocation(ID, name.c_str()), (GLint)value[0], (GLint)value[1]);
    }
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setFloat2(const std::string& name, glm::vec2 value) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), value[0], value[1]);
    }
	void setFloat4(const std::string& name, glm::vec4 value) const
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), value[0], value[1], value[2], value[3]);
    }
    void setUintV(const std::string& name, int count, GLuint* value) const
    {
        glUniform1uiv(glGetUniformLocation(ID, name.c_str()), count, value);
    }
    void setFloatV(const std::string& name, int count, GLfloat* value) const
    {
        glUniform1fv(glGetUniformLocation(ID, name.c_str()), count, value);
    }
    void setUint(const std::string& name, GLuint value) const
    {
        glUniform1ui(glGetUniformLocation(ID, name.c_str()), value);
    }
	void setMatrix4(const std::string& name, glm::mat4 m) {
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(m));
	}
    // ------------------------------------------------------------------------

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};
#endif