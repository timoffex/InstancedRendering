#include "myclprogram.h"
#include "myclerrors.h"

#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <QIODevice>
#include <QByteArray>

MyCLProgram::MyCLProgram()
    : mCreated(false)
{
}


bool MyCLProgram::create(MyCLWrapper *wrapper, QString sourceFilePath)
{
    mCLWrapper = wrapper;

    QFile sourceFile(sourceFilePath);
    if (!sourceFile.exists())
    {
        qDebug() << "Source file " << sourceFile.fileName() << " doesn't exist.";
        return false;
    }

    if (!sourceFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Could not open the source file " << sourceFile.fileName();
        return false;
    }

    QString sourceCode = QTextStream(&sourceFile).readAll();
    sourceFile.close();

    QByteArray bytes = sourceCode.toLatin1();
    const char *data = bytes.data();

    cl_int err;
    mProgram = clCreateProgramWithSource(wrapper->context(), 1, &data, NULL, &err);

    if (!mProgram || err != CL_SUCCESS)
    {
        // TODO create error parse for this
        qDebug() << "Failed to create program. Program: " << mProgram;
        return false;
    }

    err = clBuildProgram(mProgram, 0, NULL, NULL, NULL, NULL);

    if (err != CL_SUCCESS)
    {
        size_t length;
        char buffer[2048];

        clGetProgramBuildInfo(mProgram, wrapper->device(), CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &length);

        qDebug() << "Failed to build program.";
        qDebug() << QString::fromStdString(parseBuildReturnCode(err));
        qDebug() << buffer;

        return false;
    }

    mCreated = true;
    return true;
}

void MyCLProgram::destroy()
{
    Q_ASSERT(mCreated);

    clReleaseProgram(mProgram);

    mCreated = false;
}
