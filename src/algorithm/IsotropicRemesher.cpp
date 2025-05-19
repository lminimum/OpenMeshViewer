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
    // —— 一次性开启所有需要的 Status / Normals 属性 ——  
    mesh_.request_vertex_status();
    mesh_.request_edge_status();
    mesh_.request_face_status();
    mesh_.request_halfedge_status();

    mesh_.request_face_normals();
    mesh_.request_vertex_normals();
    mesh_.update_normals();

    // 计算目标边长
    float L = computeTargetLength(mesh_);
    targetMin_ = 0.8f * L;
    targetMax_ = 1.2f * L;
    maxIter_ = 5;
}

void IsotropicRemesher::remesh() {
    for (int iter = 0; iter < maxIter_; ++iter) {
        qDebug() << "[remesh] Iteration" << iter + 1;

        splitLongEdges();
        mesh_.garbage_collection();    // —— 回收后属性数组自动 resize ——  
        mesh_.update_normals();        // —— 为所有顶点重新计算法线 ——  

        collapseShortEdges();
        mesh_.garbage_collection();
        mesh_.update_normals();

        flipEdges();
        // flip 不会标记删除但仍可能影响法线
        mesh_.update_normals();

        smoothTangential();
        mesh_.update_normals();
    }
    qDebug() << "[remesh] Complete";
}

void IsotropicRemesher::splitLongEdges() {
    QVector<Mesh::EdgeHandle> toSplit;
    for (auto eh : mesh_.edges()) {
        if (mesh_.is_boundary(eh)) continue;
        float len = mesh_.calc_edge_length(eh);
        if (len > targetMax_)
            toSplit.push_back(eh);
    }
    for (auto eh : toSplit) {
        // 新顶点一定要在 split 前加入，OpenMesh 会自动为新顶点分配所有已开启的属性槽
        auto mid = mesh_.calc_edge_midpoint(eh);
        auto vh = mesh_.add_vertex(mid);
        mesh_.split_edge(eh, vh);
    }
    qDebug() << "[split] Splitted" << toSplit.size() << "edges.";
}

void IsotropicRemesher::collapseShortEdges() {
    QVector<Mesh::HalfedgeHandle> toCollapse;
    for (auto heh : mesh_.halfedges()) {
        if (!mesh_.is_collapse_ok(heh)) continue;
        float len = (mesh_.point(mesh_.from_vertex_handle(heh))
            - mesh_.point(mesh_.to_vertex_handle(heh))).norm();
        if (len < targetMin_)
            toCollapse.push_back(heh);
    }
    int cnt = 0;
    for (auto heh : toCollapse) {
        if (mesh_.is_collapse_ok(heh)) {
            mesh_.collapse(heh);
            ++cnt;
        }
    }
    qDebug() << "[collapse] Collapsed" << cnt << "edges.";
}


// —— 翻转可翻边以改善顶点度数 ——
void IsotropicRemesher::flipEdges() {
    int cnt = 0;
    for (auto e_it = mesh_.edges_begin(); e_it != mesh_.edges_end(); ++e_it) {
        if (mesh_.is_flip_ok(*e_it)) {
            mesh_.flip(*e_it);
            ++cnt;
        }
    }
    qDebug() << "[flip] Flipped" << cnt << "edges";
}

// —— 切线空间内拉普拉斯平滑 ——
void IsotropicRemesher::smoothTangential() {
    std::vector<OpenMesh::Vec3f> newPos(mesh_.n_vertices());
    // 第一步：计算新位置
    for (auto v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it) {
        if (mesh_.is_boundary(*v_it)) {
            newPos[v_it->idx()] = mesh_.point(*v_it);
            continue;
        }
        // 1) 计算邻点质心
        QVector3D C(0, 0, 0);
        int nb = 0;
        for (auto vv_it = mesh_.vv_iter(*v_it); vv_it.is_valid(); ++vv_it) {
            C += toQVector3D(mesh_.point(*vv_it));
            ++nb;
        }
        C /= float(nb);
        // 2) 切线位移
        QVector3D P = toQVector3D(mesh_.point(*v_it));
        QVector3D N = toQVector3D(mesh_.normal(*v_it));
        QVector3D disp = C - P;
        QVector3D tan = disp - QVector3D::dotProduct(disp, N) * N;
        // 0.5 是推荐的平滑因子
        QVector3D Q = P + 0.5f * tan;
        newPos[v_it->idx()] = toVec3f(Q);
    }
    // 第二步：赋值
    for (auto v_it = mesh_.vertices_begin(); v_it != mesh_.vertices_end(); ++v_it) {
        mesh_.set_point(*v_it, newPos[v_it->idx()]);
    }
    qDebug() << "[smooth] Tangential smoothing done";
}
