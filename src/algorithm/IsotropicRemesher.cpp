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
	targetMax_ = 4.0f / 3.0f * L; 
    maxIter_ = 10;
}

void IsotropicRemesher::remesh() {
    for (int iter = 0; iter < maxIter_; ++iter) {
        qDebug() << "[remesh] Iteration" << iter + 1;

        splitLongEdges();
        mesh_.garbage_collection();
        collapseShortEdges(targetMin_, targetMax_);
        mesh_.garbage_collection();
        equalizeValences();
        mesh_.garbage_collection();

        tangentialRelaxation();  // 新增：切向松弛
        projectToSurface();      // 新增：曲面投影

        mesh_.update_normals();
    }
    qDebug() << "[remesh] Complete.";
}

void IsotropicRemesher::splitLongEdges() {
    std::unordered_set<Mesh::EdgeHandle> edgeSet;
    for (auto eh : mesh_.edges()) {
        if (!mesh_.status(eh).deleted() && !mesh_.is_boundary(eh))
            edgeSet.insert(eh);
    }

    int totalSplit = 0;

    while (!edgeSet.empty()) {
        auto iter = edgeSet.begin();
        Mesh::EdgeHandle eh = *iter;
        edgeSet.erase(iter);

        if (!mesh_.is_valid_handle(eh)) continue;
        if (mesh_.status(eh).deleted()) continue;

        float len = mesh_.calc_edge_length(eh);
        if (len > targetMax_) {
            // 拆分并返回中点顶点句柄
            auto mid = mesh_.calc_edge_midpoint(eh);
            auto vh = mesh_.add_vertex(mid);
            mesh_.split_edge(eh, vh);
            ++totalSplit;

            // 将新增顶点的邻接边加入集合
            for (auto ve_it = mesh_.ve_begin(vh); ve_it.is_valid(); ++ve_it) {
                if (!mesh_.status(*ve_it).deleted() && !mesh_.is_boundary(*ve_it))
                    edgeSet.insert(*ve_it);
            }
        }
    }

    qDebug() << "[splitLongEdges] Total splits:" << totalSplit;
}

void IsotropicRemesher::collapseShortEdges(float low, float high) {
    std::unordered_set<Mesh::EdgeHandle> edgeSet;
    for (auto eh : mesh_.edges()) {
        if (!mesh_.status(eh).deleted() && !mesh_.is_boundary(eh))
            edgeSet.insert(eh);
    }

    int totalCollapse = 0;

    while (!edgeSet.empty()) {
        auto iter = edgeSet.begin();
        Mesh::EdgeHandle eh = *iter;
        edgeSet.erase(iter);

        if (!mesh_.is_valid_handle(eh)) continue;
        if (mesh_.status(eh).deleted()) continue;

        float len = mesh_.calc_edge_length(eh);
        if (len >= low) continue;

        auto heh = mesh_.halfedge_handle(eh, 0);
        auto from_vh = mesh_.from_vertex_handle(heh);
        auto to_vh = mesh_.to_vertex_handle(heh);

        // 跳过边界顶点（不动边界）
        if (mesh_.is_boundary(from_vh) || mesh_.is_boundary(to_vh))
            continue;

        // 检查邻点是否有边界点（避免造成度数膨胀）
        bool hasBoundaryNeighbor = false;
        for (auto vv_it = mesh_.vv_begin(from_vh); vv_it.is_valid(); ++vv_it)
            if (mesh_.is_boundary(*vv_it)) { hasBoundaryNeighbor = true; break; }
        for (auto vv_it = mesh_.vv_begin(to_vh); vv_it.is_valid(); ++vv_it)
            if (mesh_.is_boundary(*vv_it)) { hasBoundaryNeighbor = true; break; }
        if (hasBoundaryNeighbor) continue;

        // collapse 后不能生成超过 high 的边
        bool willExceed = false;
        auto mid = (mesh_.point(from_vh) + mesh_.point(to_vh)) * 0.5f;
        for (auto vv_it = mesh_.vv_begin(from_vh); vv_it.is_valid(); ++vv_it) {
            if (*vv_it == to_vh) continue;
            if ((mesh_.point(*vv_it) - mid).norm() > high) {
                willExceed = true;
                break;
            }
        }
        if (willExceed) continue;

        if (mesh_.is_collapse_ok(heh)) {
            mesh_.set_point(to_vh, mid); // collapse 方向统一为 to_vh
            mesh_.collapse(heh);
            ++totalCollapse;

            // 将新顶点的邻边加入集合
            for (auto ve_it = mesh_.ve_begin(to_vh); ve_it.is_valid(); ++ve_it)
                if (!mesh_.status(*ve_it).deleted() && !mesh_.is_boundary(*ve_it))
                    edgeSet.insert(*ve_it);
        }
    }

    qDebug() << "[collapseShortEdges] Total collapses:" << totalCollapse;
}

