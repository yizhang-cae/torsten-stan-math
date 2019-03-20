#ifndef STAN_MATH_PRIM_MAT_PROB_NORMAL_ID_GLM_LPDF_HPP
#define STAN_MATH_PRIM_MAT_PROB_NORMAL_ID_GLM_LPDF_HPP

#include <stan/math/prim/scal/meta/is_constant_struct.hpp>
#include <stan/math/prim/scal/meta/partials_return_type.hpp>
#include <stan/math/prim/scal/meta/operands_and_partials.hpp>
#include <stan/math/prim/scal/err/check_consistent_sizes.hpp>
#include <stan/math/prim/scal/err/check_finite.hpp>
#include <stan/math/prim/scal/fun/constants.hpp>
#include <stan/math/prim/mat/fun/value_of.hpp>
#include <stan/math/prim/scal/meta/include_summand.hpp>
#include <stan/math/prim/mat/meta/is_vector.hpp>
#include <stan/math/prim/scal/meta/scalar_seq_view.hpp>
#include <stan/math/prim/scal/fun/sum.hpp>
#include <stan/math/prim/scal/fun/as_array.hpp>
#include <cmath>

namespace stan {
namespace math {

/**
 * Returns the log PDF of the Generalized Linear Model (GLM)
 * with Normal distribution and id link function.
 * If containers are supplied, returns the log sum of the probabilities.
 * The idea is that normal_id_glm_lpdf(y, x, alpha, beta, sigma) should
 * compute a more efficient version of normal_lpdf(y, alpha + x * beta, sigma)
 * by using analytically simplified gradients.
 * @tparam T_y type of vector of dependent variables (labels);
 * @tparam T_x type of the matrix of independent variables (features); this
 * should be an Eigen::Matrix type whose number of rows should match the
 * length of y and whose number of columns should match the length of beta
 * @tparam T_alpha type of the intercept(s);
 * this can be a vector (of the same length as y) of intercepts or a single
 * value (for models with constant intercept);
 * @tparam T_beta type of the weight vector;
 * this can also be a single value;
 * @tparam T_scale type of the (positive) scale(s);
 * this can be a vector (of the same length as y, for heteroskedasticity)
 * or a scalar.
 * @param y vector parameter
 * @param x design matrix
 * @param alpha intercept (in log odds)
 * @param beta weight vector
 * @param sigma (Sequence of) scale parameters for the normal
 * distribution.
 * @return log probability or log sum of probabilities
 * @throw std::domain_error if x, beta or alpha is infinite.
 * @throw std::domain_error if the scale is not positive.
 * @throw std::invalid_argument if container sizes mismatch.
 */
template <bool propto, typename T_y, typename T_x, typename T_alpha,
          typename T_beta, typename T_scale>
typename return_type<T_y, T_x, T_alpha, T_beta, T_scale>::type
normal_id_glm_lpdf(const T_y &y, const T_x &x, const T_alpha &alpha,
                   const T_beta &beta, const T_scale &sigma) {
  static const char *function = "normal_id_glm_lpdf";
  typedef typename stan::partials_return_type<T_y, T_x, T_alpha, T_beta,
                                              T_scale>::type T_partials_return;
  typedef typename std::conditional<
      is_vector<T_scale>::value,
      Eigen::Array<typename stan::partials_return_type<T_scale>::type, -1, 1>,
      typename stan::partials_return_type<T_scale>::type>::type T_scale_val;
//  typedef typename std::conditional<
//          is_vector<T_alpha>::value,
//          Eigen::Array<typename stan::partials_return_type<T_alpha>::type, -1, 1>,
//          typename stan::partials_return_type<T_alpha>::type>::type T_alpha_val;
//  typedef typename std::conditional<
//          is_vector<T_beta>::value,
//          Eigen::Array<typename stan::partials_return_type<T_beta>::type, -1, 1>,
//          typename stan::partials_return_type<T_beta>::type>::type T_beta_val;
//  typedef typename std::conditional<
//      is_vector<T_y>::value,
//      Eigen::Array<typename stan::partials_return_type<T_y>::type, -1, 1>,
//      typename stan::partials_return_type<T_y>::type>::type T_y_val;

  using Eigen::Array;
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using std::exp;

  if (!(stan::length(y) && stan::length(x) && stan::length(beta)
        && stan::length(sigma)))
    return 0.0;

  T_partials_return logp(0.0);

  const size_t N = x.col(0).size();
  const size_t M = x.row(0).size();

  check_finite(function, "Vector of dependent variables", y);
  check_finite(function, "Weight vector", beta);
  check_finite(function, "Intercept", alpha);
  check_positive_finite(function, "Scale vector", sigma);
  check_consistent_size(function, "Vector of dependent variables", y, N);
  check_consistent_size(function, "Weight vector", beta, M);
  if (is_vector<T_scale>::value)
    check_consistent_sizes(function, "Vector of scale parameters", sigma,
                           "Vector of dependent variables", y);
  if (is_vector<T_alpha>::value)
    check_consistent_sizes(function, "Vector of intercepts", alpha,
                           "Vector of dependent variables", y);

  if (!include_summand<propto, T_y, T_x, T_alpha, T_beta, T_scale>::value)
    return 0.0;

  // Set up data structures and compute log-density.
  if (include_summand<propto>::value)
    logp += NEG_LOG_SQRT_TWO_PI * N;

  const auto& x_val = value_of(x);
  const auto& beta_dbl = value_of(beta);
//  Matrix<T_partials_return, Dynamic, 1> beta_dbl(M, 1);
//  {
//    scalar_seq_view<T_beta> beta_vec(beta);
//    for (size_t m = 0; m < M; ++m) {
//      beta_dbl[m] = value_of(beta_vec[m]);
//    }
//  }
  const auto &alpha_val = value_of(alpha);
  const auto &sigma_val = value_of(sigma);
  const auto &y_val = value_of(y);

  T_scale_val inv_sigma = 1 / as_array(sigma_val);
  Array<T_partials_return, Dynamic, 1> y_minus_mu_over_sigma
      = (as_array(y_val) - (x_val * beta_dbl).array() - as_array(alpha_val)) * inv_sigma;
  Matrix<T_partials_return, Dynamic, 1> y_minus_mu_over_sigma_squared
      = y_minus_mu_over_sigma * y_minus_mu_over_sigma;
  check_finite(function, "Matrix of independent variables",
               y_minus_mu_over_sigma_squared);
  if (include_summand<propto, T_scale>::value) {
    if (is_vector<T_scale>::value)
      logp -= sum(log(sigma_val));
    else
      logp -= N * sum(log(sigma_val));
  }

  double y_minus_mu_over_sigma_squared_sum;
  if(include_summand<propto, T_y, T_x, T_alpha, T_beta, T_scale>::value ||
      (!is_constant_struct<T_scale>::value && !is_vector<T_scale>::value)){
    y_minus_mu_over_sigma_squared_sum = y_minus_mu_over_sigma_squared.sum();
  }

  if (include_summand<propto, T_y, T_x, T_alpha, T_beta, T_scale>::value)
    logp -= 0.5 * y_minus_mu_over_sigma_squared_sum;

  // Compute the necessary derivatives.
  operands_and_partials<T_y, T_x, T_alpha, T_beta, T_scale> ops_partials(
      y, x, alpha, beta, sigma);
  if (!(is_constant_struct<T_y>::value && is_constant_struct<T_x>::value
        && is_constant_struct<T_beta>::value
        && is_constant_struct<T_alpha>::value)) {
    Matrix<T_partials_return, Dynamic, 1> mu_derivative
        = (inv_sigma * y_minus_mu_over_sigma).matrix();
    if (!is_constant_struct<T_y>::value) {
      ops_partials.edge1_.partials_ = -mu_derivative;
    }
    if (!is_constant_struct<T_x>::value) {
      ops_partials.edge2_.partials_ = mu_derivative * beta_dbl.transpose();
    }
    if (!is_constant_struct<T_beta>::value) {
      ops_partials.edge4_.partials_ = mu_derivative.transpose() * x_val;
    }
    if (!is_constant_struct<T_alpha>::value) {
      if (is_vector<T_alpha>::value)
        ops_partials.edge3_.partials_ = mu_derivative;
      else
        ops_partials.edge3_.partials_[0] = mu_derivative.sum();
    }
    if (!is_constant_struct<T_scale>::value) {
      if (is_vector<T_scale>::value) {
        ops_partials.edge5_.partials_
            = (y_minus_mu_over_sigma_squared.array() - 1) * inv_sigma;
      } else {
        ops_partials.edge5_.partials_[0]
            = (y_minus_mu_over_sigma_squared_sum - N) * inv_sigma;
      }
    }
  }
  return ops_partials.build(logp);
}

template <typename T_y, typename T_x, typename T_alpha, typename T_beta,
          typename T_scale>
inline typename return_type<T_y, T_x, T_alpha, T_beta, T_scale>::type
normal_id_glm_lpdf(const T_y &y, const T_x &x, const T_alpha &alpha,
                   const T_beta &beta, const T_scale &sigma) {
  return normal_id_glm_lpdf<false>(y, x, alpha, beta, sigma);
}
}  // namespace math
}  // namespace stan
#endif
