#ifndef CSSHADER_H
#define CSSHADER_H

// dear imgui setup only
#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


class ComputeShader
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
	void compileAndLinkProgram(const char* cShaderCode){
		// 2. compile shaders
		unsigned int compute;
		// vertex shader
		compute = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(compute, 1, &cShaderCode, NULL);
		glCompileShader(compute);
		checkCompileErrors(compute, "COMPUTE");
		// shader Program
		ID = glCreateProgram();
		glAttachShader(ID, compute);
		glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");
		// delete the shaders as they're linked into our program now and no longer necessary
		glDeleteShader(compute);
	}
public:
	unsigned int ID;
	ComputeShader(){}
	// constructor generates the shader on the fly
	// ------------------------------------------------------------------------
	ComputeShader(const char* computePath)
	{
		// 1. retrieve the vertex/fragment source code from filePath
		std::string computeCode = loadStringFile(computePath);
		const char* cShaderCode = computeCode.c_str();
		compileAndLinkProgram(cShaderCode);
	}
	ComputeShader(const char* computePath, const char* entryPoint){
		std::string entryPointName = entryPoint;
		// pre-pending the new entry point
		std::string shaderCode = loadStringFile(computePath);
		std::string versionDirective = shaderCode.substr(0, 12);
		shaderCode = shaderCode.substr(12, (int)shaderCode.size()-12);
		shaderCode = versionDirective + "\n#define " + entryPointName + " main\n" + shaderCode;
		compileAndLinkProgram(shaderCode.c_str());
	}
	
	// activate the shader
	// ------------------------------------------------------------------------
	void use()
	{
		glUseProgram(ID);
	}
	void runProgram(int x, int y, int z) {
		glUseProgram(ID);
		glDispatchCompute(x, y, z);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
	// utility uniform functions
	// ------------------------------------------------------------------------
	void setTexture(const std::string& name, GLuint& texture) const{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), texture);
	}
	void setBool(const std::string& name, bool value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
	}
	// ------------------------------------------------------------------------
	void setInt(const std::string& name, int value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	//------------------------------------------------------------------------
	void setFloat(const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}
	void setFloat4(const std::string& name, float* value) const
	{
		glUniform4f(glGetUniformLocation(ID, name.c_str()), value[0], value[1], value[2], value[3]);
	}


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