void IsotropicRemesher::equalizeValences() {
    int flippedCount = 0;

    // 计算单个顶点理想度数
    auto optimalValence = [this](Mesh::VertexHandle vh) -> int {
        return mesh_.is_boundary(vh) ? 4 : 6;
        };

    // 计算给定4个顶点的凸凹情况：判断四边形是否是凸的
    auto isQuadConvex = [this](const Mesh::Point& p0, const Mesh::Point& p1, const Mesh::Point& p2, const Mesh::Point& p3) -> bool {
        // 利用向量叉积判断凸凹，四边形顶点顺序是 p0,p1,p2,p3（顺时针或逆时针）
        auto cross = [](const Mesh::Point& a, const Mesh::Point& b) {
            return a[0] * b[1] - a[1] * b[0]; // 2D cross product (only XY平面投影)
            };

        // 在3D中，需要投影到平面，这里简化为判断3D叉积方向一致性：
        // 计算四个边的法向量z方向分量符号一致则为凸四边形
        QVector3D v0 = QVector3D(p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]);
        QVector3D v1 = QVector3D(p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2]);
        QVector3D v2 = QVector3D(p3[0] - p2[0], p3[1] - p2[1], p3[2] - p2[2]);
        QVector3D v3 = QVector3D(p0[0] - p3[0], p0[1] - p3[1], p0[2] - p3[2]);

        QVector3D c0 = QVector3D::crossProduct(v0, v1);
        QVector3D c1 = QVector3D::crossProduct(v1, v2);
        QVector3D c2 = QVector3D::crossProduct(v2, v3);
        QVector3D c3 = QVector3D::crossProduct(v3, v0);

        float sign0 = QVector3D::dotProduct(c0, c1) >= 0 ? 1.0f : -1.0f;
        float sign1 = QVector3D::dotProduct(c1, c2) >= 0 ? 1.0f : -1.0f;
        float sign2 = QVector3D::dotProduct(c2, c3) >= 0 ? 1.0f : -1.0f;
        float sign3 = QVector3D::dotProduct(c3, c0) >= 0 ? 1.0f : -1.0f;

        // 如果符号都相同，则是凸四边形
        return (sign0 == sign1) && (sign1 == sign2) && (sign2 == sign3);
        };

    // 计算单个顶点度数
    auto valence = [this](Mesh::VertexHandle vh) -> int {
        int count = 0;
        for (auto vv_it = mesh_.vv_begin(vh); vv_it.is_valid(); ++vv_it) {
            if (!mesh_.status(*vv_it).deleted())
                ++count;
        }
        return count;
        };

    // 计算四个顶点的valence excess之和
    auto valenceExcessSum = [&](Mesh::VertexHandle v0, Mesh::VertexHandle v1, Mesh::VertexHandle v2, Mesh::VertexHandle v3) -> int {
        int sum = 0;
        sum += std::abs(valence(v0) - optimalValence(v0));
        sum += std::abs(valence(v1) - optimalValence(v1));
        sum += std::abs(valence(v2) - optimalValence(v2));
        sum += std::abs(valence(v3) - optimalValence(v3));
        return sum;
        };

    // 遍历所有边尝试翻转
    for (auto eh : mesh_.edges()) {
        if (mesh_.status(eh).deleted()) continue;
        if (mesh_.is_boundary(eh)) continue; // 跳过边界边

        // halfedges of this edge
        auto heh0 = mesh_.halfedge_handle(eh, 0);
        auto heh1 = mesh_.halfedge_handle(eh, 1);

        auto v0 = mesh_.from_vertex_handle(heh0);
        auto v1 = mesh_.to_vertex_handle(heh0);
        auto v2 = mesh_.to_vertex_handle(mesh_.next_halfedge_handle(heh0));
        auto v3 = mesh_.to_vertex_handle(mesh_.next_halfedge_handle(heh1));

        // 计算当前valence excess
        int oldExcess = valenceExcessSum(v0, v1, v2, v3);

        // 翻转后顶点度数变更：
        // v0, v1 减1，v2, v3 加1
        int newExcess =
            std::abs((valence(v0) - 1) - optimalValence(v0)) +
            std::abs((valence(v1) - 1) - optimalValence(v1)) +
            std::abs((valence(v2) + 1) - optimalValence(v2)) +
            std::abs((valence(v3) + 1) - optimalValence(v3));

        if (newExcess >= oldExcess)
            continue;

        // 检查翻转后内部点是否度数至少为3（不产生度数2的内部点）
        auto checkVertex = [&](Mesh::VertexHandle vh) {
            if (mesh_.is_boundary(vh)) return true; // 边界点允许度数为2
            int newVal = valence(vh);
            if (vh == v0 || vh == v1) newVal -= 1;
            if (vh == v2 || vh == v3) newVal += 1;
            return newVal >= 3;
            };

        if (!checkVertex(v0) || !checkVertex(v1) || !checkVertex(v2) || !checkVertex(v3))
            continue;

        // 判断四边形是否凸
        auto p0 = mesh_.point(v0);
        auto p1 = mesh_.point(v1);
        auto p2 = mesh_.point(v2);
        auto p3 = mesh_.point(v3);

        if (!isQuadConvex(p0, p2, p3, p1)) // 翻转后四边形顶点顺序需正确，这里根据flip后点顺序传参
            continue;

        // 检查翻转是否合法
        if (!mesh_.is_flip_ok(eh))
            continue;

        // 执行翻转
        mesh_.flip(eh);
        ++flippedCount;
    }


    qDebug() << "[equalizeValences] Total edge flips:" << flippedCount;
}

