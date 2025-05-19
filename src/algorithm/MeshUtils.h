// MeshUtils.h
#pragma once

#include <QVector3D>
#include <OpenMesh/Core/Geometry/VectorT.hh>

inline QVector3D toQVector3D(const OpenMesh::Vec3f& v) {
    return QVector3D(v[0], v[1], v[2]);
}

inline OpenMesh::Vec3f toVec3f(const QVector3D& v) {
    return OpenMesh::Vec3f(v.x(), v.y(), v.z());
}
