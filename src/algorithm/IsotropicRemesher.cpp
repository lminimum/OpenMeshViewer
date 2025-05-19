#include "IsotropicRemesher.h"
#include "MeshUtils.h"
#include <QVector>
#include <QDebug>
#include <cmath>
#include <QVector3D>

// 边长计算辅助函数
static float computeAverageEdgeLength(Mesh& mesh) {
    float totalLength = 0.0f;
    int edgeCount = 0;

    for (auto e_it = mesh.edges_begin(); e_it != mesh.edges_end(); ++e_it) {
        auto heh = mesh.halfedge_handle(*e_it, 0);
        auto from = mesh.point(mesh.from_vertex_handle(heh));
        auto to = mesh.point(mesh.to_vertex_handle(heh));
        totalLength += (from - to).norm();
        ++edgeCount;
    }
    return (edgeCount > 0) ? (totalLength / edgeCount) : 0.0f;
}

IsotropicRemesher::IsotropicRemesher(Mesh& mesh)
    : mesh_(mesh) {
    float avg = computeAverageEdgeLength(mesh_);
    minLength_ = 0.8f * avg;
    maxLength_ = 1.2f * avg;
    maxIter_ = 5;
}

void IsotropicRemesher::remesh() {
    for (int i = 0; i < maxIter_; ++i) {
        qDebug() << "[remesh] Iteration" << (i + 1);
        splitLongEdges();
        collapseShortEdges();
        flipEdges();
        smoothTangential();
    }
    mesh_.garbage_collection();
    qDebug() << "[remesh] Remeshing complete.";
}

void IsotropicRemesher::splitLongEdges() {
    QVector<Mesh::EdgeHandle> edgesToSplit;

    for (auto e_it = mesh_.edges_begin(); e_it != mesh_.edges_end(); ++e_it) {
        if (!mesh_.is_boundary(*e_it)) {
            auto heh = mesh_.halfedge_handle(*e_it, 0);
            auto from = mesh_.point(mesh_.from_vertex_handle(heh));
            auto to = mesh_.point(mesh_.to_vertex_handle(heh));
            float length = (from - to).norm();

            if (length > maxLength_) {
                edgesToSplit.push_back(*e_it);
            }
        }
    }

    for (auto& eh : edgesToSplit) {
        OpenMesh::Vec3f mid = mesh_.calc_edge_midpoint(eh);
        Mesh::VertexHandle vh = mesh_.add_vertex(mid);  // 💡 正确：先加顶点再 split
        mesh_.split_edge(eh, vh);
    }

    qDebug() << "[splitLongEdges] Split" << edgesToSplit.size() << "edges.";
}

void IsotropicRemesher::collapseShortEdges() {
    QVector<Mesh::HalfedgeHandle> edgesToCollapse;

    for (auto he_it = mesh_.halfedges_begin(); he_it != mesh_.halfedges_end(); ++he_it) {
        if (mesh_.is_collapse_ok(*he_it)) {
            auto from = mesh_.point(mesh_.from_vertex_handle(*he_it));
            auto to = mesh_.point(mesh_.to_vertex_handle(*he_it));
            float length = (from - to).norm();

            if (length < minLength_) {
                edgesToCollapse.push_back(*he_it);
            }
        }
    }

    for (auto& heh : edgesToCollapse) {
        if (mesh_.is_collapse_ok(heh)) {
            mesh_.collapse(heh);
        }
    }

    qDebug() << "[collapseShortEdges] Collapsed" << edgesToCollapse.size() << "edges.";
}

void IsotropicRemesher::flipEdges() {
    int flipCount = 0;

    for (auto e_it = mesh_.edges_begin(); e_it != mesh_.edges_end(); ++e_it) {
        if (mesh_.is_flip_ok(*e_it)) {
            mesh_.flip(*e_it);
            ++flipCount;
        }
    }

    qDebug() << "[flipEdges] Flipped" << flipCount << "edges.";
}

void IsotropicRemesher::smoothTangential() {
    std::vector<QVector3D> newPositions(mesh_.n_vertices());

    for (auto v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it) {
        if (!mesh_.is_boundary(*v_it)) {
            QVector3D centroid(0, 0, 0);
            int count = 0;
            for (auto vv_it = mesh_.vv_iter(*v_it); vv_it.is_valid(); ++vv_it) {
                centroid += toQVector3D(mesh_.point(*vv_it));
                ++count;
            }
            if (count > 0) {
                centroid /= count;
                QVector3D current = toQVector3D(mesh_.point(*v_it));
                QVector3D normal = toQVector3D(mesh_.normal(*v_it));
                QVector3D displacement = centroid - current;
                QVector3D tangential = displacement - QVector3D::dotProduct(displacement, normal) * normal;
                newPositions[v_it->idx()] = current + tangential * 0.5f;  // 0.5 控制平滑强度
            }
            else {
                newPositions[v_it->idx()] = toQVector3D(mesh_.point(*v_it));
            }
        }
        else {
            newPositions[v_it->idx()] = toQVector3D(mesh_.point(*v_it)); // 保持边界点不变
        }
    }

    for (auto v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it) {
        mesh_.set_point(*v_it, toVec3f(newPositions[v_it->idx()]));
    }

    qDebug() << "[smoothTangential] Smoothed vertices.";
}
