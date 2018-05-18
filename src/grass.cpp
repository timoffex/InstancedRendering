#include "grass.h"

#include <QtMath>
#include <QVector4D>
#include <QVector3D>
#include <QVector2D>
#include <QMatrix3x3>
#include <QMatrix4x4>

class Quad
{
    QVector4D _p1, _p2, _p3, _p4;
    QVector2D _t1, _t2, _t3, _t4;

public:

    Quad(const Quad &other) = default;
    Quad(QVector3D p1, QVector3D p2, QVector3D p3, QVector3D p4,
         QVector2D t1, QVector2D t2, QVector2D t3, QVector2D t4)
        : _p1(p1,1), _p2(p2,1), _p3(p3,1), _p4(p4,1),
          _t1(t1), _t2(t2), _t3(t3), _t4(t4)
    {}

    QVector3D p1() const { return _p1.toVector3D(); }
    QVector3D p2() const { return _p2.toVector3D(); }
    QVector3D p3() const { return _p3.toVector3D(); }
    QVector3D p4() const { return _p4.toVector3D(); }

    QVector2D t1() const { return _t1; }
    QVector2D t2() const { return _t2; }
    QVector2D t3() const { return _t3; }
    QVector2D t4() const { return _t4; }

    void appendToData(QVector<float> &data) const
    {
        data.append({
                        _p1.x(), _p1.y(), _p1.z(),  _t1.x(), _t1.y(),
                        _p2.x(), _p2.y(), _p2.z(),  _t2.x(), _t2.y(),
                        _p3.x(), _p3.y(), _p3.z(),  _t3.x(), _t3.y(),

                        _p1.x(), _p1.y(), _p1.z(),  _t1.x(), _t1.y(),
                        _p3.x(), _p3.y(), _p3.z(),  _t3.x(), _t3.y(),
                        _p4.x(), _p4.y(), _p4.z(),  _t4.x(), _t4.y(),
                    });
    }

    void translate(QVector3D offset)
    {
        QVector4D offsetW(offset);
        _p1 += offsetW;
        _p2 += offsetW;
        _p3 += offsetW;
        _p4 += offsetW;
    }

    void translateTex(QVector2D offset)
    {
        _t1 += offset;
        _t2 += offset;
        _t3 += offset;
        _t4 += offset;
    }
};

static Quad operator *(const QMatrix4x4 &mat, const Quad &quad)
{
    return Quad(mat * quad.p1(),
                mat * quad.p2(),
                mat * quad.p3(),
                mat * quad.p4(),
                quad.t1(), quad.t2(), quad.t3(), quad.t4());
}

/// Adds a quad. The texture coordinates are rotated 90 degrees
/// so that the bottom side of the quad corresponds to the left
/// side of the texture.
static void addVerticalQuadRotated(QVector<float> &data,
                            float xMin, float xMax,
                            float yMin, float yMax,
                            float txMin, float txMax,
                            float tyMin, float tyMax)
{
    data.append({
                    xMin, yMin, 0,  txMin, tyMax,
                    xMax, yMin, 0,  txMin, tyMin,
                    xMax, yMax, 0,  txMax, tyMin,

                    xMin, yMin, 0,  txMin, tyMax,
                    xMax, yMax, 0,  txMax, tyMin,
                    xMin, yMax, 0,  txMax, tyMax,
                });
}

QVector<float> Grass::makeGrassModel(int numSegments, float width, float height, float bendAngle)
{
    QVector<float> model;


    float w2 = width / 2;
    float dh = height / numSegments;
    float dtx = 1.0 / numSegments;
    float dBend = bendAngle / numSegments;

    Quad initialQuad(QVector3D(-w2, 0, 0), QVector3D(w2, 0, 0),
                 QVector3D(w2, dh, 0), QVector3D(-w2, dh, 0),
                 QVector2D(0, 1), QVector2D(0, 0),
                 QVector2D(dtx, 0), QVector2D(dtx, 1));

    QVector3D offsetDV(0, dh, 0);
    QVector3D offsetV(0, 0, 0);
    QVector2D offsetT(dtx, 0);

    QMatrix4x4 dRotation;
    dRotation.rotate(dBend, 1, 0);

    initialQuad.appendToData(model);
    for (int segIdx = 1; segIdx < numSegments; ++segIdx)
    {
        initialQuad = dRotation * initialQuad;
        offsetV += offsetDV;
        offsetDV = dRotation * offsetDV;

        Quad curQuad(initialQuad);
        curQuad.translate(offsetV);
        curQuad.translateTex(offsetT * segIdx);

        curQuad.appendToData(model);
    }


    return model;
}
