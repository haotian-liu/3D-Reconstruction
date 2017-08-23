//
// Created by Haotian on 2017/8/23.
//

#include "ShaderProgram.h"
#include "GLInclude.h"

ShaderProgram::ShaderProgram()
        : mVertexShader(NULL), mFragmentShader(NULL), mVertexOwnership(false), mFragmentOwnership(false),
          mProgramId(0) {}

//////////////////////////////////////////////////////////////////////////

bool ShaderProgram::AddShader(Shader *aShader, bool aGiveOwnership /* = true */) {
    if (aShader->IsCompiled()) {
        if (aShader->Type() == Shader::Shader_Vertex) {
            if (mVertexShader != NULL && mVertexOwnership) {
                CleanVertexShader();
            }

            mVertexShader = aShader;
            mVertexOwnership = aGiveOwnership;
        } else if (aShader->Type() == Shader::Shader_Fragment) {
            if (mFragmentShader != NULL && mFragmentOwnership) {
                CleanFragmentShader();
            }

            mFragmentShader = aShader;
            mFragmentOwnership = aGiveOwnership;
        }

        return true;
    } else {
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////

bool ShaderProgram::Link() {
    if (mVertexShader != NULL && mFragmentShader != NULL) {
        if (mProgramId != 0) {
            glDetachShader(ProgramId(), mVertexShader->ShaderId());
            glDetachShader(ProgramId(), mFragmentShader->ShaderId());

            glDeleteProgram(ProgramId());
        }

        //Create the shader program and attach the two shaders to it.
        mProgramId = glCreateProgram();
        //printf("Program: %d\n", mProgramId);

        glAttachShader(ProgramId(), mVertexShader->ShaderId());
        glAttachShader(ProgramId(), mFragmentShader->ShaderId());

        glBindFragDataLocation(ProgramId(), 0, "FragColor");

        //Link the shader program
        glLinkProgram(ProgramId());

        return IsLinked();
    } else {
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////

bool ShaderProgram::IsLinked() const {
    int CompilationStatus = 0;
    glGetProgramiv(ProgramId(), GL_LINK_STATUS, &CompilationStatus);
    return CompilationStatus == GL_TRUE;
}

//////////////////////////////////////////////////////////////////////////

std::string ShaderProgram::Log() const {
    int LogLength = 0;
    glGetProgramiv(ProgramId(), GL_INFO_LOG_LENGTH, &LogLength);

    char *Log = new char[LogLength];

    int ReturnedLength = 0;
    glGetProgramInfoLog(ProgramId(), LogLength, &ReturnedLength, Log);


    std::string RetStr(Log);
    delete[] Log;

    return RetStr;
}

//////////////////////////////////////////////////////////////////////////

Shader &ShaderProgram::GetShader(Shader::ShaderType aType) {
    switch (aType) {
        case Shader::Shader_Vertex:
            return *mVertexShader;
        case Shader::Shader_Fragment:
            return *mFragmentShader;
        default:
            return *((Shader *) NULL);
    }
}

//////////////////////////////////////////////////////////////////////////

void ShaderProgram::CleanVertexShader() {
    if (mVertexShader != NULL) {
        glDetachShader(mVertexShader->ShaderId(), ProgramId());
        if (mVertexOwnership) {
            delete mVertexShader;
            mVertexShader = NULL;
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void ShaderProgram::CleanFragmentShader() {
    if (mFragmentShader != NULL) {
        glDetachShader(mFragmentShader->ShaderId(), ProgramId());
        if (mFragmentOwnership) {
            delete mFragmentShader;
            mFragmentShader = NULL;
        }
    }
}

void ShaderProgram::Activate() const {
    glUseProgram(ProgramId());
}

//////////////////////////////////////////////////////////////////////////

void ShaderProgram::Deactivate() const {
    glUseProgram(NULL);
}
//////////////////////////////////////////////////////////////////////////

ShaderProgram::~ShaderProgram() {
    CleanVertexShader();
    CleanFragmentShader();

    glDeleteProgram(ProgramId());
}