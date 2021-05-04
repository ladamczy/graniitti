// Regge Amplitudes
//
// (c) 2017-2021 Mikael Mieskolainen
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#ifndef MREGGE_H
#define MREGGE_H

// C++
#include <complex>
#include <random>
#include <vector>

// Own
#include "Graniitti/M4Vec.h"
#include "Graniitti/MKinematics.h"


namespace gra {

// Regge amplitude parameters
namespace PARAM_REGGE {

extern bool initialized;

extern std::vector<double> a0;
extern std::vector<double> ap;
extern std::vector<double> sgn;

extern double s0;

extern std::string offshellFF;
extern double      b_EXP;
extern double      a_OREAR;
extern double      b_OREAR;
extern double      b_POW;

extern bool   reggeize;
extern double omega;
extern int    ampcombs;

// Meson/Baryon Couplings and Pomeron, Reggeon, Reggeon exchanges
extern std::vector<double> c;       // coupling
extern std::vector<bool>   active;  // on/off

void PrintParam();
void ReadParameters(int PDG, const std::string &modelfile);

std::complex<double> JPC_CS_coupling(const gra::LORENTZSCALAR &lts,
                                     const gra::PARAM_RES &    resonance);

// Amplitude form factors
double Proton_FF(double tprime, double b);
double Meson_FF(double that, double M2);
double Baryon_FF(double that, double M2);
double ResonanceFormFactor(double shat, double M2, double S0);

// Propagators
double               Meson_prop(double that, double M2);
double               Baryon_prop(double that, double M2);
std::complex<double> FSI_prop(double t_hat, double M2);
}  // namespace PARAM_REGGE

// Matrix element dimension: " GeV^" << -(2*external_legs - 8)
class MRegge {
 public:
  MRegge(gra::LORENTZSCALAR &lts, const std::string &modelfile);
  ~MRegge() {}

  // Regge amplitudes
  std::complex<double> ME8(gra::LORENTZSCALAR &lts);
  std::complex<double> ME6(gra::LORENTZSCALAR &lts);
  std::complex<double> ME4(gra::LORENTZSCALAR &lts, double sign) const;
  std::complex<double> ME2(gra::LORENTZSCALAR &lts, int mode) const;
  std::complex<double> ME3(gra::LORENTZSCALAR &lts, gra::PARAM_RES &resonance) const;
  std::complex<double> ME3ODD(gra::LORENTZSCALAR &lts, gra::PARAM_RES &resonance) const;

  std::complex<double> ME3HEL(gra::LORENTZSCALAR &lts, gra::PARAM_RES &resonance) const;
  std::complex<double> PhotoME3(gra::LORENTZSCALAR &lts, gra::PARAM_RES &resonance) const;

  void PomPomProtonVertex(const gra::LORENTZSCALAR &lts, double &FF_A, double &FF_B) const;

  // Constructor amplitude leg permutations
  void ConstructPerm(int type);

  // Regge propagators
  std::complex<double> PropOnly(double s, double t, int k = 0) const;
  std::complex<double> OdderonProp(double s, double t) const;
  std::complex<double> PhotoProp(double s, double t, double m2, bool excite,
                                 double M2_forward) const;

  // Helicity functions
  double               g_Vertex(double t, double lambda_i, double lambda_f) const;
  std::complex<double> gik_Vertex(double t1, double t2, double dphi, int lambda_h, int J, int P,
                                  int JMAX) const;
  double               gammaLambda(double t1, double t2, double m1, double m2) const;
  int                  xi3(int J, int P, int P_i, int sigma_i, int P_k, int sigma_k) const;

 private:
  // Amplitude permutations, these need to be initialized within calling amplitude function
  std::vector<std::vector<int>> permutations4;
  std::vector<std::vector<int>> permutations6;
};

}  // namespace gra

#endif