void IsotropicRemesher::tangentialRelaxation() {
    try {
        // 确保法线存在
        if (!mesh_.has_vertex_normals()) {
            mesh_.request_vertex_normals();
        }
        mesh_.update_normals();

        std::vector<Mesh::Point> newPositions(mesh_.n_vertices());
        float relaxationFactor = 0.2f;  // 平滑强度

        for (int i = 0; i < mesh_.n_vertices(); ++i) {
            auto vh = Mesh::VertexHandle(i);

            // 跳过无效或已删除顶点
            if (!vh.is_valid() || mesh_.status(vh).deleted()) {
                newPositions[i] = mesh_.point(vh);
                continue;
            }

            // 保持边界顶点不动
            if (mesh_.is_boundary(vh)) {
                newPositions[i] = mesh_.point(vh);
                continue;
            }

            // 计算邻居顶点的平均位置
            Mesh::Point avgPos(0.0f, 0.0f, 0.0f);
            int neighborCount = 0;
            for (auto vv_it = mesh_.vv_begin(vh); vv_it.is_valid(); ++vv_it) {
                auto neighborVh = *vv_it;
                if (!neighborVh.is_valid() || mesh_.status(neighborVh).deleted()) continue;
                avgPos += mesh_.point(neighborVh);
                neighborCount++;
            }

            if (neighborCount == 0) {
                newPositions[i] = mesh_.point(vh);
                continue;
            }

            avgPos /= static_cast<float>(neighborCount);

            // Laplacian 位移向量
            Mesh::Point displacement = avgPos - mesh_.point(vh);

            // 切向投影：使用 Gram-Schmidt 方法移除法线分量
            Mesh::Point normal = mesh_.normal(vh).normalized();
            Mesh::Point tangentialDisplacement = displacement - (normal | displacement) * normal;

            // 可选：限制最大移动距离
            float maxStep = targetMax_ * 0.5f;
            if (tangentialDisplacement.norm() > maxStep) {
                tangentialDisplacement = tangentialDisplacement.normalized() * maxStep;
            }

            newPositions[i] = mesh_.point(vh) + relaxationFactor * tangentialDisplacement;
        }

        // 应用更新
        for (int i = 0; i < static_cast<int>(newPositions.size()); ++i) {
            auto vh = Mesh::VertexHandle(i);
            if (vh.is_valid() && !mesh_.status(vh).deleted() && !mesh_.is_boundary(vh)) {
                mesh_.set_point(vh, newPositions[i]);
            }
        }

        std::cout << "[tangentialRelaxation] Tangential relaxation completed with full projection." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error in tangentialRelaxation: " << e.what() << std::endl;
        throw;
    }
}


