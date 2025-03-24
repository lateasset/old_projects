#ifndef SHADER_H
#define SHADER_H

#include "glad/glad.h"
#include <opencv2/core.hpp>
//#include <QMatrix4x4>
//#include <QMatrix3x3>
//#include <QVector3D>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream gShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try
        {
	    std::string shaderFolder = "src/";
            // open files
            vShaderFile.open(shaderFolder + vertexPath);
            fShaderFile.open(shaderFolder + fragmentPath);
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
            // if geometry shader path is present, also load a geometry shader
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
	std::cout << "SHADER::FILE_SUCCESFULLY_READ" << std::endl;
        const char* vShaderCode = vertexCode.c_str();
        const char * fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
std::cout << "VERTEX SHADER = " << vertex << std::endl;
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
std::cout << "FRAGMENT SHADER = " << fragment << std::endl;
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // if geometry shader is given, compile geometry shader
        unsigned int geometry;
        // shader Program
        ID = glCreateProgram();
std::cout << "SHADER PROGRAM = " << ID << std::endl;
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
	std::cout << "End shader" << std::endl;
    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use()
    {
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string &name, const cv::Vec2f &value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string &name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    // ------------------------------------------------------------------------
        void setVec2(const std::string &name, const glm::vec2 &value) const
        {
            glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
        }
    // ------------------------------------------------------------------------
    void setVec3(const std::string &name, const cv::Vec3f &value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string &name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    // ------------------------------------------------------------------------
        void setVec3(const std::string &name, const glm::vec3 &value) const
        {
            glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
        }
    // ------------------------------------------------------------------------
    void setVec4(const std::string &name, const cv::Vec4f &value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string &name, float x, float y, float z, float w)
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }
    
    // ------------------------------------------------------------------------
    void setVec4(const std::string &name, const glm::vec4 &value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string &name, const cv::Matx22f &mat) const
    {
    //glm::mat2 m(mat(0, 0), mat(0, 1), mat(1, 0), mat(1, 1));
    glm::mat2 m(mat(0, 0), mat(1, 0), mat(0, 1), mat(1, 1));
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &m[0][0]);
        //m.data()
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string &name, const cv::Matx33f &mat) const
    {
    //glm::mat3 m(mat(0, 0), mat(0, 1), mat(0, 2), mat(1, 0), mat(1, 1), mat(1,2), mat(2, 0), mat(2, 1), mat(2,2));
    glm::mat3 m(mat(0, 0), mat(1, 0), mat(2, 0), mat(0, 1), mat(1, 1), mat(2,1), mat(0, 2), mat(1, 2), mat(2,2));
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &m[0][0]);
        //m.data()
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string &name, const cv::Matx44f &mat) const
    {
     //glm::mat4 m(mat(0, 0), mat(0, 1), mat(0,2), mat(0, 3), mat(1, 0), mat(1, 1), mat(1, 2), mat(1, 3), mat(2, 0), mat(2, 1), mat(2,2), mat(2, 3), mat(3, 0), mat(3, 1), mat(3,2), mat(3, 3));
     glm::mat4 m(mat(0, 0), mat(1, 0), mat(2,0), mat(3, 0), mat(0, 1), mat(1, 1), mat(2, 1), mat(3, 1), mat(0, 2), mat(1, 2), mat(2,2), mat(3, 2), mat(0, 3), mat(1, 3), mat(2,3), mat(3, 3));
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &m[0][0]);
        //m.data()
    }
    // ------------------------------------------------------------------------
        void setMat2(const std::string &name, const glm::mat2 &mat) const
        {
            glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
        // ------------------------------------------------------------------------
        void setMat3(const std::string &name, const glm::mat3 &mat) const
        {
            glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
        // ------------------------------------------------------------------------
        void setMat4(const std::string &name, const glm::mat4 &mat) const
        {
            glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if(type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if(!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if(!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};
#endif
