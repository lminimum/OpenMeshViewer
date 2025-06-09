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
    void collapseShortEdges(float low, float high);
	void equalizeValences();
    void tangentialRelaxation();  // 新增：切向松弛
    void projectToSurface();      // 新增：曲面投影

    // 辅助函数：判断点是否在三角形内（用于投影）
    bool isPointInsideTriangle(Mesh::FaceHandle fh, const Mesh::Point& p);
  /*  void flipEdges();
    void smoothTangential();*/
};

#endif // ISOTROPIC_REMESHER_H
