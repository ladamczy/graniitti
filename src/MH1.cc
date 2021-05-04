// Templated 1D-histogram class with real or complex weights
//
// (c) 2017-2021 Mikael Mieskolainen
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

// C++
#include <algorithm>
#include <complex>
#include <iostream>
#include <valarray>
#include <vector>

// Own
#include "Graniitti/MH1.h"
#include "Graniitti/MMath.h"

// Libraries
#include "rang.hpp"

namespace gra {
// Constructor
template <class T>
MH1<T>::MH1(int xbins, double xmin, double xmax, std::string namestr) {
  name = namestr;
  // Init
  ResetBounds(xbins, xmin, xmax);
  FILLBUFF = false;
}

// Constructor with only number of bins
template <class T>
MH1<T>::MH1(int xbins, std::string namestr) {
  name     = namestr;
  XBINS    = xbins;
  FILLBUFF = true;
}

// Constructor with varying bin sizes [N x 2]
template <class T>
MH1<T>::MH1(std::vector<std::vector<double>> edges, std::string namestr) {
  name = namestr;

  // Init
  ResetBounds(edges);
  FILLBUFF = false;
}

// Empty constructor
template <class T>
MH1<T>::MH1() {
  XBINS    = 50;  // Default
  FILLBUFF = true;
}

// Destructor
template <class T>
MH1<T>::~MH1() {}


// Raw output to a file (or to the screen if file = stdout)
template <class T>
void MH1<T>::RawOutput(FILE *file) const {
  fprintf(file, "#binlow,binhigh,value,error,value/binwidth,error/binwidth \n");

  for (std::size_t idx = 0; idx < static_cast<unsigned int>(XBINS); ++idx) {
    // This will take care of amplitude versus amplitude squared
    const double value     = GetPositiveDefinite(idx);
    const double value_err = GetBinError(idx);

    const double lower = GetBinXVal(idx, -1);
    const double upper = GetBinXVal(idx, 1);
    const double width = upper - lower;

    fprintf(file, "%0.6E,%0.6E,%0.6E,%0.6E,%0.6E,%0.6E\n", lower, upper, value, value_err,
            value / width, value_err / width);
  }
}

// Print screen
template <class T>
void MH1<T>::Print(double width) const {
  if (!(fills > 0)) {  // No fills
    std::cout << "MH1::Print: <" << name << "> Fills = " << fills << std::endl;
    return;
  }

  // Histogram name
  std::cout << "MH1::Print: <" << name << ">" << std::endl;

  const std::size_t N         = XBINS * width;         // Number of ascii columns (PARAMETER)
  double            maxvisual = 1.1 * GetMaxWeight();  // Give some % space to top

  // Top left corner
  std::cout << "                       |";
  for (std::size_t i = 0; i < N; ++i) { std::cout << "="; }
  std::cout << "| " << std::endl;

  for (std::size_t idx = 0; idx < static_cast<unsigned int>(XBINS); ++idx) {
    printf("(%9.2E, %9.2E] |", GetBinXVal(idx, -1), GetBinXVal(idx, 1));


    // This will take care of amplitude versus amplitude squared
    const double value     = GetPositiveDefinite(idx);
    const double value_err = GetBinError(idx);

    // Visualization values scaled between [0,maxw]
    const double w = (maxvisual > 0) ? value / maxvisual : 0;

    // Y-AXIS index
    const std::size_t ind = std::round(w * (N - 1));

    const int Nc = GetBinCount(idx);
    if (Nc > 0) {  // non-zero bin

      // Error on value (only on double histograms)
      const double      relative_err = value_err / value;
      const std::size_t U            = std::ceil(w * (N - 1) * (1 + relative_err));   // up
      const std::size_t D            = std::floor(w * (N - 1) * (1 - relative_err));  // down
      // Print now
      for (std::size_t j = 0; j < N; ++j) {
        if (j == ind) {
          std::cout << rang::bg::magenta << "+" << rang::bg::reset;
        } else if (j >= D && j != ind && j <= U) {
          std::cout << rang::bg::magenta << " " << rang::bg::reset;
        } else {
          std::cout << " ";
        }
      }
      // No events in the bin
    } else {
      for (std::size_t j = 0; j < N; ++j) { std::cout << " "; }
    }

    // Raw value +- error
    // printf("| %0.1E +- %0.1E \n", value, value_err);
    // raw value +- stat | dO/dx (differential cross section, for example)
    printf("| %0.1E +- %0.1E [dO/dx] = %0.1E \n", value, value_err, GetdOdX(idx));
  }

  // Empty bottom left corner
  std::cout << "                       |";

  for (std::size_t i = 0; i < N; ++i) { std::cout << "="; }
  std::cout << "| " << std::endl;

  // Print statistics
  std::cout << "<binned statistics>" << std::endl;
  std::pair<double, double> valerr = WeightMeanAndError();
  printf(" <W> = %0.3E +- %0.3E  [F = %lld | U/O = %lld/%lld]\n", valerr.first, valerr.second,
         fills, underflow, overflow);

  const double mean   = GetMean(1);
  const double sqmean = GetMean(2);
  printf(" <X> = %0.3f, <X^2> = %0.3f, <X^2> - <X>^2 = %0.3f\n", mean, sqmean,
         sqmean - std::pow(mean, 2));

  std::cout << std::endl;
}


template <class T>
std::pair<double, double> MH1<T>::WeightMeanAndError() const {
  const double N    = fills;  // Need to use number of total fills here, not counts in bins
  const double val  = std::abs(SumWeights()) / N;
  const double err2 = std::abs(SumWeights2()) / N - gra::math::pow2(val);
  const double err  = gra::math::msqrt(err2 / N);

  return {val, err};
}

// Get valarrays containing X values and corresponding density values
template <class T>
void MH1<T>::GetXPositiveDefinite(std::valarray<double> &x, std::valarray<double> &y) const {
  x.resize(XBINS);
  y.resize(XBINS);
  for (std::size_t i = 0; i < static_cast<unsigned int>(XBINS); ++i) {
    x[i] = GetBinXVal(i, 0);  // Get center value
    y[i] = GetPositiveDefinite(i);
  }
}

// Histogram mean (based on binned values), possibly with some power applied
template <class T>
double MH1<T>::GetMean(int power) const {
  double sum  = 0.0;
  double norm = 0.0;  // Normalization

  for (std::size_t i = 0; i < static_cast<unsigned int>(XBINS); ++i) {
    double X_value = 0;
    if (binedges.empty()) {
      const double binwidth = (XMAX - XMIN) / XBINS;
      X_value               = binwidth * (i + 1) - binwidth / 2.0 + XMIN;
    } else {
      X_value = (binedges[i][1] - binedges[i][0]) / 2;
    }

    const double weight = GetPositiveDefinite(i);
    sum += weight * std::pow(X_value, power);
    norm += weight;
  }
  if (norm > 0) {
    return sum / norm;
  } else {
    return 0.0;
  }
}

template <class T>
bool MH1<T>::ValidBin(int idx) const {
  if (idx >= 0 && idx < XBINS) { return true; }
  return false;
}

// Unweighted fill
template <class T>
void MH1<T>::Fill(double xvalue) {
  // Call the weighted with weight 1.0
  Fill(xvalue, 1.0);
}

// Weighted fill
template <class T>
void MH1<T>::Fill(double xvalue, T weight) {
  if (!FILLBUFF) {  // Normal filling

    fills += 1;

    // Find out bin
    int idx = 0;
    GetBinIdx(xvalue, idx);

    if (idx == -3) { nanflow += 1; }
    if (idx == -1) { underflow += 1; }
    if (idx == -2) { overflow += 1; }

    if (ValidBin(idx)) {
      weights[idx] += weight;
      weights2[idx] += std::abs(std::conj(weight) * weight);
      counts[idx] += 1;
    }
  } else {  // Autorange initialization
    buff_values.push_back(xvalue);
    buff_weights.push_back(weight);

    if (buff_values.size() > static_cast<unsigned int>(AUTOBUFFSIZE)) { FlushBuffer(); }
  }
}

// Automatic histogram range algorithm
template <class T>
void MH1<T>::FlushBuffer() {
  if (FILLBUFF && buff_values.size() > 0) {
    FILLBUFF = false;  // no more filling buffer

    // Find out mean
    double mu   = 0;
    double sumW = 0;
    for (std::size_t i = 0; i < buff_values.size(); ++i) {
      mu += std::abs(buff_values[i] * buff_weights[i]);
      sumW += std::abs(buff_weights[i]);
    }
    if (sumW > 0) { mu /= sumW; }

    // Variance
    double var = 0;
    for (std::size_t i = 0; i < buff_values.size(); ++i) {
      var += std::abs(buff_weights[i]) * std::pow(buff_values[i] - mu, 2);
    }
    if (sumW > 0) { var /= sumW; }

    // Minimum and maximum
    auto         it1    = std::min_element(buff_values.begin(), buff_values.end());
    auto         it2    = std::max_element(buff_values.begin(), buff_values.end());
    const double minval = *it1;
    const double maxval = *it2;

    // Set new histogram bounds
    const double std = std::sqrt(std::abs(var));

    double xmin = mu - 2.5 * std;
    double xmax = mu + 2.5 * std;

    // A numerical failure may happen with variance calculation, then use this
    if (std::isnan(xmin) || std::isnan(xmax)) {
      xmin = minval;
      xmax = maxval;
    }

    // If symmetric setup set by user
    if (AUTOSYMMETRY) {
      double val = (std::abs(xmin) + std::abs(xmax)) / 2.0;
      xmin       = -val;
      xmax       = val;
    }

    // We have only positive values, such as invariant mass
    if (minval > 0.0) { xmin = std::max(0.0, xmin); }

    ResetBounds(XBINS, xmin, xmax);

    // Fill buffered events
    for (std::size_t i = 0; i < buff_values.size(); ++i) { Fill(buff_values[i], buff_weights[i]); }

    // Clear buffers
    buff_values.clear();
    buff_weights.clear();
  }
}

// Reset histogram completely
template <class T>
void MH1<T>::ResetBounds(int xbins) {
  ResetBounds(xbins, 0.0, 0.0);
  FILLBUFF = true;  // Autorange on, no explicit bounds
}

// Reset histogram completely
template <class T>
void MH1<T>::ResetBounds(std::vector<std::vector<double>> edges) {
  binedges = edges;

  if (binedges.empty()) {
    throw std::invalid_argument("MH1<T>::ResetBounds: input edge array is empty " +
                                std::to_string(binedges.size()));
  }
  for (std::size_t i = 0; i < binedges.size(); ++i) {
    if (binedges[i].size() != 2) {
      throw std::invalid_argument(
          "MH1<T>::ResetBounds: every element of the edge array should size 2 (minimum, maximum)");
    }
    if (binedges[i][1] <= binedges[i][0]) {
      throw std::invalid_argument("MH1<T>::ResetBounds: non-monotonic binedges at index = " +
                                  std::to_string(i));
    }
  }

  ResetBounds(binedges.size(), binedges[0][0], binedges[binedges.size() - 1][1]);
}

// Reset histogram completely
template <class T>
void MH1<T>::ResetBounds(int xbins, double xmin, double xmax) {
  XMIN  = xmin;
  XMAX  = xmax;
  XBINS = xbins;

  // Init
  std::vector<T> null(XBINS, 0.0);
  weights  = null;
  weights2 = null;
  counts   = std::vector<long long int>(XBINS, 0);

  Clear();           // Call also this!
  FILLBUFF = false;  // No autorange, explicit bounds provided
}

// Clear the histogram data but keep the bounds
template <class T>
void MH1<T>::Clear() {
  for (std::size_t i = 0; i < static_cast<unsigned int>(XBINS); ++i) {
    weights[i]  = 0.0;
    weights2[i] = 0.0;
    counts[i]   = 0;
  }
  fills     = 0;
  underflow = 0;
  overflow  = 0;
  nanflow   = 0;
}

// Sum over all histogram bin weights
template <class T>
T MH1<T>::SumWeights() const {
  T sum = 0.0;
  for (std::size_t i = 0; i < static_cast<unsigned int>(XBINS); ++i) { sum += weights[i]; }
  return sum;
}

// Sum over all histogram bin weights squared
template <class T>
T MH1<T>::SumWeights2() const {
  T sum = 0.0;
  for (std::size_t i = 0; i < static_cast<unsigned int>(XBINS); ++i) { sum += weights2[i]; }
  return sum;
}

// Sum the number of bin counts (not the same as fills)
template <class T>
long long int MH1<T>::SumBinCounts() const {
  long long int sum = 0;
  for (std::size_t i = 0; i < static_cast<unsigned int>(XBINS); ++i) { sum += counts[i]; }
  return sum;
}

// Get maximum histogram bin weight, for complex return |w|^2
template <class T>
double MH1<T>::GetMaxWeight() const {
  double maxval = 0;
  for (std::size_t i = 0; i < static_cast<unsigned int>(XBINS); ++i) {
    if (GetPositiveDefinite(i) > maxval) { maxval = GetPositiveDefinite(i); }
  }
  return maxval;
}

// Get minimum histogram bin weight, for complex return |w|^2
template <class T>
double MH1<T>::GetMinWeight() const {
  double minval = 1e128;
  for (std::size_t i = 0; i < static_cast<unsigned int>(XBINS); ++i) {
    if (GetPositiveDefinite(i) < minval) { minval = GetPositiveDefinite(i); }
  }
  return minval;
}

// Get number of event fills in the bin
template <class T>
long long int MH1<T>::GetBinCount(int idx) const {
  if (ValidBin(idx)) {
    return counts[idx];
  } else {
    return 0;
  }
}

// Get weight of the bin, for complex return complex number
template <class T>
T MH1<T>::GetBinWeight(int idx) const {
  if (ValidBin(idx)) {
    return weights[idx];
  } else {
    return 0.0;
  }
}

// Get weight of the bin, for complex return complex number
template <class T>
T MH1<T>::GetBinWeight2(int idx) const {
  if (ValidBin(idx)) {
    return weights2[idx];
  } else {
    return 0.0;
  }
}

// Error estimate on bin
template <class T>
double MH1<T>::GetBinError(int idx) const {
  if (!ValidBin(idx)) { return 0.0; }
  double err = 0;
  if (GetBinCount(idx) > 0) {
    if (IsReal()) {
      err = std::sqrt(std::abs(GetBinWeight2(idx)));  // [\sum_i w_i^2)]^{1/2}
    } else {
      err = std::abs(GetBinWeight2(idx));  // [\sum_i w_i^2)]^{1/2}
    }
  }
  return err;
}

// Return weight (double) or |w|^2 (complex case) as a differential distribution
template <class T>
double MH1<T>::GetdOdX(int idx) const {
  const double norm = fills * GetBinWidth(idx);

  if (ValidBin(idx) && norm > 0.0) {
    return GetPositiveDefinite(idx) / norm;
  } else {
    return 0.0;
  }
}

// Return uncertainty as a differential distribution
template <class T>
double MH1<T>::GetdOdXError(int idx) const {
  const double norm = fills * GetBinWidth(idx);

  if (ValidBin(idx) && norm > 0.0) {
    return GetBinError(idx) / norm;
  } else {
    return 0.0;
  }
}

// Return bin normalization
template <class T>
double MH1<T>::GetBinWidth(int idx) const {
  if (binedges.empty()) {
    return (XMAX - XMIN) / XBINS;
  } else {
    return binedges[idx][1] - binedges[idx][0];
  }
}

// Return weight (double) or |w|^2 (complex case)
template <class T>
double MH1<T>::GetPositiveDefinite(int idx) const {
  if (ValidBin(idx)) {
    if (IsReal()) { return std::abs(weights[idx]); }
    return std::pow(std::abs(weights[idx]), 2);
  } else {
    return 0.0;
  }
}


// Get bin index (idx) corresponding to value (xvalue)
template <class T>
void MH1<T>::GetBinIdx(double xvalue, int &idx) {
  // Equal width binning
  if (binedges.empty()) {
    idx = ComputeIdx(xvalue, XMIN, XMAX, XBINS, LOGX);

    // Variable width binning
  } else {
    idx = ComputeIdx(xvalue, binedges);
  }
}

// Get bin value in units of X for a given bin index
// boundary = -1,0,1 (lower, center, upper)
template <class T>
double MH1<T>::GetBinXVal(int idx, int boundary) const {
  // Non-equal width binning
  if (!binedges.empty()) {
    if (boundary == -1) return binedges[idx][0];
    if (boundary == 0) return (binedges[idx][1] - binedges[idx][0]) / 2;
    if (boundary == 1) return binedges[idx][1];
  }

  // Equal binning
  if (idx > XBINS - 1) {
    throw std::invalid_argument("MH1::GetBinXVal: idx = " + std::to_string(idx) +
                                " > XBINS-1 = " + std::to_string(XBINS - 1));
  }
  if (LOGX) {
    const double log10step = (std::log10(XMAX) - std::log10(XMIN)) / XBINS;
    if (boundary == -1) {
      return std::pow(10, std::log10(XMIN) + idx * log10step);
    } else if (boundary == 0) {
      return (std::pow(10, std::log10(XMIN) + idx * log10step) +
              std::pow(10, std::log10(XMIN) + (idx + 1) * log10step)) /
             2;
    } else if (boundary == 1) {
      return std::pow(10, std::log10(XMIN) + (idx + 1) * log10step);
    } else {
      throw std::invalid_argument("MH1::GetBinXVal: Bin boundary not valid (-1,0,1)");
    }
  } else {
    const double binwidth = (XMAX - XMIN) / XBINS;
    const double value    = XMIN + (idx + 1) * binwidth;
    if (boundary == -1) {
      return value - binwidth;
    } else if (boundary == 0) {
      return value - binwidth / 2.0;
    } else if (boundary == 1) {
      return value;
    } else {
      throw std::invalid_argument("MH1::GetBinXVal: Bin boundary not valid (-1,0,1)");
    }
  }
}


// Compute bin index for variable width binning
//
template <class T>
int MH1<T>::ComputeIdx(double value, const std::vector<std::vector<double>> &edges) const {
  if (std::isnan(value) || std::isinf(value)) { return -3; }

  // Underflow
  if (value < edges[0][0]) {
    return -1;
  }
  // Overflow
  else if (value > edges[edges.size() - 1][1]) {
    return -2;
  }
  // Binary search
  else {
    /*
    // Linear search
    for (std::size_t i = 0; i < binedges.size(); ++i) {
      if (binedges[i][0] < xvalue && xvalue <= binedges[i][1]) {
        idx = i;
      }
    }
    */
    // Binary search via STL library
    const auto it = std::upper_bound(edges.begin(), edges.end(), value, compare_bin_edges());
    return (it != edges.end()) ? it - edges.begin() : -3;  // -3 happens only if hole
  }
}


// Compute table/histogram index for linearly or base-10 logarithmically spaced
// bins. Computes exact uniform filling within bin boundaries.
//
// In the logarithmic case, MINVAL and MAXVAL > 0, naturally.
//
//
// Underflow     returns -1
// Overflow      returns -2
// Invalid input returns -3
//
template <class T>
int MH1<T>::ComputeIdx(double value, double minval, double maxval, int nbins, bool logbins) const {
  if (std::isnan(value) || std::isinf(value)) { return -3; }
  if (value < minval) { return -1; }  // underflow
  if (value > maxval) { return -2; }  // overflow

  int idx = 0;
  // Logarithmic binning
  if (logbins) {
    // Check do we have non-negative input
    idx = value > 0 ? std::floor(nbins * (std::log10(value) - std::log10(minval)) /
                                 (std::log10(maxval) - std::log10(minval)))
                    : -1;
    // Linear binning
  } else {
    const double BINWIDTH = (maxval - minval) / nbins;
    idx                   = std::floor((value - minval) / BINWIDTH);
  }
  return idx;
}

// Instantiate (necessary for compilation)
template class MH1<double>;
template class MH1<std::complex<double>>;

}  // namespace gra
