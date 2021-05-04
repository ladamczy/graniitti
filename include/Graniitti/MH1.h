// Templated 1D-histogram class with real or complex weights
//
// (c) 2017-2021 Mikael Mieskolainen
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#ifndef MH1_H
#define MH1_H

// C++
#include <complex>
#include <vector>

#include "json.hpp"

using json = nlohmann::json;

namespace std {
// Handle std::complex
template <class T>
void to_json(json &j, const std::complex<T> &p) {
  j = json{p.real(), p.imag()};
}

template <class T>
void from_json(const json &j, std::complex<T> &p) {
  p.real(j.at(0));
  p.imag(j.at(1));
}
}  // namespace std

namespace gra {

struct compare_bin_edges {
  // Find the right bin given a vector {lower,upper}. Use with std::upper_bound
  //
  bool operator()(const std::vector<double> &bin, const double &value) { return (bin[0] < value); }
  bool operator()(const double &value, const std::vector<double> &bin) { return (value <= bin[1]); }
};

template <class T>
class MH1 {
 public:
  MH1(int xbins, double xmin, double xmax, std::string namestr = "noname");
  MH1(int xbins, std::string namestr = "noname");
  MH1(std::vector<std::vector<double>> edges, std::string namestr = "noname");

  MH1();
  ~MH1();

  void Fill(double xvalue);
  void Fill(double xvalue, T weight);
  void Clear();

  void VectorOutput(std::vector<std::vector<double>> &bins,
                    std::vector<std::vector<double>> &vals) const;
  void RawOutput(FILE *file = stdout) const;

  long long int FillCount() const { return fills; }
  long long int SumBinCounts() const;
  double        GetMean(int power) const;

  std::pair<double, double> WeightMeanAndError() const;

  // Probability density
  std::vector<double> GetProbDensity() const {
    double sum = 0;
    for (std::size_t i = 0; i < weights.size(); ++i) { sum += GetPositiveDefinite(i); }
    std::vector<double> p(weights.size(), 0.0);
    if (sum > 0) {
      for (std::size_t i = 0; i < weights.size(); ++i) { p[i] = GetPositiveDefinite(i) / sum; }
    }
    return p;
  }

  // Get full histogram data
  std::vector<T>             GetWeights() const { return weights; }
  std::vector<T>             GetWeights2() const { return weights2; }
  std::vector<long long int> GetCounts() const { return counts; }

  void GetXPositiveDefinite(std::valarray<double> &x, std::valarray<double> &y) const;

  T SumWeights() const;
  T SumWeights2() const;

  T      GetBinWeight(int idx) const;
  T      GetBinWeight2(int idx) const;
  double GetBinError(int idx) const;

  double GetMaxWeight() const;
  double GetMinWeight() const;

  long long int GetBinCount(int idx) const;
  double        GetPositiveDefinite(int idx) const;
  double        GetdOdX(int idx) const;
  double        GetdOdXError(int idx) const;

  double GetBinXVal(int idx, int boundary = 0) const;
  void   GetBinIdx(double xvalue, int &idx);
  void   Print(double width = 1.25) const;  // default argument

  void ResetBounds(int xbins);
  void ResetBounds(int xbins, double xmin, double xmax);
  void ResetBounds(std::vector<std::vector<double>> edges);

  // Set logarithmic binning
  void SetLogX() {
    if (XMIN < 1e-9) {
      throw std::invalid_argument(
          "MH1::SetLogX: Error: Minimum boundary XMIN = " + std::to_string(XMIN) + " < 0");
    }
    LOGX = true;
  }

  // Number of bins
  std::size_t XBins() const { return XBINS; }

  // Copy constructor (DEFAULT is fine)
  // MH1<T> MH1(const MH1<T>& obj) {}

  // Overload + operator to add two histograms
  MH1<T> operator+(const MH1<T> &rhs) {
    if (this->XBINS != rhs.XBINS) {
      throw std::domain_error("MH1<T> + operator: Histograms with different number of bins");
    }

    MH1<T> h(this->XBINS, this->XMIN, this->XMAX, this->name);
    h.binedges = this->binedges;

    h.fills     = this->fills + rhs.fills;
    h.underflow = this->underflow + rhs.underflow;
    h.overflow  = this->overflow + rhs.overflow;
    h.nanflow   = this->nanflow + rhs.nanflow;

    // DATA
    h.weights  = this->weights;
    h.weights2 = this->weights2;
    h.counts   = this->counts;
    for (std::size_t i = 0; i < h.weights.size(); ++i) {
      h.weights[i] += rhs.weights[i];
      h.weights2[i] += rhs.weights2[i];
      h.counts[i] += rhs.counts[i];
    }
    return h;
  }

  // Overload - operator to subtract two histograms
  MH1<T> operator-(const MH1<T> &rhs) {
    if (this->XBINS != rhs.XBINS) {
      throw std::domain_error("MH1<T> - operator: Histograms with different number of bins");
    }

    MH1<T> h(this->XBINS, this->XMIN, this->XMAX, this->name);
    h.binedges = this->binedges;

    h.fills     = this->fills - rhs.fills;
    h.underflow = this->underflow - rhs.underflow;
    h.overflow  = this->overflow - rhs.overflow;
    h.nanflow   = this->nanflow - rhs.nanflow;

    // DATA
    h.weights  = this->weights;
    h.weights2 = this->weights2;
    h.counts   = this->counts;
    for (std::size_t i = 0; i < h.weights.size(); ++i) {
      h.weights[i] -= rhs.weights[i];
      h.weights2[i] -= rhs.weights2[i];
      h.counts[i] -= rhs.counts[i];
    }
    return h;
  }

