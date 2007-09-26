/**
 * @file local_expansion.h
 *
 * The header file for the local expansion
 */

#ifndef LOCAL_EXPANSION
#define LOCAL_EXPANSION

#include <values.h>

#include "fastlib/fastlib.h"
#include "kernel_derivative.h"
#include "series_expansion_aux.h"

template<typename TKernel, typename TKernelDerivative>
class FarFieldExpansion;

/**
 * Local expansion class
 */
template<typename TKernel, typename TKernelDerivative>
class LocalExpansion {
  FORBID_COPY(LocalExpansion);
  
   public:
  
  typedef TKernel Kernel;
  
  typedef TKernelDerivative KernelDerivative;

 private:
  
  /** The type of the kernel */
  Kernel kernel_;
  
  /** The center of the expansion */
  Vector center_;
  
  /** The coefficients */
  Vector coeffs_;
  
  /** order */
  int order_;
  
  /** precomputed quantities */
  SeriesExpansionAux *sea_;
  
  /** Derivative computer based on the kernel passed in */
  KernelDerivative kd_;

 public:
  
  LocalExpansion() {}
  
  ~LocalExpansion() {}
  
  // getters and setters
  
  /** Get the coefficients */
  double bandwidth_sq() const { return kernel_.bandwidth_sq(); }
  
  /** Get the center of expansion */
  const Vector& get_center() const { return center_; }
  
  Vector& get_center() { return center_; }

  /** Get the coefficients */
  const Vector& get_coeffs() const { return coeffs_; }
  
  /** Get the approximation order */
  int get_order() const { return order_; }

  /** Get the maximum possible approximation order */
  int get_max_order() const { return sea_->get_max_order(); }

  /** Set the approximation order */
  void set_order(int new_order) { order_ = new_order; }

  // interesting functions...
  
  /**
   * Accumulates the local moment represented by the given reference
   * data into the coefficients
   */
  void AccumulateCoeffs(const Matrix& data, const Vector& weights,
			int begin, int end, int order);

  /**
   * This does not apply for local coefficients.
   */
  void RefineCoeffs(const Matrix& data, const Vector& weights,
		    int begin, int end, int order) { }
  
  /**
   * Evaluates the local coefficients at the given point
   */
  double EvaluateField(Matrix* data=NULL, int row_num=-1,
		       Vector* x_q=NULL) const;
  
  /**
   * Initializes the current local expansion object with the given
   * center.
   */
  void Init(double bandwidth, const Vector& center, 
	    SeriesExpansionAux *sea);

  void Init(double bandwidth, SeriesExpansionAux *sea);
  
  /**
   * Computes the required order for evaluating the local expansion
   * for any query point within the specified region for a given bound.
   */
  int OrderForEvaluating(const DHrectBound<2> &local_region,
			 double min_dist_sqd_regions,
                         double max_error, double *actual_error) const;

  /**
   * Prints out the series expansion represented by this object.
   */
  void PrintDebug(const char *name="", FILE *stream=stderr) const;

  /**
   * Translate from a far field expansion to the expansion here.
   * The translated coefficients are added up to the ones here.
   */
  void TranslateFromFarField
    (const FarFieldExpansion<TKernel, TKernelDerivative> &se);
  
  /**
   * Translate to the given local expansion. The translated coefficients
   * are added up to the passed-in local expansion coefficients.
   */
  void TranslateToLocal
    (LocalExpansion<TKernel, TKernelDerivative> &se);

};

