//
// Created by Haotian on 2017/8/23.
//

#ifndef INC_3DRECONSTRUCTION_SHADERPROGRAM_H
#define INC_3DRECONSTRUCTION_SHADERPROGRAM_H

#include "Shader.h"

class ShaderProgram {
public:

    ShaderProgram();

    virtual ~ShaderProgram();

    //Adds a shader to the program. aShader must have been compiled
    //successfully. If aGiveOwnership is set to true, pointer aShader
    //becomes owned by the ShaderProgram object and will be deleted
    //along with the ShaderProgram. Ownership isn't assumed by the
    //Shader program if AddShader returns false, even if aGiveOwnership
    //is set to true.
    //
    //Returns true if aShader was successfully compiled and added to
    //the program, false if aShader wasn't added to the program
    //(happens if aShader wasn't successfully compiled.)
    bool AddShader(Shader *aShader, bool aGiveOwnership = true);

    //Returns the shader or type aType. If no shader of
    //this type has been added a NULL reference is returned.
    Shader &GetShader(Shader::ShaderType aType);

    //Activates/Deactivate the shader program for rendering.
    void Activate() const;

    void Deactivate() const;

    //Links the shader, returns true if the linking process ended
    //successfully, false otherwise.
    bool Link();

    //Returns true/false whether the program was successfully linked.
    //If a new shader was added after a successful link and the
    //subsequent attempted linking failed, the method returns false.
    //If a new shader was added after a successful link and no
    //subsequent linking were attempted, the method returns true.
    bool IsLinked() const;

    //Returns the linking log
    std::string Log() const;

    //OpenGL's internal id corresponding to the program.
    unsigned int ProgramId() const { return mProgramId; }

protected:

    void CleanVertexShader();

    void CleanFragmentShader();

    Shader *mVertexShader;
    Shader *mFragmentShader;

    unsigned int mProgramId;

    bool mVertexOwnership;
    bool mFragmentOwnership;
};


#endif //INC_3DRECONSTRUCTION_SHADERPROGRAM_H
