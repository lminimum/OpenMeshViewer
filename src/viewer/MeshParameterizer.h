#ifndef MESHPARAMETERIZER_H
#define MESHPARAMETERIZER_H

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <Eigen/Sparse>
#include <vector>

typedef OpenMesh::TriMesh_ArrayKernelT<> Mesh;

class MeshParameterizer
{
public:
    static bool parameterize(Mesh& mesh);  // 执行参数化
};

#endif // MESHPARAMETERIZER_H