template<typename TKernel, typename TKernelDerivative>
void LocalExpansion<TKernel, TKernelDerivative>::AccumulateCoeffs
(const Matrix& data, const Vector& weights, 
 int begin, int end, int order) {

  if(order > order_) {
    order_ = order;
  }

  int dim = sea_->get_dimension();
  int total_num_coeffs = sea_->get_total_num_coeffs(order);
  
  // get inverse factorials (precomputed)
  Vector neg_inv_multiindex_factorials;
  neg_inv_multiindex_factorials.Alias
    (sea_->get_neg_inv_multiindex_factorials());

  // declare deritave mapping
  Matrix derivative_map;
  derivative_map.Init(dim, order + 1);
  
  // some temporary variables
  Vector arrtmp;
  arrtmp.Init(total_num_coeffs);
  Vector x_r_minus_x_Q;
  x_r_minus_x_Q.Init(dim);
  
  // sqrt two times bandwidth
  double bandwidth_factor = kd_.BandwidthFactor(kernel_.bandwidth_sq());
  
  // for each data point,
  for(index_t r = begin; r < end; r++) {
    
    // calculate x_r - x_Q
    for(index_t d = 0; d < dim; d++) {
      x_r_minus_x_Q[d] = (center_[d] - data.get(d, r)) / 
	bandwidth_factor;
    }
    
    // precompute necessary partial derivatives based on coordinate difference
    kd_.ComputeDirectionalDerivatives(x_r_minus_x_Q, derivative_map);
    
    // compute h_{beta}((x_r - x_Q) / sqrt(2h^2))
    for(index_t j = 0; j < total_num_coeffs; j++) {
      ArrayList<int> mapping = sea_->get_multiindex(j);
      arrtmp[j] = kd_.ComputePartialDerivative(derivative_map, mapping);
    }

    for(index_t j = 0; j < total_num_coeffs; j++) {
      coeffs_[j] += neg_inv_multiindex_factorials[j] * weights[r] * 
	arrtmp[j];
    }
  } // End of looping through each reference point.
}

template<typename TKernel, typename TKernelDerivative>
  void LocalExpansion<TKernel, TKernelDerivative>::PrintDebug
  (const char *name, FILE *stream) const {
  
    
  int dim = sea_->get_dimension();
  int total_num_coeffs = sea_->get_total_num_coeffs(order_);

  fprintf(stream, "----- SERIESEXPANSION %s ------\n", name);
  fprintf(stream, "Local expansion\n");
  fprintf(stream, "Center: ");
  
  for (index_t i = 0; i < center_.length(); i++) {
    fprintf(stream, "%g ", center_[i]);
  }
  fprintf(stream, "\n");
  
  fprintf(stream, "f(");
  for(index_t d = 0; d < dim; d++) {
    fprintf(stream, "x_q%d", d);
    if(d < dim - 1)
      fprintf(stream, ",");
  }
  fprintf(stream, ") = \\sum\\limits_{x_r \\in R} K(||x_q - x_r||) = ");
  
  for (index_t i = 0; i < total_num_coeffs; i++) {
    ArrayList<int> mapping = sea_->get_multiindex(i);
    fprintf(stream, "%g", coeffs_[i]);
    
    for(index_t d = 0; d < dim; d++) {
      fprintf(stream, "(x_q%d - (%g))^%d ", d, center_[d], mapping[d]);
    }

    if(i < total_num_coeffs - 1) {
      fprintf(stream, " + ");
    }
  }
  fprintf(stream, "\n");
}

template<typename TKernel, typename TKernelDerivative>
  double LocalExpansion<TKernel, TKernelDerivative>::
  EvaluateField(Matrix* data, int row_num, Vector* x_q) const {
  
  index_t k, t, tail;
  
  // total number of coefficient
  int total_num_coeffs = sea_->get_total_num_coeffs(order_);

  // number of dimensions
  int dim = sea_->get_dimension();

  // evaluated sum to be returned
  double sum = 0;
  
  // sqrt two bandwidth
  double bandwidth_factor = kd_.BandwidthFactor(kernel_.bandwidth_sq());

  // temporary variable
  Vector x_Q_to_x_q;
  x_Q_to_x_q.Init(dim);
  Vector tmp;
  tmp.Init(total_num_coeffs);
  ArrayList<int> heads;
  heads.Init(dim + 1);
  
  // compute (x_q - x_Q) / (sqrt(2h^2))
  for(index_t i = 0; i < dim; i++) {
    
    if(data == NULL) {
      x_Q_to_x_q[i] = ((*x_q)[i] - center_[i]) / bandwidth_factor;
    }
    else {
      x_Q_to_x_q[i] = (data->get(i, row_num) - center_[i]) / 
	bandwidth_factor;
    }
  }
  
  for(index_t i = 0; i < dim; i++)
    heads[i] = 0;
  heads[dim] = MAXINT;

  tmp[0] = 1.0;

  for(k = 1, t = 1, tail = 1; k <= order_; k++, tail = t) {

    for(index_t i = 0; i < dim; i++) {
      int head = heads[i];
      heads[i] = t;

      for(index_t j = head; j < tail; j++, t++) {
        tmp[t] = tmp[j] * x_Q_to_x_q[i];
      }
    }
  }

  for(index_t i = 0; i < total_num_coeffs; i++) {
    sum += coeffs_[i] * tmp[i];
  }

  return sum;
}

