#ifndef MYCLPROGRAM_H
#define MYCLPROGRAM_H

#include "include_opencl.h"
#include "myclwrapper.h"

#include <QString>

class MyCLProgram
{
public:
    MyCLProgram();

    /// Creates the program from the given sourceFile. Returns
    /// true on success, false on failure.
    bool create(MyCLWrapper *wrapper, QString sourceFile);
    void destroy();

    cl_program program() const { return mProgram; }

private:
    bool mCreated;
    cl_program mProgram;

    MyCLWrapper *mCLWrapper;
};

#endif // MYCLPROGRAM_H
