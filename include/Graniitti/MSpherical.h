// Functional methods for Spherical Harmonic Expansions
//
// (c) 2017-2021 Mikael Mieskolainen
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#ifndef MSPHERICAL_H
#define MSPHERICAL_H

// C++
#include <complex>
#include <iostream>
#include <vector>

// Own
#include "Graniitti/MMath.h"
#include "Graniitti/MMatrix.h"

namespace gra {
namespace spherical {
// Metadata
struct Meta {
  std::string NAME;             // Input name ID
  std::string LEGEND;           // Legend string
  std::string YAXIS;            // Yaxis string
  std::string MODE;             // MC or DATA
  bool        FASTSIM = false;  // Fast simulation on
  std::string FRAME;            // Lorentz frame
  double      SCALE = 1.0;      // Scale/normalization value

  std::vector<std::string> TITLES;  // Phase space titles

  // So we can use this in std::map<>
  bool operator<(const Meta &rhs) const { return NAME < rhs.NAME; }

  void Print() const {
    std::cout << "NAME:     " << NAME << std::endl;
    std::cout << "LEGEND:   " << LEGEND << std::endl;
    std::cout << "YAXIS:    " << YAXIS << std::endl;
    std::cout << "MODE:     " << MODE << std::endl;
    std::cout << "FRAME:    " << FRAME << std::endl;
    std::cout << "FASTSIM:  " << (FASTSIM ? "true" : "false") << std::endl;
    std::cout << "SCALE:    " << SCALE << std::endl;

    std::cout << std::endl;
    for (std::size_t i = 0; i < TITLES.size(); ++i) {
      printf("TITLES[%lu] = %s \n", i, TITLES[i].c_str());
    }
  }
};

// Microevent structure
struct Omega {
  // Decay daughter
  // rest frame variables
  double costheta = 0.0;
  double phi      = 0.0;

  // Invariant / system lab frame variables
  double M  = 0.0;
  double Pt = 0.0;
  double Y  = 0.0;

  bool fiducial = false;
  bool selected = false;
};

// Container
struct Data {
  // Metadata
  spherical::Meta META;

  // Events
  std::vector<spherical::Omega> EVENTS;
};

// Detector data for one hypercell, e.g., in (M,Pt,Y)
struct SH_DET {
  // Moment mixing matrix
  MMatrix<double> MIXlm;

  // Efficiency decomposition coefficients
  std::vector<double> E_lm;
  std::vector<double> E_lm_error;
};

// Data for one hypercell, e.g., in (M,Pt,Y)
struct SH {
  // Directly (algebraic) observed moments
  std::vector<double> t_lm_MPP;
  std::vector<double> t_lm_MPP_error;

  // Extended Maximum Likelihood fitted moments
  std::vector<double> t_lm_EML;
  std::vector<double> t_lm_EML_error;
};

MMatrix<double> GetGMixing(const std::vector<Omega> &events, const std::vector<std::size_t> &ind,
                           int LMAX, const std::string &mode);

std::pair<std::vector<double>, std::vector<double>> GetELM(const std::vector<Omega> &      MC,
                                                           const std::vector<std::size_t> &ind,
                                                           int LMAX, const std::string &mode);

std::vector<double> SphericalMoments(const std::vector<Omega> &      input,
                                     const std::vector<std::size_t> &ind, int LMAX,
                                     const std::string &mode);

MMatrix<double> YLM(const std::vector<Omega> &events, int LMAX);

double HarmDotProd(const std::vector<double> &G, const std::vector<double> &x,
                   const std::vector<bool> &ACTIVE, int LMAX);

void PrintOutMoments(const std::vector<double> &x, const std::vector<double> &x_error,
                     const std::vector<bool> &ACTIVE, int LMAX);

std::vector<std::size_t> GetIndices(const std::vector<Omega> &events, const std::vector<double> &M,
                                    const std::vector<double> &Pt, const std::vector<double> &Y);

void   TestSphericalIntegrals(int LMAX);
int    LinearInd(int l, int m);
double CalcError(double f2, double f, double N);
void   PrintMatrix(FILE *fp, const std::vector<std::vector<double>> &A);

MMatrix<double> Y_real_synthesize(const std::vector<double> &c_lm, const std::vector<bool> &ACTIVE,
                                  std::size_t N, std::vector<double> &costheta,
                                  std::vector<double> &phi, bool normalized = false);

std::vector<double> ErrorProp(const MMatrix<double> &A, const std::vector<double> &x);

}  // namespace spherical
}  // namespace gra

#endif