  // Overload * operator to multiply two histograms
  MH1<T> operator*(const MH1<T> &rhs) {
    if (this->XBINS != rhs.XBINS) {
      throw std::domain_error("MH1<T> * operator: Histograms with different number of bins");
    }

    MH1<T> h(this->XBINS, this->XMIN, this->XMAX, this->name);
    h.binedges = this->binedges;

    h.fills     = this->fills;
    h.underflow = this->underflow;
    h.overflow  = this->overflow;
    h.nanflow   = this->nanflow;

    // DATA
    h.weights  = this->weights;
    h.weights2 = this->weights2;
    h.counts   = this->counts;
    for (std::size_t i = 0; i < h.weights.size(); ++i) {
      h.weights[i] *= rhs.weights[i];
      h.weights2[i] *= rhs.weights2[i];
      h.counts[i] *= rhs.counts[i];
    }
    return h;
  }

  // Overload / operator to divide two histograms
  MH1<T> operator/(const MH1<T> &rhs) {
    if (this->XBINS != rhs.XBINS) {
      throw std::domain_error("MH1<T> / operator: Histograms with different number of bins");
    }

    MH1<T> h(this->XBINS, this->XMIN, this->XMAX, this->name);
    h.binedges = this->binedges;

    h.fills     = this->fills;
    h.underflow = this->underflow;
    h.overflow  = this->overflow;
    h.nanflow   = this->nanflow;

    // DATA
    h.weights  = this->weights;
    h.weights2 = this->weights2;
    h.counts   = this->counts;
    for (std::size_t i = 0; i < h.weights.size(); ++i) {
      h.weights[i]  = (std::abs(rhs.weights[i]) > 0) ? h.weights[i] / rhs.weights[i] : 0;
      h.weights2[i] = (std::abs(rhs.weights2[i]) > 0) ? h.weights2[i] / rhs.weights2[i] : 0;
      h.counts[i]   = (rhs.counts[i] > 0) ? h.counts[i] / rhs.counts[i] : 0;
    }
    return h;
  }

  // AUTOBUFFSIZE for autorange buffer size
  void SetAutoBuffSize(int n) { AUTOBUFFSIZE = n; }
  void FlushBuffer();

  // Symmetric bounds
  void SetAutoSymmetry(bool in) { AUTOSYMMETRY = in; }

  // Get histogram bounds
  void GetBounds(int &xbins, double &xmin, double &xmax) const {
    if (binedges.empty()) {
      xbins = XBINS;
      xmin  = XMIN;
      xmax  = XMAX;
    } else {
      xbins = binedges.size();
      xmin  = binedges[0][0];
      xmax  = binedges[binedges.size() - 1][1];
    }
  }

  void FuseBuffer(const MH1<T> &rhs) {
    buff_values.insert(buff_values.end(), rhs.buff_values.begin(), rhs.buff_values.end());
    buff_weights.insert(buff_weights.end(), rhs.buff_weights.begin(), rhs.buff_weights.end());
  }

  // Keep it public for buffer fusion
  std::vector<double> buff_values;
  std::vector<T>      buff_weights;

  // Json output
  void struct2json(json &j) const {
    // Construct bins explicitly
    std::vector<std::vector<double>> bins;
    for (std::size_t idx = 0; idx < static_cast<unsigned int>(XBINS); ++idx) {
      bins.push_back(std::vector<double>{GetBinXVal(idx, -1), GetBinXVal(idx, 1)});
    }

    j["name"]     = name;
    j["binedges"] = bins;

    j["fills"]     = fills;
    j["overflow"]  = overflow;
    j["underflow"] = underflow;
    j["nanflow"]   = nanflow;

    json jw(weights);
    json jw2(weights2);

    j["weights"]  = jw;
    j["weights2"] = jw2;
    j["counts"]   = counts;
  }


 private:
  std::string name;  // Histogram name

  // -----------------------------------------------------------
  // For autorange

  bool FILLBUFF     = false;
  int  AUTOBUFFSIZE = 100000;  // Default AUTOBUFFSIZE
  bool AUTOSYMMETRY = false;
  // -----------------------------------------------------------

  // Used for non-uniform width binning, otherwise can be empty
  std::vector<std::vector<double>> binedges;

  // Boundary conditions
  double XMIN  = 0.0;
  double XMAX  = 0.0;
  int    XBINS = 0;

  // Number of fills, overflow and underflow counts
  long long int fills     = 0;
  long long int overflow  = 0;
  long long int underflow = 0;
  long long int nanflow   = 0;

  // weights (in unweighted case weights = counts)
  std::vector<T>             weights;
  std::vector<T>             weights2;
  std::vector<long long int> counts;  // counts

  // Logarithmic binning
  bool LOGX = false;

  double GetBinWidth(int idx) const;
  bool   ValidBin(int idx) const;
  int    ComputeIdx(double value, double minval, double maxval, int nbins, bool logbins) const;
  int    ComputeIdx(double value, const std::vector<std::vector<double>> &edges) const;

  // Comparing do we have double or std::complex<double>
  constexpr bool IsReal() const { return std::is_same<T, double>::value; }
  constexpr bool IsComplex() const { return std::is_same<T, std::complex<double>>::value; }
};

// Include the implementation
//#include "MH1.tcc"

}  // namespace gra

#endif