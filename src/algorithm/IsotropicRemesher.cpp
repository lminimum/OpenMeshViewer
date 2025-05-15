#include "IsotropicRemesher.h"
#include <OpenMesh/Tools/Subdivider/Uniform/LoopT.hh>

IsotropicRemesher::IsotropicRemesher(Mesh& mesh)
    : mesh_(mesh) {
}

void IsotropicRemesher::remesh(int iterations) {
    OpenMesh::Subdivider::Uniform::LoopT<Mesh> loop;
    loop.attach(mesh_);
    loop(iterations);
    loop.detach();
}
