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
public:
	unsigned int ID;
	ComputeShader(){}
	// constructor generates the shader on the fly
	// ------------------------------------------------------------------------
	ComputeShader(const char* computePath)
	{
		// 1. retrieve the vertex/fragment source code from filePath
		std::string computeCode;
		std::ifstream cShaderFile;
		// ensure ifstream objects can throw exceptions:
		cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			cShaderFile.open(computePath);
			std::stringstream cShaderStream;
			// read file's buffer contents into streams
			cShaderStream << cShaderFile.rdbuf();
			// close file handlers
			cShaderFile.close();
			// convert stream into string
			computeCode = cShaderStream.str();
		}
		catch (std::ifstream::failure& e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
		}
		const char* cShaderCode = computeCode.c_str();
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