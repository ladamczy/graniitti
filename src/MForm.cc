// Form factors, structure functions, Regge trajectories etc. parametrizations
//
// (c) 2017-2021 Mikael Mieskolainen
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

// C++
#include <complex>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

// Own
#include "Graniitti/MForm.h"
#include "Graniitti/MGraniitti.h"
#include "Graniitti/MKinematics.h"
#include "Graniitti/MMath.h"
#include "Graniitti/MProcess.h"
#include "Graniitti/MQED.h"
#include "Graniitti/MSpin.h"

// Libraries
#include "json.hpp"
#include "rang.hpp"


using gra::math::abs2;
using gra::math::msqrt;
using gra::math::PI;
using gra::math::pow2;
using gra::math::pow3;
using gra::math::zi;

using gra::PDG::mp;
using gra::PDG::mpi;

namespace gra {

// Structure functions and nuclear form factors
namespace PARAM_STRUCTURE {

std::string F2        = "CKMT";
std::string EM        = "DIPOLE";
std::string QED_alpha = "ZERO";

bool initialized = false;
// Read parameters from file
void ReadParameters(const std::string &modelfile) {
  try {
    const std::string data = gra::aux::GetInputData(modelfile);
    nlohmann::json    j    = nlohmann::json::parse(data);

    const std::string XID = "PARAM_STRUCTURE";

    PARAM_STRUCTURE::F2        = j.at(XID).at("F2");
    PARAM_STRUCTURE::EM        = j.at(XID).at("EM");
    PARAM_STRUCTURE::QED_alpha = j.at(XID).at("QED_alpha");

    initialized = true;

  } catch (...) {
    std::string str = "PARAM_STRUCTURE::ReadParameters: Error parsing " + modelfile +
                      " (Check for extra/missing commas)";
    throw std::invalid_argument(str);
  }
}

}  // namespace PARAM_STRUCTURE

// Model parameters
namespace PARAM_SOFT {
// Pomeron trajectory
double DELTA_P = 0.0;
double ALPHA_P = 0.0;

// Couplings
double gN_P  = 0.0;
double gN_O  = 0.0;
double g3P   = 0.0;
double gamma = 0.0;

// Proton form factor
double fc1 = 0.0;
double fc2 = 0.0;

// Pion loop
double fc3 = 0.0;

// On/Off
bool ODDERON_ON = false;

bool initialized = false;

std::string GetHashString() {
  std::string str = std::to_string(PARAM_SOFT::DELTA_P) + std::to_string(PARAM_SOFT::ALPHA_P) +
                    std::to_string(PARAM_SOFT::gN_P) + std::to_string(PARAM_SOFT::gN_O) +
                    std::to_string(PARAM_SOFT::fc1) + std::to_string(PARAM_SOFT::fc2) +
                    std::to_string(PARAM_SOFT::fc3) + std::to_string(PARAM_SOFT::ODDERON_ON);
  return str;
}

void PrintParam() {
  printf("PARAM_SOFT:: Soft model parameters: \n\n");
  printf("- DELTA_P    = %0.5f \n", DELTA_P);
  printf("- ALPHA_P    = %0.5f [GeV^{-2}] \n", ALPHA_P);
  printf("- gN_P       = %0.5f [GeV^{-1}] \n", gN_P);
  printf("- gN_O       = %0.5f [GeV^{-1}] \n", gN_O);
  printf("- g3P        = %0.5f \n", g3P / gN_P);  // Convention
  printf("- gamma      = %0.5f \n", gamma);
  printf("- fc1        = %0.5f [GeV^2] \n", fc1);
  printf("- fc2        = %0.5f [GeV^2] \n", fc2);
  printf("- fc3        = %0.5f [GeV^2] \n", fc3);
  printf("- ODDERON_ON = ");
  std::cout << (ODDERON_ON ? "true" : "false") << std::endl;
  std::cout << std::endl << std::endl;
}

// Read parameters from file
void ReadParameters(const std::string &modelfile) {
  try {
    const std::string data = gra::aux::GetInputData(modelfile);
    nlohmann::json    j    = nlohmann::json::parse(data);

    const std::string XID = "PARAM_SOFT";

    PARAM_SOFT::DELTA_P = j.at(XID).at("DELTA_P");
    PARAM_SOFT::ALPHA_P = j.at(XID).at("ALPHA_P");
    PARAM_SOFT::gN_P    = j.at(XID).at("gN_P");
    PARAM_SOFT::gN_O    = j.at(XID).at("gN_O");

    double triple3P   = j.at(XID).at("g3P");
    PARAM_SOFT::g3P   = triple3P * PARAM_SOFT::gN_P;  // Convention
    PARAM_SOFT::gamma = j.at(XID).at("gamma");

    PARAM_SOFT::fc1 = j.at(XID).at("fc1");
    PARAM_SOFT::fc2 = j.at(XID).at("fc2");
    PARAM_SOFT::fc3 = j.at(XID).at("fc3");

    PARAM_SOFT::ODDERON_ON = j.at(XID).at("ODDERON_ON");

    initialized = true;
    PARAM_SOFT::PrintParam();

  } catch (...) {
    std::string str = "PARAM_SOFT::ReadParameters: Error parsing " + modelfile +
                      " (Check for extra/missing commas)";
    throw std::invalid_argument(str);
  }
}

}  // namespace PARAM_SOFT

// Flat amplitude parameters
namespace PARAM_FLAT {
int    active = 0;
double b      = 0.0;

bool initialized = false;

// Read parameters from file
void ReadParameters(const std::string &modelfile) {
  try {
    const std::string data = gra::aux::GetInputData(modelfile);
    nlohmann::json    j    = nlohmann::json::parse(data);

    const std::string XID = "PARAM_FLAT";

    PARAM_FLAT::b = j.at(XID).at("b");

    initialized = true;
  } catch (...) {
    std::string str = "PARAM_FLAT::ReadParameters: Error parsing " + modelfile +
                      " (Check for extra/missing commas)";
    throw std::invalid_argument(str);
  }
}
}  // namespace PARAM_FLAT

// Forward proton excitation
namespace PARAM_NSTAR {
std::string         fragment = "none";
std::vector<double> rc       = {0.0, 0.0, 0.0};

bool initialized = false;

// Read parameters from file
void ReadParameters(const std::string &modelfile) {
  try {
    const std::string data = gra::aux::GetInputData(modelfile);
    nlohmann::json    j    = nlohmann::json::parse(data);

    const std::string XID = "PARAM_NSTAR";

    PARAM_NSTAR::fragment  = j.at(XID).at("fragment");
    std::vector<double> rc = j.at(XID).at("rc");
    PARAM_NSTAR::rc        = rc;

    // Make sure they sum to one
    const double sum_rc = std::accumulate(rc.begin(), rc.end(), 0);
    for (std::size_t i = 0; i < PARAM_NSTAR::rc.size(); ++i) { PARAM_NSTAR::rc[i] /= sum_rc; }

    initialized = true;
  } catch (...) {
    std::string str = "PARAM_NSTAR::ReadParameters: Error parsing " + modelfile +
                      " (Check for extra/missing commas)";
    throw std::invalid_argument(str);
  }
}
}  // namespace PARAM_NSTAR

namespace form {

// Read resonance parameters
gra::PARAM_RES ReadResonance(const std::string &resparam_str, MRandom &rng) {

  // =====================================================================
  // Find global resonance parameters

  bool        SPINGEN = false;
  bool        SPINDEC = false;
  std::string FRAME   = "null";
  int         JMAX    = 0;

  {
    // Read and parse
    const std::string fullpath =
        gra::aux::GetBasePath(2) + "/modeldata/" + gra::MODELPARAM + "/GENERAL.json";

    const std::string data = gra::aux::GetInputData(fullpath);
    nlohmann::json    j;

    try {
      j = nlohmann::json::parse(data);
    } catch (...) {
      std::string str =
          "form::ReadResonance: Error parsing " + fullpath + " (Check for extra/missing commas)";
      throw std::invalid_argument(str);
    }

    SPINGEN = j.at("PARAM_SPIN").at("SPINGEN");
    SPINDEC = j.at("PARAM_SPIN").at("SPINDEC");
    FRAME   = j.at("PARAM_SPIN").at("FRAME");
    JMAX    = j.at("PARAM_SPIN").at("JMAX");

    std::cout << "gra::form::ReadResonance: General parameters:" << std::endl;
    std::cout << std::endl;
    std::cout << "- SPINGEN = " << std::boolalpha << SPINGEN << std::noboolalpha <<
      "\t(generation 2 -> 1 spin correlations active)" << std::endl;
    std::cout << "- SPINDEC = " << std::boolalpha << SPINDEC << std::noboolalpha <<
      "\t(decay      1 -> 2 spin correlations active)" << std::endl;
    std::cout << "- FRAME   = " << FRAME   <<
      "\t(central system polarization frame)"          << std::endl;
    std::cout << "- JMAX    = " << JMAX    <<
      "\t(maximum ang. momentum of sliding Pomeron)"   << std::endl;
    std::cout << std::endl;
  }

  // =====================================================================

  std::cout << "gra::form::ReadResonance: Reading " + resparam_str + " ";

  // Create a JSON object from file
  std::string inputfile =
      gra::aux::GetBasePath(2) + "/modeldata/" + gra::MODELPARAM + "/" + resparam_str;

  // Read and parse
  std::string    data;
  nlohmann::json j;

  try {
    data = gra::aux::GetInputData(inputfile);
    j    = nlohmann::json::parse(data);
  } catch (...) {
    throw std::invalid_argument("form::ReadResonance: Error parsing '" + resparam_str + "'");
  }

  // Resonance parameters
  gra::PARAM_RES res;

  try {
    // -------------------------------------------------------------------
    // Collect global variables
    res.SPINGEN = SPINGEN;
    res.SPINDEC = SPINDEC;
    res.FRAME   = FRAME;
    res.JMAX    = JMAX;
    // -------------------------------------------------------------------

    // Complex coupling
    double g_A   = j.at("PARAM_RES").at("g_A");
    double g_phi = j.at("PARAM_RES").at("g_phi");
    res.g        = g_A * std::exp(std::complex<double>(0, 1) * g_phi);

    // Form factor parameter
    res.g_FF = j.at("PARAM_RES").at("g_FF");

    // PDG code
    res.p.pdg = j.at("PARAM_RES").at("PDG");

    // Mass
    res.p.mass = j.at("PARAM_RES").at("M");
    if (res.p.mass < 0) {
      std::string str = "MAux:ReadResonance:: <" + resparam_str + "> Invalid M < 0 !";
      throw std::invalid_argument(str);
    }

    // Width
    res.p.width = j.at("PARAM_RES").at("W");
    if (res.p.width < 0) {
      std::string str = "MAux:ReadResonance:: <" + resparam_str + "> Invalid W < 0 !";
      throw std::invalid_argument(str);
    }

    // Spin
    const double J = j.at("PARAM_RES").at("J");
    res.p.spinX2   = J * 2;
    if (res.p.spinX2 < 0) {
      std::string str = "MAux:ReadResonance:: <" + resparam_str + "> Invalid J < 0 !";
      throw std::invalid_argument(str);
    }

    // Parity
    res.p.P = j.at("PARAM_RES").at("P");
    if (!(res.p.P == -1 || res.p.P == 1)) {
      std::string str = "MAux:ReadResonance:: <" + resparam_str + "> Invalid P (not -1 or 1) !";
      throw std::invalid_argument(str);
    }

    // ------------------------------------------------------------------
    // Tensor Pomeron couplings vector
    std::vector<double> g_Tensor = j.at("PARAM_RES").at("g_Tensor");
    res.g_Tensor                 = g_Tensor;

    if (J == 0 && res.g_Tensor.size() != 2) {
      throw std::invalid_argument("MAux::ReadResonance:: <" + resparam_str +
                                  "> Tensor Pomeron coupling array should be of size 2 for J = 0");
    }
    if (J == 1 && res.g_Tensor.size() != 2) {
      throw std::invalid_argument("MAux::ReadResonance:: <" + resparam_str +
                                  "> Tensor Pomeron coupling array should be of size 2 for J = 1");
    }
    if (J == 2 && res.g_Tensor.size() != 7) {
      throw std::invalid_argument("MAux::ReadResonance:: <" + resparam_str +
                                  "> Tensor Pomeron coupling array should be of size 7 for J = 2");
    }
    // ------------------------------------------------------------------

    // Validity of these is taken care of in the functions
    res.BW = j.at("PARAM_RES").at("BW");

    const bool P_conservation = true;

    const int                     n = res.p.spinX2 + 1;  // n = 2J + 1
    MMatrix<std::complex<double>> rho(n, n);

    // Construct the density matrix
    if (res.p.spinX2 == 0) {
      rho[0][0] = 1.0;  // Unit

    } else {
      // Draw random density matrices (until the number set by user)
      if (j.at("PARAM_RES").at("random_rho") > 0) {
        for (std::size_t k = 0; k < j.at("PARAM_RES").at("random_rho"); ++k) {
          rho = gra::spin::RandomRho(res.p.spinX2 / 2.0, P_conservation, rng);
        }

        // Construct spin density matrix from the input
      } else {
        for (std::size_t a = 0; a < rho.size_row(); ++a) {
          for (std::size_t b = 0; b < rho.size_col(); ++b) {
            const double Re = j.at("PARAM_RES").at("rho_real").at(a).at(b);
            const double Im = j.at("PARAM_RES").at("rho_imag").at(a).at(b);
            rho[a][b]       = Re + zi * Im;
          }
        }
        // Check positivity conditions
        if (gra::spin::Positivity(rho, res.p.spinX2 / 2.0) == false) {
          std::string str = "gra::form::ReadResonance: <" + resparam_str +
                            "> Input density matrix not positive definite!";
          throw std::invalid_argument(str);
        }
      }
      res.rho = rho;
    }
    std::cout << rang::fg::green << "[DONE]" << rang::fg::reset << std::endl;
  } catch (nlohmann::json::exception &e) {
    throw std::invalid_argument("form::ReadResonance: Missing parameter in '" + resparam_str +
                                "' : " + e.what());
  }

  return res;
}

// Regge signature factor, alpha_t is alpha(t), and signature sigma = +-1
//
// Remember that denominator hits pole at every integer PI * alpha(t)
//
std::complex<double> ReggeEta(double alpha_t, double sigma) {
  const double denom = std::sin(PI * alpha_t);
  return -(1.0 + sigma * std::exp(-zi * PI * alpha_t)) / denom;
}

// Regge signature factor for linear trajectories at t -> 0
//
//         t = Mandelstam t
//  alpha_t0 = alpha(0)
//        ap = alpha'
//     sigma = +-1
//
std::complex<double> ReggeEtaLinear(double t, double alpha_t0, double ap, double sigma) {
  return ReggeEta(alpha_t0, sigma) * std::exp(-zi * PI / 2.0 * ap * t);
}

// ----------------------------------------------------------------------
// Non-linear Pomeron trajectory functional form:
//
// [REFERENCE: Khoze, Martin, Ryskin, arxiv.org/abs/hep-ph/0007359]
double S3PomAlpha(double t) {
  // Additive quark model Beta_pi / Beta_p = 2/3 ->
  const double BETApi = (2.0 / 3.0) * PARAM_SOFT::gN_P;
  const double PIC    = (pow2(BETApi) * pow2(mpi)) / (32.0 * pow3(PI));

  // Pomeron trajectory value
  const double h       = PIC * S3HPL(4.0 * pow2(mpi) / std::abs(t),
                               t);  // pion loop insert (ADD with minus sign)
  const double alpha_P = 1.0 + PARAM_SOFT::DELTA_P + PARAM_SOFT::ALPHA_P * t - h;

  return alpha_P;
}

// Pion loop insert to the non-linear Pomeron trajectory
double S3HPL(double tau, double t) {
  const double m      = 1.0;  // fixed scale (GeV)
  const double sqrtau = msqrt(1 + tau);

  // Pion-Pomeron form factor parametrization
  const double F_pi = 1.0 / (1.0 - t / PARAM_SOFT::fc3);

  return (4.0 / tau) * pow2(F_pi) *
         (2.0 * tau - std::pow(1.0 + tau, 3.0 / 2.0) * std::log((sqrtau + 1.0) / (sqrtau - 1.0)) +
          std::log((m * m) / (mpi * mpi)));
}
// ----------------------------------------------------------------------

// Elastic proton form factor parametrization
//
// <apply at amplitude level>
// [REFERENCE: Khoze, Martin, Ryskin, arxiv.org/abs/hep-ph/0007359]
double S3F(double t) {
  return (1.0 / (1 - t / PARAM_SOFT::fc1)) * (1.0 / (1 - t / PARAM_SOFT::fc2));
}

// Proton inelastic form factor / structure function
// parametrization for Pomeron processes [THIS FUNCTION IS ANSATZ - IMPROVE!]
//
// Motivated by arxiv.org/abs/hep-ph/9305319
//
// <apply at amplitude level>
double S3FINEL(double t, double M2) {
  constexpr double DELTA_P = 0.0808;
  constexpr double a       = 0.5616;  // GeV^{2}
  double           f       = std::pow(std::abs(t) / (M2 * (std::abs(t) + a)), 0.5 * (1 + DELTA_P));

  return f;
}

// Proton inelastic structure function F2(x,Q^2) parametrization
//
// The basic idea is that at low-Q^2, a fully non-perturbative description
// (parametrization) is needed.
// At high Q^2, DGLAP evolution could be done in log(Q^2) starting from the
// input description.
//
// Now, some (very) classic ones have been implemented. Add new one here!
//
// [REFERENCE: Donnachie, Landshoff, arxiv.org/abs/hep-ph/9305319]
// [REFERENCE: Capella, Kaidalov, Merino, Tran Tranh Van, arxiv.org/abs/hep-ph/9405338v1]
//
double F2xQ2(double xbj, double Q2) {
  if (PARAM_STRUCTURE::F2 == "DL") {
    constexpr double A = 0.324;
    constexpr double B = 0.098;

    constexpr double DELTA_P = 0.0808;
    constexpr double DELTA_R = 0.5475;

    constexpr double a = 0.561991692786383;
    constexpr double b = 0.011133;

    const double F2 = A * std::pow(xbj, -DELTA_P) * std::pow(Q2 / (Q2 + a), 1 + DELTA_P) +
                      B * std::pow(xbj, 1 - DELTA_R) * std::pow(Q2 / (Q2 + b), DELTA_R);

    return F2;
  }

  else if (PARAM_STRUCTURE::F2 == "CKMT") {
    constexpr double A       = 0.1502;
    constexpr double B_u     = 1.2064;
    constexpr double B_d     = 0.1798;
    constexpr double alpha_R = 0.4150;
    constexpr double DELTA_0 = 0.0800;

    constexpr double a = 0.2631;
    constexpr double b = 0.6452;
    constexpr double c = 3.5489;
    constexpr double d = 1.1170;

    const double n_Q2     = (3.0 / 2.0) * (1 + Q2 / (Q2 + c));
    const double DELTA_Q2 = DELTA_0 * (1 + (2 * Q2) / (Q2 + d));

    const double C1 = std::pow(Q2 / (Q2 + a), 1.0 + DELTA_Q2);
    const double C2 = std::pow(Q2 / (Q2 + b), alpha_R);

    const double F2 = A * std::pow(xbj, -DELTA_Q2) * std::pow(1 - xbj, n_Q2 + 4.0) * C1 +
                      std::pow(xbj, 1.0 - alpha_R) *
                          (B_u * std::pow(1 - xbj, n_Q2) + B_d * std::pow(1 - xbj, n_Q2 + 1.0)) *
                          C2;

    return F2;
  } else {
    throw std::invalid_argument("gra::form::F2xQ2: Unknown PARAM_STRUCTURE::F2 = " +
                                PARAM_STRUCTURE::F2);
  }
}

// "Purely magnetic structure function"
//
// Callan-Gross relation for spin-1/2: F_2(x) = 2xF_1(x) under Bjorken scaling
// For spin-0, F_1(x) = 0
//
// Longitudinal structure function definition (e.g. QCD):
// F_L(xbj,Q2) = (1 + 4*pow2(xbj*mp)/Q2) * F2(xbj, Q2) - 2xbj * F1(xbj,Q2)
//
double F1xQ2(double xbj, double Q2) { return F2xQ2(xbj, Q2) / (2.0 * xbj); }

// QED: Proton magnetic moment in magneton units (mu_p / mu_N)
double mu_ratio() { return 2.792847337; }

// ============================================================================
// Photon flux densities and form factors, input Q^2 as positive

// kT unintegrated coherent EPA photon flux as in:
//
// [REFERENCE: Luszczak, Schaefer, Szczurek, arxiv.org/abs/1802.03244]
//
// Form factors:
// [REFERENCE, Punjabi et al., arxiv.org/abs/1503.01452v4]
//
// Proton electromagnetic form factors: Basic notions, present
// achievements and future perspectives, Physics Reports, 2015
// <www.sciencedirect.com/science/article/pzi/S0370157314003184>
//
// [REFERENCE: Budnev, Ginzburg, Meledin, Serbo, EPA paper, Physics Reports, 1976]
// <www.sciencedirect.com/science/article/pzi/0370157375900095>
//
//
// Proton EM form factor F1 (Dirac)
double F1(double Q2) {
  Q2               = std::abs(Q2);
  const double tau = Q2 / pow2(2 * mp);

  return 1.0 / (tau + 1) * G_E(Q2) + tau / (tau + 1) * G_M(Q2);
}

// Proton EM form factor F2 (Pauli)
double F2(double Q2) {
  Q2               = std::abs(Q2);
  const double tau = Q2 / pow2(2 * mp);

  return -1.0 / (tau + 1) * G_E(Q2) + 1.0 / (tau + 1) * G_M(Q2);
}

// Rosenbluth separation:
// low-Q^2 dominated by G_E, high-Q^2 dominated by G_M

// "Sachs Form Factor" goes as follows:
// G_E(0) = 1 for proton, 0 for neutron
// G_M(0) = mu_p for proton, mu_n for neutrons

// <http://www.scholarpedia.org/article/Nucleon_Form_factors>

double G_E(double Q2) {
  Q2 = std::abs(Q2);  // For safety

  if (PARAM_STRUCTURE::EM == "DIPOLE") {
    return G_E_DIPOLE(Q2);
  } else if (PARAM_STRUCTURE::EM == "KELLY") {
    return G_E_KELLY(Q2);
  } else {
    throw std::invalid_argument("gra::form::G_E: Unknown proton EM-form factor chosen = " +
                                PARAM_STRUCTURE::EM);
  }
}

double G_M(double Q2) {
  Q2 = std::abs(Q2);  // For safety

  if (PARAM_STRUCTURE::EM == "DIPOLE") {
    return G_M_DIPOLE(Q2);
  } else if (PARAM_STRUCTURE::EM == "KELLY") {
    return G_M_KELLY(Q2);
  } else {
    throw std::invalid_argument("gra::form::G_M: Unknown proton EM-form factor chosen = " +
                                PARAM_STRUCTURE::EM);
  }
}


// The simplest possible: Dipole parametrization of nucleon EM-form factors
//
//
double G_E_DIPOLE(double Q2) {
  return G_M(Q2) / mu_ratio();  // Scaling assumption
}
double G_M_DIPOLE(double Q2) {
  constexpr double lambda2 = 0.71;  // Dipole parameter GeV^2
  return mu_ratio() / pow2(1.0 + Q2 / lambda2);
}

// Simple parametrization of nucleon EM-form factors
//
// [REFERENCE: Kelly, journals.aps.org/prc/pdf/10.1103/PhysRevC.70.068202]
double G_E_KELLY(double Q2) {
  static const std::vector<double> a = {1, -0.24};
  static const std::vector<double> b = {10.98, 12.82, 21.97};

  const double tau = Q2 / pow2(2 * mp);

  // Numerator
  double num = 0.0;   // 0
  num += a[0];        // a_0 tau^0
  num += a[1] * tau;  // a_1 tau^1

  // Denominator
  double den = 1.0;         // 1.0
  den += b[0] * tau;        // b_1 tau^1
  den += b[1] * pow2(tau);  // b_2 tau^2
  den += b[2] * pow3(tau);  // b_3 tau^3

  return num / den;
}

double G_M_KELLY(double Q2) {
  static const std::vector<double> a = {1, 0.12};
  static const std::vector<double> b = {10.97, 18.86, 6.55};

  const double tau = Q2 / pow2(2 * mp);

  // Numerator
  double num = 0.0;   // 0
  num += a[0];        // a_0 tau^0
  num += a[1] * tau;  // a_1 tau^1

  // Denominator
  double den = 1.0;         // 1.0
  den += b[0] * tau;        // b_1 tau^1
  den += b[1] * pow2(tau);  // b_2 tau^2
  den += b[2] * pow3(tau);  // b_3 tau^3

  return mu_ratio() * num / den;
}

// Coherent photon flux from proton
// xi ~ longitudinal momentum loss [0,1]
// t  ~ Mandelstam t
// pt ~ proton transverse momentum
//
//
//  p ---------F_E---------> p' with xi = 1 - p^*_z'/p_z
//               $
//                $
//                 $
//
// Factors applied here, compatible with 2 -> N phase space sampling:
//
//  1/xi    [~ sub Moller flux]
//  1/pt2   [~ kt-factorization] (cancels with pt2 from numerator)
//  16pi^2  [~ kinematics volume factor]
//
double CohFlux(double xi, double t, double pt) {
  const double pt2 = pow2(pt);
  const double xi2 = pow2(xi);
  const double mp2 = pow2(mp);
  const double Q2  = std::abs(t);

  const double PART1 =
      (4.0 * pow2(mp) * pow2(G_E(Q2)) + Q2 * pow2(G_M(Q2))) / (4.0 * pow2(mp) + Q2);
  const double PART2 = pow2(G_M(Q2));
  const double DELTA = pt2 / (pt2 + xi2 * mp2);

  double f =
      qed::alpha_QED() / PI * ((1.0 - xi) * pow2(DELTA) * PART1 + (xi2 / 4.0) * DELTA * PART2);

  // Factors
  f /= xi;
  f /= pt2;
  f *= 16.0 * gra::math::PIPI;

  return f;  // Use at cross section level
}

// Incoherent photon flux from a dissociated proton with mass M.
// When M -> mp, this reproduces CohFlux() if
// F2(x,Q^2) does reproduce the elastic limit (not all parametrizations do)
//
//
//  p ---------F2(x,Q^2)----->  p* with xi = 1 - p^*_z / p_z
//             -------------->
//             ---x---------->
//                 $
//                  $
//                   $
//
// Factors applied as with CohFlux() above.
//
double IncohFlux(double xi, double t, double pt, double M2) {
  constexpr double mp2 = pow2(mp);

  const double pt2   = pow2(pt);
  const double xi2   = pow2(xi);
  const double Q2    = std::abs(t);
  const double xbj   = Q2 / (Q2 + M2 - mp2);  // Bjorken-x
  const double DELTA = pt2 / (pt2 + xi * (M2 - mp2) + xi2 * mp2);

  double f = qed::alpha_QED() / PI *
             ((1.0 - xi) * pow2(DELTA) * F2xQ2(xbj, Q2) / (Q2 + M2 - mp2) +
              (xi2 / (4.0 * pow2(xbj))) * DELTA * 2.0 * xbj * F1xQ2(xbj, Q2) / (Q2 + M2 - mp2));

  // Factors
  f /= xi;
  f /= pt2;
  f *= 16.0 * gra::math::PIPI;

  return f;  // Use at cross section level
}

// Drees-Zeppenfeld proton coherent gamma flux (collinear)
double DZFlux(double x) {
  const double Q2min = (pow2(mp) * pow2(x)) / (1.0 - x);
  const double A     = 1.0 + 0.71 / Q2min;

  double f = qed::alpha_QED() / (2.0 * PI * x) * (1.0 + pow2(1.0 - x)) *
             (std::log(A) - 11.0 / 6.0 + 3.0 / A - 3.0 / (2.0 * pow2(A)) + 1.0 / (3 * pow3(A)));

  return f;  // Use at cross section level
}

// Breit-Wigner propagators / form factors
//
//
//
// Useful identity for normalization:
//
// \int dm^2 \frac{1}{(m^2 - M0^2)^2 + M0^2Gamma^2} \equiv \frac{\pi}{M0 Gamma}
//
// based on squaring the complex propagator
// D(m^2) = 1 / (m^2 - M0^2 + iM0*Gamma), and integrating.
//
std::complex<double> CBW(const gra::LORENTZSCALAR &lts, const gra::PARAM_RES &resonance) {
  switch (resonance.BW) {
    case 1:
      return CBW_FW(lts.m2, resonance.p.mass, resonance.p.width);
    case 2:
      return CBW_RW(lts.m2, resonance.p.mass, resonance.p.width);
    case 3:
      return CBW_BF(lts.m2, resonance.p.mass, resonance.p.width, resonance.p.spinX2 / 2.0,
                    lts.decaytree[0].p4.M(), lts.decaytree[1].p4.M());
    case 4:
      return CBW_JR(lts.m2, resonance.p.mass, resonance.p.width, resonance.p.spinX2 / 2.0);

    default:
      throw std::invalid_argument("CBW: Unknown BW (Breit-Wigner) parameter: " +
                                  std::to_string(resonance.BW));
  }
}

// See e.g.
// [REFERENCE: TASI Lectures on propagators, users.ictp.it/~smr2244/tait-supplemental.pdf]
// [REFERENCE: Cacciapaglia, Deandrea, Curtis, arxiv.org/abs/0906.3417v2]
// [REFERENCE:
// www.t2.ucsd.edu/twiki2/pub/UCSDTier2/Physics214Spring2015/ajw-breit-wigner-cbx99-55.pdf]

// ----------------------------------------------------------------------
// Delta function \delta(\hat{s} - M_0^2) replacement function:
//    \int d\hat{s} \delta(\hat{s} - M_0^2) -> \int d\hat{s}
//    deltaBW(\hat{s},M0,Gamma)
//
// To be applied at cross section level
double deltaBWxsec(double shat, double M0, double Gamma) {
  return M0 * Gamma / PI / (pow2(shat - pow2(M0)) + pow2(M0 * Gamma));
}

// To be applied at amplitude level
double deltaBWamp(double shat, double M0, double Gamma) {
  return msqrt(deltaBWxsec(shat, M0, Gamma));
}
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Breit-Wigner propagator parametrizations

// 1. Complex Fixed Width Relativistic Breit-Wigner
std::complex<double> CBW_FW(double m2, double M0, double Gamma) {
  return -1.0 / (m2 - M0 * M0 + zi * M0 * Gamma);
}

// 2. Complex Running Width Relativistic Breit-Wigner
std::complex<double> CBW_RW(double m2, double M0, double Gamma) {
  return -1.0 / (m2 - M0 * M0 + zi * std::sqrt(m2) * Gamma);
}

// 3. J = 0,1,2 Complex Relativistic Breit-Wigner
// with angular barrier effects type of Bleit-Weiskopf
// mA and mB are the masses of daughters (GeV)
std::complex<double> CBW_BF(double m2, double M0, double Gamma, int J, double mA, double mB) {
  const double u       = msqrt((m2 - pow2(mA + mB)) * (m2 - pow2(mA - mB))) / (2 * msqrt(m2));
  const double d       = msqrt((M0 * M0 - pow2(mA + mB)) * (M0 * M0 - pow2(mA - mB))) / (2 * M0);
  const double Bfactor = pow(u / d, 2 * J + 1);
  return -1.0 / (m2 - M0 * M0 + zi * Gamma * M0 * M0 / msqrt(m2) * Bfactor);
}

// 4. Spin dependent Relativistic Breit-Wigner (should not be used blindly!)
//
// [REFERENCE: Alwall el al., arxiv.org/abs/1402.1178]
//
std::complex<double> CBW_JR(double m2, double M0, double Gamma, double J) {
  const std::complex<double> denom = (m2 - M0 * M0 + zi * M0 * Gamma);

  if (static_cast<int>(J) == 0) {  // J = 0
    return -1.0 / denom;
  } else if (static_cast<int>(J * 2) == 1) {  // J = 1/2
    return -2.0 * msqrt(m2) / denom;
  } else if (static_cast<int>(J) == 1) {  // J = 1
    return -(1.0 - m2 / (M0 * M0)) / denom;
  } else if (static_cast<int>(J * 2) == 3) {  // J = 3/2
    return -(2.0 / 3.0) * msqrt(m2) * (1.0 - m2 / (M0 * M0)) / denom;
  } else if (static_cast<int>(J) == 2) {  // J = 2
    return -(7.0 / 6.0 - (4.0 / 3.0) * (m2 / (M0 * M0)) +
             (2.0 / 3.0) * (m2 * m2) / (gra::math::pow4(M0))) /
           denom;
  } else {
    return -1.0 / denom;  // Too high spin
  }
}

}  // namespace form
}  // namespace gra
