#ifndef ISOTROPIC_REMESHER_H
#define ISOTROPIC_REMESHER_H

#include "../viewer/MeshViewerWidget.h"  

class IsotropicRemesher {
public:
    IsotropicRemesher(Mesh& mesh);
    void remesh();

private:
    Mesh& mesh_;
    float targetMin_;
    float targetMax_;
    int maxIter_;

    void splitLongEdges();
    void collapseShortEdges();
    void flipEdges();
    void smoothTangential();
};

#endif // ISOTROPIC_REMESHER_H
