#include "grasswindclprogram.h"

#include "cl_interface/myclerrors.h"

#include <QFile>
#include <QString>
#include <QTextStream>
#include <QByteArray>
#include <QDebug>

#include <QtMath>


GrassWindCLProgram::GrassWindCLProgram()
    : mCreated(false),
      mProgram(),
      mGrassReact2Kernel()
{
}

GrassWindCLProgram::~GrassWindCLProgram()
{
    release();
}

bool GrassWindCLProgram::create(MyCLWrapper *wrapper)
{
    mCLWrapper = wrapper;

    if (!mProgram.create(wrapper, ":/compute/grassWindReact.cl"))
    {
        qDebug() << "Failed to create program. Program: " << mProgram.program();
        return false;
    }


    if (!mGrassReact2Kernel.createFromProgram(wrapper, mProgram.program(), "reactToWind2"))
    {
        qDebug() << "Failed to create reactToWind2 kernel.";
        return false;
    }

    mCreated = true;
    return true;
}

void GrassWindCLProgram::release()
{
    if (mCreated)
    {
        mGrassReact2Kernel.destroy();
        mProgram.destroy();

        mCreated = false;
    }
}


bool GrassWindCLProgram::reactToWind2(cl_mem grassWindOffsets,
                                      cl_mem grassPeriodOffsets,
                                      cl_mem grassNormalizedPositions,
                                      cl_image windVelocity,
                                      cl_uint numBlades,
                                      cl_float time)
{
    Q_ASSERT( mCreated );

    return mGrassReact2Kernel(numBlades,
                              grassWindOffsets,
                              grassPeriodOffsets,
                              grassNormalizedPositions,
                              windVelocity,
                              numBlades,
                              time);
}
