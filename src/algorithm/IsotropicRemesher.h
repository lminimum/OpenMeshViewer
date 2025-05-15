#pragma once

#include "../viewer/MeshViewerWidget.h"  
class IsotropicRemesher {
public:
    explicit IsotropicRemesher(Mesh& mesh);
    void remesh(int iterations = 3);

private:
    Mesh& mesh_;
};
