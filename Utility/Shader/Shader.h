//
// Created by Haotian on 2017/8/23.
//

#ifndef INC_3DRECONSTRUCTION_SHADER_H
#define INC_3DRECONSTRUCTION_SHADER_H

#include <string>

class Shader {
public:

    enum ShaderType {
        Shader_Vertex,
        Shader_Fragment,
    };

    Shader(ShaderType aType);

    virtual ~Shader();

    //Compiles the source code of the shader located in file aFileName.
    //Returns true if the shader compiled successfully, false otherwise.
    bool CompileSourceFile(const std::string &aFileName);

    //Returns true if the shader has been succcessfully compiled once
    //and can be integrated in a shader program.
    bool IsCompiled() const;

    //Returns the compilation log for this shader.
    std::string Log() const;

    //Returns the OpenGL shader Id for this shader.
    unsigned int ShaderId() const { return mShaderId; }

    //Returns the shader type of this given shader object.
    ShaderType Type() const { return mType; }

protected:

    ShaderType mType;
    unsigned int mShaderId;
};


#endif //INC_3DRECONSTRUCTION_SHADER_H
