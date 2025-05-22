#include "IsotropicRemesher.h"
#include <OpenMesh/Core/IO/MeshIO.hh>
#include "MeshUtils.h"
#include <QVector>
#include <QDebug>
#include <cmath>
#include <QVector3D>

// —— 辅助：计算全网平均边长 ——
static float computeTargetLength(Mesh& mesh) {
    double sum = 0.0;
    size_t cnt = 0;
    for (auto e_it = mesh.edges_begin(); e_it != mesh.edges_end(); ++e_it) {
        auto heh = mesh.halfedge_handle(*e_it, 0);
        auto p0 = mesh.point(mesh.from_vertex_handle(heh));
        auto p1 = mesh.point(mesh.to_vertex_handle(heh));
        sum += (p0 - p1).norm();
        ++cnt;
    }
    return cnt ? float(sum / cnt) : 0.0f;
}

IsotropicRemesher::IsotropicRemesher(Mesh& mesh)
    : mesh_(mesh)
{
    mesh_.request_vertex_status();
    mesh_.request_edge_status();
    mesh_.request_face_status();
    mesh_.request_halfedge_status();

    mesh_.request_face_normals();
    mesh_.request_vertex_normals();
    mesh_.update_normals();

    float L = computeTargetLength(mesh_);
    targetMin_ = 0.8f * L;
    targetMax_ = 1.2f * L;
    maxIter_ = 5;
}

void IsotropicRemesher::remesh() {
    for (int iter = 0; iter < maxIter_; ++iter) {
        qDebug() << "[remesh] Iteration" << iter + 1;

        splitLongEdges();
        mesh_.garbage_collection();
        mesh_.update_normals();
    }
    qDebug() << "[remesh] Complete (Edge Split only)";
}

// —— 仅执行长边拆分（Edge Split）操作 ——
void IsotropicRemesher::splitLongEdges() {
    QVector<Mesh::EdgeHandle> toSplit;
    for (auto eh : mesh_.edges()) {
        if (mesh_.is_boundary(eh)) continue;
        float len = mesh_.calc_edge_length(eh);
        if (len > targetMax_)
            toSplit.push_back(eh);
    }

    for (auto eh : toSplit) {
        auto mid = mesh_.calc_edge_midpoint(eh);
        auto vh = mesh_.add_vertex(mid);
        mesh_.split_edge(eh, vh);
    }

    qDebug() << "[split] Splitted" << toSplit.size() << "edges.";
}
