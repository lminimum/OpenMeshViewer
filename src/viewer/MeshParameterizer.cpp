#include "MeshParameterizer.h"
#include <cmath>
#include <map>

bool MeshParameterizer::parameterize(Mesh& mesh)
{
    mesh.request_vertex_texcoords2D();
    mesh.request_vertex_status();
    mesh.request_edge_status();
    mesh.request_face_status();

    // 找边界环
    std::vector<Mesh::VertexHandle> boundary;
    for (auto vh : mesh.vertices())
    {
        if (mesh.is_boundary(vh))
            boundary.push_back(vh);
    }

    if (boundary.empty()) return false;

    int n = boundary.size();

    // 将边界点均匀映射到单位圆
    for (int i = 0; i < n; ++i)
    {
        float angle = 2.0f * M_PI * i / n;
        float x = std::cos(angle);
        float y = std::sin(angle);
        mesh.set_texcoord2D(boundary[i], Mesh::TexCoord2D(x, y));
    }

    // 创建内部顶点索引映射
    std::map<int, int> idxMap;
    std::vector<int> internalVtxIdx;
    int idx = 0;

    for (auto vh : mesh.vertices())
    {
        if (!mesh.is_boundary(vh))
        {
            idxMap[vh.idx()] = idx++;
            internalVtxIdx.push_back(vh.idx());
        }
    }

    int N = internalVtxIdx.size();

    typedef Eigen::SparseMatrix<double> SpMat;
    typedef Eigen::Triplet<double> T;
    std::vector<T> triplets;
    Eigen::VectorXd bx(N), by(N);
    bx.setZero(); by.setZero();

    // 构建稀疏矩阵
    for (int row = 0; row < N; ++row)
    {
        int vi = internalVtxIdx[row];
        Mesh::VertexHandle vh = mesh.vertex_handle(vi);
        std::vector<int> neighbors;

        for (auto vv_it = mesh.vv_iter(vh); vv_it.is_valid(); ++vv_it)
        {
            neighbors.push_back(vv_it->idx());
        }

        double weight = 1.0 / neighbors.size();
        triplets.push_back(T(row, row, 1.0));

        for (int vj : neighbors)
        {
            if (!mesh.is_boundary(mesh.vertex_handle(vj)))
            {
                triplets.push_back(T(row, idxMap[vj], -weight));
            }
            else
            {
                auto uv = mesh.texcoord2D(mesh.vertex_handle(vj));
                bx[row] += weight * uv[0];
                by[row] += weight * uv[1];
            }
        }
    }

    SpMat A(N, N);
    A.setFromTriplets(triplets.begin(), triplets.end());

    // 求解线性系统
    Eigen::SimplicialLDLT<SpMat> solver;
    solver.compute(A);
    Eigen::VectorXd x = solver.solve(bx);
    Eigen::VectorXd y = solver.solve(by);

    // 写回 UV
    for (int row = 0; row < N; ++row)
    {
        int vi = internalVtxIdx[row];
        mesh.set_texcoord2D(mesh.vertex_handle(vi), Mesh::TexCoord2D(x[row], y[row]));
    }

    return true;
}
