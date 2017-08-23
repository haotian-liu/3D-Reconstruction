//
// Created by Haotian on 2017/8/23.
//

#include "Shader.h"
#include <fstream>
#include "GLInclude.h"

Shader::Shader(Shader::ShaderType aType)
        : mType(aType), mShaderId(0) {}

static void dumpShaderLog(unsigned int obj) {
    int infologLength = 0;
    int charsWritten = 0;
    char *infoLog;

    glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

    if (infologLength > 0) {
        infoLog = (char *) malloc(infologLength);
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
        printf("%s\n", infoLog);
        free(infoLog);
    }
}

static void dumpProgramLog(unsigned int obj) {
    int infologLength = 0;
    int charsWritten = 0;
    char *infoLog;

    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

    if (infologLength > 0) {
        infoLog = (char *) malloc(infologLength);
        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
        printf("%s\n", infoLog);
        free(infoLog);
    }
}

bool Shader::CompileSourceFile(const std::string &aFileName) {
    //Delete previous shader if the class already contained one.
    if (mShaderId != 0) {
        glDeleteShader(mShaderId);
        mShaderId = 0;
    }

    //Open and read the shader file.
    std::fstream ShaderFile;
    ShaderFile.open(aFileName);

    std::string ShaderCode((std::istreambuf_iterator<char>(ShaderFile)),
                           std::istreambuf_iterator<char>()
    );

    ShaderFile.close();

    //Create the shader object
    if (mType == Shader_Vertex) {
        mShaderId = glCreateShader(GL_VERTEX_SHADER);
    } else if (mType == Shader_Fragment) {
        mShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    }

    //Associate the shader's source code to the shader object.
    const char *ShaderChars = ShaderCode.c_str();
    glShaderSource(mShaderId, 1, &ShaderChars, NULL);

    //Compile the shader
    glCompileShader(mShaderId);

    mShaderId = mShaderId;

    dumpShaderLog(mShaderId);

    return IsCompiled();
}

bool Shader::IsCompiled() const {
    int CompilationStatus = 0;
    glGetShaderiv(mShaderId, GL_COMPILE_STATUS, &CompilationStatus);

    return CompilationStatus == GL_TRUE;
}

std::string Shader::Log() const {
    int LogLength = 0;
    glGetShaderiv(mShaderId, GL_INFO_LOG_LENGTH, &LogLength);

    auto Log = new char[LogLength];

    int ReturnedLength = 0;
    glGetShaderInfoLog(mShaderId, LogLength, &ReturnedLength, Log);

    std::string RetStr(Log);
    delete[] Log;

    return RetStr;
}

Shader::~Shader() {
    if (mShaderId != 0) {
        glDeleteShader(mShaderId);
        mShaderId = 0;
    }
}