template<typename TKernel, typename TKernelDerivative>
  void LocalExpansion<TKernel, TKernelDerivative>::Init
  (double bandwidth, const Vector& center, SeriesExpansionAux *sea) {
  
  // copy kernel type, center, and bandwidth squared
  kernel_.Init(bandwidth);
  center_.Copy(center);
  order_ = 0;
  sea_ = sea;

  // initialize coefficient array
  coeffs_.Init(sea_->get_max_total_num_coeffs());
  coeffs_.SetZero();
}

template<typename TKernel, typename TKernelDerivative>
  void LocalExpansion<TKernel, TKernelDerivative>::Init
  (double bandwidth, SeriesExpansionAux *sea) {
  
  // copy kernel type, center, and bandwidth squared
  kernel_.Init(bandwidth);
  center_.Init(sea->get_dimension());
  order_ = 0;
  sea_ = sea;

  // initialize coefficient array
  coeffs_.Init(sea_->get_max_total_num_coeffs());
  coeffs_.SetZero();
}

template<typename TKernel, typename TKernelDerivative>
  int LocalExpansion<TKernel, TKernelDerivative>::OrderForEvaluating
  (const DHrectBound<2> &local_region, double min_dist_sqd_regions,
   double max_error, double *actual_error) const {
  
  double frontfactor =
    exp(-min_dist_sqd_regions / (4 * kernel_.bandwidth_sq()));
  double widest_width = 0;
  int dim = local_region.dim();
  int max_order = sea_->get_max_order();
  
  // find out the widest dimension and its length
  for(index_t d = 0; d < dim; d++) {
    DRange range = local_region.get(d);
    widest_width = max(widest_width, range.width());
  }
  
  double two_bandwidth = 2 * sqrt(kernel_.bandwidth_sq());
  double r = widest_width / two_bandwidth;
  
  // This is not really necessary for O(D^p) expansion, but it is for
  // speeding up the convergence of the Taylor expansion.
  if(r >= 1.0)
    return -1;
  
  double r_raised_to_p_alpha = 1.0;
  double ret;
  int p_alpha = 0;
  double floor_fact, ceil_fact;
  int remainder;
  
  do {
    
    if(p_alpha > max_order)
      return -1;
    
    r_raised_to_p_alpha *= r;
    
    floor_fact =
      sea_->factorial((int)floor(((double) p_alpha) / ((double) dim)));
    ceil_fact =
      sea_->factorial((int)ceil(((double) p_alpha) / ((double) dim)));
    
    if(floor_fact < 0.0 || ceil_fact < 0.0)
      return -1;
    
    remainder = p_alpha % dim;
    
    ret = frontfactor *
      (sea_->get_total_num_coeffs(p_alpha + 1) -
       sea_->get_total_num_coeffs(p_alpha)) * r_raised_to_p_alpha /
      sqrt(pow(floor_fact, dim - remainder) * pow(ceil_fact, remainder));
    
    if(ret > max_error) {
      p_alpha++;
    }
    else {
      break;
    }
  } while(1);
  
  *actual_error = ret;
  return p_alpha;
}