void IsotropicRemesher::projectToSurface() {
    try {
        std::cout << "[projectToSurface] Request and update vertex normals" << std::endl;
        mesh_.request_vertex_normals();
        std::cout << "Normals requested" << std::endl;
        mesh_.update_vertex_normals();
        std::cout << "Normals updated" << std::endl;

        int vertexCount = 0;
        for (auto vh : mesh_.vertices()) {
            if (++vertexCount % 1000 == 0) {
                std::cout << "[projectToSurface] Processing vertex " << vertexCount << std::endl;
            }

            if (!vh.is_valid() || mesh_.status(vh).deleted()) {
                std::cout << "[projectToSurface] Skipping invalid vertex ID=" << vh.idx() << std::endl;
                continue;
            }

            if (mesh_.is_boundary(vh)) continue;

            Mesh::Point p = mesh_.point(vh);
            Mesh::Point normal = mesh_.normal(vh);
            float minDist = std::numeric_limits<float>::max();
            Mesh::Point closestPoint = p;

            int faceCount = 0;
            for (auto vf_it = mesh_.vf_begin(vh); vf_it.is_valid(); ++vf_it) {
                auto fh = *vf_it;
                if (++faceCount % 1000 == 0) {
                    std::cout << "[projectToSurface] Processing face " << faceCount << " for vertex " << vh.idx() << std::endl;
                }

                if (!fh.is_valid() || mesh_.status(fh).deleted()) {
                    std::cout << "[projectToSurface] Skipping invalid face ID=" << fh.idx() << std::endl;
                    continue;
                }

                Mesh::Point faceNormal = mesh_.normal(fh);
                Mesh::Point faceCenter(0.0, 0.0, 0.0);
                int validVerts = 0;

                for (auto fv_it = mesh_.fv_begin(fh); fv_it.is_valid(); ++fv_it) {
                    if (fv_it->is_valid() && !mesh_.status(*fv_it).deleted()) {
                        faceCenter += mesh_.point(*fv_it);
                        validVerts++;
                    }
                }

                if (validVerts != 3) continue;
                faceCenter /= 3.0f;

                Mesh::Point vToFace = faceCenter - p;
                float dist = vToFace | faceNormal;
                Mesh::Point projectedPoint = p + dist * faceNormal;

                if (isPointInsideTriangle(fh, projectedPoint)) {
                    float currentDist = (projectedPoint - p).norm();
                    if (currentDist < minDist) {
                        minDist = currentDist;
                        closestPoint = projectedPoint;
                    }
                }
            }

            if (vh.is_valid() && !mesh_.status(vh).deleted()) {
                mesh_.set_point(vh, closestPoint);
            }
        }
        std::cout << "[projectToSurface] Vertex projection completed, processed " << mesh_.n_vertices() << " vertices." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[projectToSurface] Exception: " << e.what() << std::endl;
        throw;
    }
}

// 辅助函数：判断点是否在三角形内（用重心坐标）
bool IsotropicRemesher::isPointInsideTriangle(Mesh::FaceHandle fh, const Mesh::Point& p) {
    // 获取三角形的三个顶点
    Mesh::ConstFaceVertexIter fv_it = mesh_.cfv_iter(fh);
    Mesh::Point p0 = mesh_.point(*fv_it); ++fv_it;
    Mesh::Point p1 = mesh_.point(*fv_it); ++fv_it;
    Mesh::Point p2 = mesh_.point(*fv_it);

    // 计算重心坐标
    Mesh::Point v0 = p1 - p0, v1 = p2 - p0, v2 = p - p0;
    float d00 = v0 | v0;
    float d01 = v0 | v1;
    float d11 = v1 | v1;
    float d20 = v2 | v0;
    float d21 = v2 | v1;
    float denom = d00 * d11 - d01 * d01;

    float alpha = (d11 * d20 - d01 * d21) / denom;
    float beta = (d00 * d21 - d01 * d20) / denom;
    float gamma = 1.0f - alpha - beta;

    // 如果重心坐标都在 [0,1] 范围内，则点在三角形内
    return (alpha >= 0) && (beta >= 0) && (gamma >= 0);
}