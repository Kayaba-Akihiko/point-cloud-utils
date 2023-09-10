
#include <npe.h>

#include "common/common.h"

#include <Eigen/Core>


namespace {

    template <typename MatrixIJK>
    void generate_cube_mesh(Eigen::Vector3d vox_origin, Eigen::Vector3d vox_size, double gap_fraction,
                            const MatrixIJK& in_ijk,
                            Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& out_v,
                            Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& out_f) {
        std::array<int, 36> indices = {
            //Top
            2, 7, 6,
            2, 3, 7,

            //Bottom
            0, 4, 5,
            0, 5, 1,

            //Left
            0, 2, 6,
            0, 6, 4,

            //Right
            1, 7, 3,
            1, 5, 7,

            //Front
            0, 3, 2,
            0, 1, 3,

            //Back
            4, 6, 7,
            4, 7, 5
        };


        std::array<float, 24> vertices = {
             0.0, 0.0, 1.0, //0
             1.0, 0.0, 1.0, //1
             0.0, 1.0, 1.0, //2
             1.0, 1.0, 1.0, //3
             0.0, 0.0, 0.0, //4
             1.0, 0.0, 0.0, //5
             0.0, 1.0, 0.0, //6
             1.0, 1.0, 0.0  //7
        };

        out_v.resize(8 * in_ijk.rows(), 3);
        out_f.resize(12 * in_ijk.rows(), 3);

        for (int i = 0; i < in_ijk.rows(); i += 1) {
            for (int vi = 0; vi < 8; vi += 1) {
                Eigen::Vector3d vertex(vertices[vi * 3 + 0] * (1.0 - gap_fraction) + 0.5 * gap_fraction,
                                       vertices[vi * 3 + 1] * (1.0 - gap_fraction) + 0.5 * gap_fraction,
                                       vertices[vi * 3 + 2] * (1.0 - gap_fraction) + 0.5 * gap_fraction);
                Eigen::Vector3d voxel_translation((double) in_ijk(i, 0), (double) in_ijk(i, 1), (double) in_ijk(i, 2));
                vertex += voxel_translation;
                vertex.array() *= vox_size.array();
                vertex += vox_origin;
                vertex = vertex.eval();

                out_v(i * 8 + vi, 0) = vertex[0];
                out_v(i * 8 + vi, 1) = vertex[1];
                out_v(i * 8 + vi, 2) = vertex[2];
            }

            for (int fi = 0; fi < 12; fi += 1) {
                out_f(i * 12 + fi, 0) = indices[fi*3+0] + i * 8;
                out_f(i * 12 + fi, 1) = indices[fi*3+1] + i * 8;
                out_f(i * 12 + fi, 2) = indices[fi*3+2] + i * 8;
            }
        }
    }
}


npe_function(_voxel_mesh_internal)
npe_arg(ijk, dense_int32, dense_int64, dense_uint32, dense_uint64)
npe_arg(gap_fraction, double)
npe_arg(vox_origin, dense_float, dense_double)
npe_arg(vox_size, dense_float, dense_double)
npe_begin_code()
    using MatrixI = Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    using MatrixF = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

    validate_point_cloud(ijk, false /*allow_0*/);
    if (vox_origin.size() != 3) {
        throw pybind11::value_error("Invalid shape");
    }

    if (vox_size.size() != 3) {
        throw pybind11::value_error("Invalid shape");
    }

    Eigen::Vector3d vsize = vox_size.template cast<double>();

    if (vsize[0] <= 0.0 || vsize[1] <= 0.0 || vsize[2] <= 0.0) {
        throw pybind11::value_error("Voxel size must be positive");
    }
    Eigen::Vector3d vorgn = vox_origin.template cast<double>();

    MatrixF geom_vertices;
    MatrixI geom_faces;

    generate_cube_mesh(vorgn, vsize, gap_fraction, ijk, geom_vertices, geom_faces);

    return std::make_tuple(npe::move(geom_vertices), npe::move(geom_faces));
npe_end_code()