template<typename TKernel, typename TKernelDerivative>
  void LocalExpansion<TKernel, TKernelDerivative>::TranslateFromFarField
  (const FarFieldExpansion<TKernel, TKernelDerivative> &se) {

  Vector pos_arrtmp, neg_arrtmp;
  Matrix derivative_map;
  Vector far_center;
  Vector cent_diff;
  Vector far_coeffs;
  int dimension = sea_->get_dimension();
  int far_order = se.get_order();
  int total_num_coeffs = sea_->get_total_num_coeffs(far_order);
  int limit;
  double bandwidth_factor = kd_.BandwidthFactor(se.bandwidth_sq());

  // get center and coefficients for far field expansion
  far_center.Alias(se.get_center());
  far_coeffs.Alias(se.get_coeffs());
  cent_diff.Init(dimension);

  // if the order of the far field expansion is greater than the
  // local one we are adding onto, then increase the order.
  if(far_order > order_) {
    order_ = far_order;
  }

  // compute Gaussian derivative
  limit = 2 * order_ + 1;
  derivative_map.Init(dimension, limit);
  pos_arrtmp.Init(total_num_coeffs);
  neg_arrtmp.Init(total_num_coeffs);

  // compute center difference divided by bw_times_sqrt_two;
  for(index_t j = 0; j < dimension; j++) {
    cent_diff[j] = (center_[j] - far_center[j]) / bandwidth_factor;
  }

  // compute required partial derivatives
  kd_.ComputeDirectionalDerivatives(cent_diff, derivative_map);
  ArrayList<int> beta_plus_alpha;
  beta_plus_alpha.Init(dimension);

  for(index_t j = 0; j < total_num_coeffs; j++) {

    ArrayList<int> beta_mapping = sea_->get_multiindex(j);
    pos_arrtmp[j] = neg_arrtmp[j] = 0;

    for(index_t k = 0; k < total_num_coeffs; k++) {

      ArrayList<int> alpha_mapping = sea_->get_multiindex(k);
      for(index_t d = 0; d < dimension; d++) {
	beta_plus_alpha[d] = beta_mapping[d] + alpha_mapping[d];
      }
      double derivative_factor =
	kd_.ComputePartialDerivative(derivative_map, beta_plus_alpha);
      
      double prod = far_coeffs[k] * derivative_factor;

      if(prod > 0) {
	pos_arrtmp[j] += prod;
      }
      else {
	neg_arrtmp[j] += prod;
      }
    } // end of k-loop
  } // end of j-loop

  Vector C_k_neg = sea_->get_neg_inv_multiindex_factorials();
  for(index_t j = 0; j < total_num_coeffs; j++) {
    coeffs_[j] += (pos_arrtmp[j] + neg_arrtmp[j]) * C_k_neg[j];
  }
}
  
template<typename TKernel, typename TKernelDerivative>
  void LocalExpansion<TKernel, TKernelDerivative>::TranslateToLocal
  (LocalExpansion<TKernel, TKernelDerivative> &se) {

  // get the center and the order and the total number of coefficients of 
  // the expansion we are translating from. Also get coefficients we
  // are translating
  Vector new_center;
  new_center.Alias(se.get_center());
  int prev_order = se.get_order();
  int total_num_coeffs = sea_->get_total_num_coeffs(order_);
  const ArrayList < int > *upper_mapping_index = 
    sea_->get_upper_mapping_index();
  Vector new_coeffs;
  new_coeffs.Alias(se.get_coeffs());

  // dimension
  int dim = sea_->get_dimension();

  // temporary variable
  ArrayList<int> tmp_storage;
  tmp_storage.Init(dim);

  // sqrt two times bandwidth
  double bandwidth_factor = kd_.BandwidthFactor(kernel_.bandwidth_sq());

  // center difference between the old center and the new one
  Vector center_diff;
  center_diff.Init(dim);
  for(index_t d = 0; d < dim; d++) {
    center_diff[d] = (new_center[d] - center_[d]) / bandwidth_factor;
  }

  // set to the new order if the order of the expansion we are translating
  // from is higher
  if(prev_order < order_) {
    se.set_order(order_);
  }

  // inverse multiindex factorials
  Vector C_k;
  C_k.Alias(sea_->get_inv_multiindex_factorials());
  
  // do the actual translation
  for(index_t j = 0; j < total_num_coeffs; j++) {

    ArrayList<int> alpha_mapping = sea_->get_multiindex(j);
    ArrayList <int> upper_mappings_for_alpha = upper_mapping_index[j];
    double pos_coeffs = 0;
    double neg_coeffs = 0;

    for(index_t k = 0; k < upper_mappings_for_alpha.size(); k++) {
      
      if(upper_mappings_for_alpha[k] >= total_num_coeffs) {
	break;
      }

      ArrayList<int> beta_mapping = 
	sea_->get_multiindex(upper_mappings_for_alpha[k]);
      int flag = 0;
      double diff1 = 1.0;

      for(index_t l = 0; l < dim; l++) {
	tmp_storage[l] = beta_mapping[l] - alpha_mapping[l];

	if(tmp_storage[l] < 0) {
	  flag = 1;
	  break;
	}
      } // end of looping over dimension
      
      if(flag)
	continue;

      for(index_t l = 0; l < dim; l++) {
	diff1 *= pow(center_diff[l], tmp_storage[l]);
      }

      double prod =  coeffs_[upper_mappings_for_alpha[k]] * diff1 *
	sea_->get_n_multichoose_k_by_pos(upper_mappings_for_alpha[k], j);
      
      if(prod > 0) {
	pos_coeffs += prod;
      }
      else {
	neg_coeffs += prod;
      }

    } // end of k loop

    new_coeffs[j] += pos_coeffs + neg_coeffs;
  } // end of j loop
}

#endif
