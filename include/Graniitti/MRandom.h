// Random number class
//
// (c) 2017-2021 Mikael Mieskolainen
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#ifndef MRANDOM_H
#define MRANDOM_H

// C++
#include <complex>
#include <random>
#include <vector>

namespace gra {
class MRandom {
 public:
  // Calling constructors of member functions
  MRandom() : flat(0, 1), gaussian(0, 1) {
    rng.seed();  // Default initialization
  }
  ~MRandom() {}

  // Set random number engine seed
  void SetSeed(int seed) {  // Keep it int, so negative can be spotted
    const int SEEDMAX = 2147483647;
    if (seed > SEEDMAX) {
      std::string str = "MRandom::SetSeed: Invalid random seed: " + std::to_string(seed) +
                        " > SEEDMAX = " + std::to_string(SEEDMAX);
      throw std::invalid_argument(str);
    }
    if (seed < 0) {
      std::string str =
          "MRandom::SetSeed: Invalid random seed: " + std::to_string(seed) + " ( < 0)";
      throw std::invalid_argument(str);
    }
    // Seed via seed sequence
    std::seed_seq seedseq({seed});
    rng.seed(seedseq);

    RNDSEED = seed;
  }


  // Return current random seed
  uint32_t GetSeed() const { return RNDSEED; }

  // Random sampling functions
  double U(double a, double b);
  double G(double mu, double sigma);
  double PowerRandom(double a, double b, double alpha);
  double RelativisticBWRandom(double m0, double Gamma, double LIMIT = 5.0, double M_MIN = 0.0);
  double CauchyRandom(double m0, double Gamma, double LIMIT = 5.0, double M_MIN = 0.0);
  int    NBDRandom(double avgN, double k, int maxvalue);
  int    PoissonRandom(double lambda);
  double ExpRandom(double lambda);
  int    LogRandom(double p, int maxvalue);
  void   DirRandom(const std::vector<double> &alpha, std::vector<double> &y);

  double NBDpdf(int n, double avgN, double k);
  double Logpdf(int k, double p);

  // For generators, check: https://nullprogram.com/blog/2017/09/21/

  // 64-bit Mersenne Twister by Matsumoto and Nishimura, fast, basic
  // For some (possible) sources problems, see:
  // [REFERENCE: Harase, https://arxiv.org/abs/1708.06018]
  // std::mt19937_64 rng;

  // 48-bit RANLUX (a bit slower)
  // [REFERENCE: Luscher, https://arxiv.org/abs/hep-lat/9309020]
  std::ranlux48 rng;

  std::ranlux48 get_generator() {
    return rng;
  }  
  
  // Distribution engines
  std::uniform_real_distribution<double> flat;
  std::normal_distribution<double>       gaussian;
  uint32_t                               RNDSEED = 0;  // Random seed set

 private:
  // Nothing
};

}  // namespace gra

#endif
