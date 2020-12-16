#include <stan/math/rev.hpp>
#include <test/unit/math/opencl/util.hpp>
#include <test/unit/util.hpp>
#include <gtest/gtest.h>
#include <vector>

TEST(MathMatrixHeadCL, Head_size) {
  using stan::math::head;
  using stan::math::matrix_cl;
  using stan::math::var_value;
  matrix_cl<double> v(3, 1);
  EXPECT_EQ(0, head(v, 0).eval().size());
  EXPECT_EQ(3, head(v, 3).eval().size());
  EXPECT_THROW(head(v, 4), std::out_of_range);

  matrix_cl<double> rv(1, 3);
  EXPECT_EQ(0, head(rv, 0).eval().size());
  EXPECT_EQ(3, head(rv, 3).eval().size());
  EXPECT_THROW(head(rv, 4), std::out_of_range);

  var_value<matrix_cl<double>> v_var = v;
  EXPECT_EQ(0, head(v_var, 0).size());
  EXPECT_EQ(3, head(v_var, 3).size());
  EXPECT_THROW(head(v_var, 4), std::out_of_range);

  var_value<matrix_cl<double>> rv_var = rv;
  EXPECT_EQ(0, head(rv_var, 0).size());
  EXPECT_EQ(3, head(rv_var, 3).size());
  EXPECT_THROW(head(rv_var, 4), std::out_of_range);
}

TEST(MathMatrixHeadCL, HeadVector4) {
  using stan::math::head;
  Eigen::VectorXd v(3);
  v << 1, 2, 3;

  Eigen::VectorXd v01 = head(v, 2);
  EXPECT_EQ(2, v01.size());
  for (int n = 0; n < 2; ++n)
    EXPECT_FLOAT_EQ(v[n], v01[n]);
}

auto head_functor
    = [](const auto& a) { return stan::math::head(a, 5); };

TEST(MathMatrixCL, head_value_check_vector) {
  stan::math::vector_d m1(9);
  m1 << 1, 2, 3, 4, 5, 6, 7, 8, 9;

  stan::math::test::compare_cpu_opencl_prim(head_functor, m1);

  stan::math::vector_v m2 = m1;
  stan::math::vector_v m3 = m1;
  stan::math::var_value<stan::math::matrix_cl<double>> m3_cl = stan::math::to_matrix_cl(m3);
  stan::math::matrix_v m2_res = stan::math::head(m2, 5);
  stan::math::var_value<stan::math::matrix_cl<double>> m3_res_cl = stan::math::head(m3_cl, 5);
  stan::math::var s = sum(m2_res) + sum(m3_res_cl);
  s.grad();

  EXPECT_MATRIX_NEAR(m2_res.val(), stan::math::from_matrix_cl(m3_res_cl.val()), 1E-8);
  EXPECT_MATRIX_NEAR(m2.adj(), m3.adj(), 1E-8);
}

TEST(MathMatrixCL, head_value_check_row_vector) {
  stan::math::row_vector_d m1(9);
  m1 << 1, 2, 3, 4, 5, 6, 7, 8, 9;

  stan::math::test::compare_cpu_opencl_prim(head_functor, m1);

  stan::math::row_vector_v m2 = m1;
  stan::math::row_vector_v m3 = m1;
  stan::math::var_value<stan::math::matrix_cl<double>> m3_cl = stan::math::to_matrix_cl(m3);
  stan::math::matrix_v m2_res = stan::math::head(m2, 5);
  stan::math::var_value<stan::math::matrix_cl<double>> m3_res_cl = stan::math::head(m3_cl, 5);
  stan::math::var s = sum(m2_res) + sum(m3_res_cl);
  s.grad();

  EXPECT_MATRIX_NEAR(m2_res.val(), stan::math::from_matrix_cl(m3_res_cl.val()), 1E-8);
  EXPECT_MATRIX_NEAR(m2.adj(), m3.adj(), 1E-8);
}