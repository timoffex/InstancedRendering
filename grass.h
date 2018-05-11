#ifndef GRASS_H
#define GRASS_H

#include <QVector>

namespace Grass {

QVector<float> makeGrassModel(int numSegments, float width, float height, float bendAngle);

}

#endif // GRASS_H
