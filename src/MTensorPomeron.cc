// Tensor Pomeron amplitudes
//
//
// [REFERENCE: Ewerz, Maniatis, Nachtmann, arxiv.org/abs/1309.3478]
// [REFERENCE: Lebiodowicz, Nachtmann, Szczurek, arxiv.org/abs/1601.04537]
// [REFERENCE: LNS, arxiv.org/abs/1801.03902]
// [REFERENCE: LNS, arxiv.org/abs/1901.11490]
//
// (c) 2017-2021 Mikael Mieskolainen
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

// C++
#include <complex>
#include <iostream>
#include <vector>

// Own
#include "Graniitti/M4Vec.h"
#include "Graniitti/MAux.h"
#include "Graniitti/MDirac.h"
#include "Graniitti/MForm.h"
#include "Graniitti/MKinematics.h"
#include "Graniitti/MMath.h"
#include "Graniitti/MQED.h"
#include "Graniitti/MRegge.h"
#include "Graniitti/MTensorPomeron.h"

// FTensor algebra
#include "FTensor.hpp"

// LOOP MACROS
#define FOR_EACH_2(X)       \
  for (const auto &u : X) { \
    for (const auto &v : X) {
#define FOR_EACH_2_END \
  }                    \
  }

#define FOR_EACH_3(X)         \
  for (const auto &u : X) {   \
    for (const auto &v : X) { \
      for (const auto &k : X) {
#define FOR_EACH_3_END \
  }                    \
  }                    \
  }

#define FOR_EACH_4(X)           \
  for (const auto &u : X) {     \
    for (const auto &v : X) {   \
      for (const auto &k : X) { \
        for (const auto &l : X) {
#define FOR_EACH_4_END \
  }                    \
  }                    \
  }                    \
  }

#define FOR_EACH_5(X)             \
  for (const auto &u : X) {       \
    for (const auto &v : X) {     \
      for (const auto &k : X) {   \
        for (const auto &l : X) { \
          for (const auto &r : X) {
#define FOR_EACH_5_END \
  }                    \
  }                    \
  }                    \
  }                    \
  }

#define FOR_EACH_6(X)               \
  for (const auto &u : X) {         \
    for (const auto &v : X) {       \
      for (const auto &k : X) {     \
        for (const auto &l : X) {   \
          for (const auto &r : X) { \
            for (const auto &s : X) {
#define FOR_EACH_6_END \
  }                    \
  }                    \
  }                    \
  }                    \
  }                    \
  }

#define FOR_PP_HELICITY               \
  for (const auto &ha : {0, 1}) {     \
    for (const auto &hb : {0, 1}) {   \
      for (const auto &h1 : {0, 1}) { \
        for (const auto &h2 : {0, 1}) {
#define FOR_PP_HELICITY_END \
  }                         \
  }                         \
  }                         \
  }

using gra::aux::indices;
using gra::math::msqrt;
using gra::math::PI;
using gra::math::pow2;
using gra::math::zi;

using FTensor::Tensor0;
using FTensor::Tensor1;
using FTensor::Tensor2;
using FTensor::Tensor3;
using FTensor::Tensor4;

namespace gra {

// Constructor
MTensorPomeron::MTensorPomeron(gra::LORENTZSCALAR &lts, const std::string &modelfile) {
  CalcRTensor();  // Pre-Calculate tensors

  // @@ MULTITHREADING LOCK NEEDED FOR THE INITIALIZATION @@
  gra::g_mutex.lock();

  if (!Param.initialized) {
    try {
      Param.ReadParameters(modelfile);
      PARAM_REGGE::ReadParameters(0, modelfile);

    } catch (...) {
      gra::g_mutex.unlock();  // need to release here, otherwise get infinite lock
      throw;
    }
  }
  gra::g_mutex.unlock();
}


// Return decay coupling constant for resonance (M,Gamma) with decay daughter mass mf
// BR being the branching ratio BR = Width_partial / Width_total
//
// Decay: Mother (spin = 0/1/2) -> scalar or pseudoscalar daughters
//
double MTensorPomeron::GDecay(int J, double M, double Gamma, double mf, double BR) {
  const double S0           = 1.0;  // Should be set as the same scale as in decay amplitudes iG[]
  const double partialWidth = Gamma * BR;

  const double P = sqrt(1 - 4 * pow2(mf / M));

  if (J == 0) {
    return sqrt(partialWidth / (1.0 / (16 * PI * M) * pow2(S0) * P));
  } else if (J == 1) {
    return sqrt(partialWidth / (M / (192 * PI) * math::pow3(P)));
  } else if (J == 2) {
    return sqrt(partialWidth / (M / (480 * PI) * pow2(M / S0) * math::pow5(P)));
  } else if (J >= 3) {
    std::cout << "MTensorPomeron::GDecay: Spin-" + std::to_string(J) +
                     " support not implemented for Tensor Pomeron model!"
              << std::endl
              << std::endl;
    return 1.0;  // future
  } else {
    throw std::invalid_argument("MTensorPomeron::GDecay: Unknown input spin J = " +
                                std::to_string(J));
  }
}


// 2 -> 3 amplitudes
//
// return value: matrix element squared with helicities summed and averaged over
// lts.hamp:     individual complex helicity amplitudes
//
double MTensorPomeron::ME3(gra::LORENTZSCALAR &lts) const {
  // Free Lorentz indices [second parameter denotes the range of index]
  FTensor::Index<'a', 4> mu1;
  FTensor::Index<'b', 4> nu1;
  FTensor::Index<'c', 4> rho1;
  FTensor::Index<'d', 4> rho2;
  FTensor::Index<'g', 4> alpha1;
  FTensor::Index<'h', 4> beta1;
  FTensor::Index<'i', 4> alpha2;
  FTensor::Index<'j', 4> beta2;
  FTensor::Index<'k', 4> mu2;
  FTensor::Index<'l', 4> nu2;

  // Kinematics
  const M4Vec pa = lts.pbeam1;
  const M4Vec pb = lts.pbeam2;

  const M4Vec p1 = lts.pfinal[1];
  const M4Vec p2 = lts.pfinal[2];
  const M4Vec p3 = lts.decaytree[0].p4;
  const M4Vec p4 = lts.decaytree[1].p4;

  // ------------------------------------------------------------------

  // Spinors (2 helicities)
  const std::array<std::vector<std::complex<double>>, 2> u_a = SpinorStates(pa, "u");
  const std::array<std::vector<std::complex<double>>, 2> u_b = SpinorStates(pb, "u");

  const std::array<std::vector<std::complex<double>>, 2> ubar_1 = SpinorStates(p1, "ubar");
  const std::array<std::vector<std::complex<double>>, 2> ubar_2 = SpinorStates(p2, "ubar");

  // ------------------------------------------------------------------

  // t-channel Pomeron propagators
  const Tensor4<std::complex<double>, 4, 4, 4, 4> iDP_1 = iD_P(lts.s1, lts.t1);
  const Tensor4<std::complex<double>, 4, 4, 4, 4> iDP_2 = iD_P(lts.s2, lts.t2);

  // High Energy Limit proton-Pomeron-proton spinor structure
  const Tensor2<std::complex<double>, 4, 4> iG_1a = iG_PppHE(p1, pa);
  const Tensor2<std::complex<double>, 4, 4> iG_2b = iG_PppHE(p2, pb);

  // ------------------------------------------------------------------

  // Reset helicity amplitude container
  // Init with zeros, important for the coherent sum!
  lts.hamp             = std::vector<std::complex<double>>(64, 0.0);
  std::size_t maxindex = 0;

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // 2. Coherent sum of Resonances (loop over)
  for (auto &x : lts.RESONANCES) {
    const PARAM_RES res = x.second;

    // Resonance parameters
    const double M0    = res.p.mass;
    const double Gamma = res.p.width;
    const int    J     = res.p.spinX2 / 2.0;
    const int    P     = res.p.P;

    // ------------------------------------------------------------------

    // =====================================================================
    // Pomeron-Pomeron-Scalar structure
    //
    //
    if (J == 0 && P == 1) {
      Tensor4<std::complex<double>, 4, 4, 4, 4> cvtx;
      std::vector<std::complex<double>>         eps3eps4;

      cvtx = iG_PPS_total(lts.q1, lts.q2, M0, "scalar", res.g_Tensor);

      // Scalar BW-propagator
      const std::complex<double> iD = iD_MES(lts.pfinal[0], M0, Gamma);

      // [PS PS]  Pseudoscalar pair decay
      if (lts.decaytree[0].p.spinX2 == 0 && lts.decaytree[1].p.spinX2 == 0) {
        eps3eps4.clear();
        eps3eps4.push_back(
            iG_f0ss(lts.decaytree[0].p4, lts.decaytree[1].p4, M0, res.hel.g_decay_tensor[0]));
      }

      // [V   V]  Massive vector pair decay
      else if (lts.decaytree[0].p.spinX2 == 2 && lts.decaytree[1].p.spinX2 == 2) {
        if (res.hel.g_decay_tensor.size() != 2) {
          throw std::invalid_argument(
              "MTensorPomeron:: S->VV Coupling array [size 2] 'hel.g_decay_tensor' not in "
              "BRANCHING.json for resonance PDG = " +
              std::to_string(res.p.pdg));
        }
        // Decay vertex with different outgoing helicity combinations
        Tensor2<std::complex<double>, 4, 4> iGf0vv =
            iG_f0vv(p3, p4, M0, res.hel.g_decay_tensor[0], res.hel.g_decay_tensor[1]);
        eps3eps4 = MassiveSpin1PolSum(iGf0vv, p3, p4);

        // Sequential decay correlations [TBD]
        // const Tensor2<std::complex<double>, 4, 4> iGvv2psps = iG_vv2psps({},
        // lts.decaytree[0].p.pdg);  eps3eps4.clear();  eps3eps4.push_back( iGf0vv(mu1, mu2) *
        // iGvv2psps(mu1, mu2) );

      } else {
        throw std::invalid_argument("MTensorPomeron::ME3: Unknown decay mode for scalar resonance");
      }

      // Two helicity states for incoming and outgoing protons
      std::size_t index = 0;
      FOR_PP_HELICITY;

      // Apply proton leg helicity conservation / No helicity flip (high energy limit)
      if (ha != h1 || hb != h2) { continue; }

      // Full proton-Pomeron-proton spinor structure (upper and lower vertex)
      // const Tensor2<std::complex<double>,4,4> iG_1a = iG_Ppp(p1, pa, ubar_1[h1], u_a[ha]);
      // const Tensor2<std::complex<double>,4,4> iG_2b = iG_Ppp(p2, pb, ubar_2[h2], u_b[hb]);

      // Evaluate block
      const std::complex<double> block = (-zi) * iG_1a(mu1, nu1) * iDP_1(mu1, nu1, alpha1, beta1) *
                                         cvtx(alpha1, beta1, alpha2, beta2) * iD *
                                         iDP_2(alpha2, beta2, mu2, nu2) * iG_2b(mu2, nu2);

      // s-channel amplitude for each outgoing helicity combination
      for (const auto &ind : indices(eps3eps4)) {
        const std::complex<double> A = block * eps3eps4[ind];
        lts.hamp[index] += A;  // Note +=
        ++index;
      }

      FOR_PP_HELICITY_END;
      maxindex = index > maxindex ? index : maxindex;

      // =====================================================================
      // Pomeron-Gamma-Vector (Photoproduction of rho, phi ...) structure
      //
      // Should add vector meson mass dependent running of t-form factors
      //
      // p --x--------------- p
      //      *
      //     y *   rho0
      //        *x=====x===== rho0
      //               |
      //               | P
      //               |
      // p ------------x----- p
      //
    } else if (J == 1 && P == -1) {
      // Gamma-Vector coupling
      const Tensor2<std::complex<double>, 4, 4> iGyV = iG_yV(0, res.p.pdg);

      // Gamma propagators
      const Tensor2<std::complex<double>, 4, 4> iDy_1 = iD_y(lts.q1.M2());
      const Tensor2<std::complex<double>, 4, 4> iDy_2 = iD_y(lts.q2.M2());

      // Vector-meson propagators
      const bool                                INDEX_UP = true;
      const Tensor2<std::complex<double>, 4, 4> iDV_1    = iD_VMES(lts.q1, M0, Gamma, INDEX_UP);
      const Tensor2<std::complex<double>, 4, 4> iDV   = iD_VMES(lts.pfinal[0], M0, Gamma, INDEX_UP);
      const Tensor2<std::complex<double>, 4, 4> iDV_2 = iD_VMES(lts.q2, M0, Gamma, INDEX_UP);

      // Pomeron-Vector-Vector coupling
      const Tensor4<std::complex<double>, 4, 4, 4, 4> iGPvv_1 =
          iG_Pvv(lts.pfinal[0], lts.q1, res.g_Tensor[0], res.g_Tensor[1]);
      const Tensor4<std::complex<double>, 4, 4, 4, 4> iGPvv_2 =
          iG_Pvv(lts.pfinal[0], lts.q2, res.g_Tensor[0], res.g_Tensor[1]);

      // Vector-Pseudoscalar-Pseudoscalar coupling
      const Tensor1<std::complex<double>, 4> iGvpsps =
          iG_vpsps(p3, p4, M0, res.hel.g_decay_tensor[0]);

      // Two helicity states for incoming and outgoing protons
      std::size_t index = 0;
      FOR_PP_HELICITY;

      // Apply proton leg helicity conservation / No helicity flip (high energy limit)
      if (ha != h1 || hb != h2) { continue; }

      // Full proton-gamma-proton spinor structure (upper and lower vertex)
      const Tensor1<std::complex<double>, 4> iG_1 = iG_ypp(p1, pa, ubar_1[h1], u_a[ha]);
      const Tensor1<std::complex<double>, 4> iG_2 = iG_ypp(p2, pb, ubar_2[h2], u_b[hb]);

      // Full proton-Pomeron-proton spinor structure (upper and lower vertex)
      const Tensor2<std::complex<double>, 4, 4> iG_1a = iG_Ppp(p1, pa, ubar_1[h1], u_a[ha]);
      const Tensor2<std::complex<double>, 4, 4> iG_2b = iG_Ppp(p2, pb, ubar_2[h2], u_b[hb]);

      // Gamma[1] - Pomeron[2] amplitude
      const std::complex<double> iM_yP = iG_1(mu1) * iDy_1(mu1, mu2) * iGyV(mu2, nu1) *
                                         iDV_1(nu1, rho1) * iDV(rho2, nu2) * iGvpsps(nu2) *
                                         iGPvv_1(rho2, rho1, alpha1, beta1) *
                                         iDP_2(alpha1, beta1, alpha2, beta2) * iG_2b(alpha2, beta2);

      // Pomeron[2] - Gamma[1] amplitude
      const std::complex<double> iM_Py = iG_2(mu1) * iDy_2(mu1, mu2) * iGyV(mu2, nu1) *
                                         iDV_2(nu1, rho1) * iDV(rho2, nu2) * iGvpsps(nu2) *
                                         iGPvv_2(rho2, rho1, alpha1, beta1) *
                                         iDP_1(alpha1, beta1, alpha2, beta2) * iG_1a(alpha2, beta2);

      // Total helicity amplitude
      const std::complex<double> A = (-zi) * (iM_yP + iM_Py);
      lts.hamp[index] += A;  // Note +=
      ++index;

      FOR_PP_HELICITY_END;
      maxindex = index > maxindex ? index : maxindex;

      // =====================================================================
      // Pomeron-Pomeron-Pseudoscalar structure
      //
      //
    } else if (J == 0 && P == -1) {
      Tensor4<std::complex<double>, 4, 4, 4, 4> cvtx;
      std::vector<std::complex<double>>         eps3eps4;
      cvtx = iG_PPS_total(lts.q1, lts.q2, M0, "pseudoscalar", res.g_Tensor);

      // Scalar BW-propagator
      const std::complex<double> iD = iD_MES(lts.pfinal[0], M0, Gamma);

      // 2 x Gamma decay part
      if (lts.decaytree.size() != 2 || lts.decaytree[0].p.pdg != 22 ||
          lts.decaytree[1].p.pdg != 22) {
        throw std::invalid_argument(
            "MTensorPomeron:: [PS decay] Only decay to gamma pair supported");
      }

      // Decay vertex with different outgoing helicity combinations
      Tensor2<std::complex<double>, 4, 4> iDECAY = iG_psvv(p3, p4, M0, res.hel.g_decay_tensor[0]);
      eps3eps4                                   = MasslessSpin1PolSum(iDECAY, p3, p4);

      // Two helicity states for incoming and outgoing protons
      std::size_t index = 0;
      FOR_PP_HELICITY;

      // Apply proton leg helicity conservation / No helicity flip (high energy limit)
      if (ha != h1 || hb != h2) { continue; }

      // Full proton-Pomeron-proton spinor structure (upper and lower vertex)
      // const Tensor2<std::complex<double>,4,4> iG_1a = iG_Ppp(p1, pa, ubar_1[h1], u_a[ha]);
      // const Tensor2<std::complex<double>,4,4> iG_2b = iG_Ppp(p2, pb, ubar_2[h2], u_b[hb]);

      // Evaluate block
      const std::complex<double> block = (-zi) * iG_1a(mu1, nu1) * iDP_1(mu1, nu1, alpha1, beta1) *
                                         cvtx(alpha1, beta1, alpha2, beta2) * iD *
                                         iDP_2(alpha2, beta2, mu2, nu2) * iG_2b(mu2, nu2);

      // s-channel amplitude for each outgoing vector state helicity combination
      for (const auto &ind : indices(eps3eps4)) {
        const std::complex<double> A = block * eps3eps4[ind];
        lts.hamp[index] += A;  // Note +=
        ++index;
      }

      FOR_PP_HELICITY_END;
      maxindex = index > maxindex ? index : maxindex;

      // =====================================================================
      // Pomeron-Pomeron-Tensor structure
      //
      //
    } else if (J == 2) {
      // Rank-6 tensor structure
      const MTensor<std::complex<double>> iGPPf2 = iG_PPT_total(lts.q1, lts.q2, M0, res.g_Tensor);

      // Tensor BW-propagator
      const bool                                      INDEX_UP = true;
      const Tensor4<std::complex<double>, 4, 4, 4, 4> iDf2 =
          iD_TMES(lts.pfinal[0], M0, Gamma, INDEX_UP);

      // Total block
      std::vector<Tensor2<std::complex<double>, 4, 4>> iD;

      // [PS PS] Pseudoscalar pair decay
      if (lts.decaytree[0].p.spinX2 == 0 && lts.decaytree[1].p.spinX2 == 0) {
        const Tensor2<std::complex<double>, 4, 4> iGf2psps =
            iG_f2psps(p3, p4, M0, res.hel.g_decay_tensor[0]);

        Tensor2<std::complex<double>, 4, 4> temp;
        temp(mu1, nu1) = iDf2(mu1, nu1, rho1, rho2) * iGf2psps(rho1, rho2);
        iD.push_back(temp);

        // [V  V] Massive vector pair decay
      } else if (lts.decaytree[0].p.spinX2 == 2 && lts.decaytree[1].p.spinX2 == 2 &&
                 lts.decaytree[0].p.pdg != 22 && lts.decaytree[1].p.mass != 22) {
        if (res.hel.g_decay_tensor.size() != 2) {
          throw std::invalid_argument(
              "MTensorPomeron:: T->VV Coupling array [size 2] 'hel.g_decay_tensor' not in "
              "BRANCHING.json for resonance PDG = " +
              std::to_string(res.p.pdg));
        }
        const Tensor4<std::complex<double>, 4, 4, 4, 4> iGf2vv =
            iG_f2vv(p3, p4, M0, res.hel.g_decay_tensor[0], res.hel.g_decay_tensor[1]);
        const std::vector<Tensor2<std::complex<double>, 4, 4>> eps3eps4 =
            MassiveSpin1PolSum(iGf2vv, p3, p4);

        // Over all helicity combinations
        Tensor2<std::complex<double>, 4, 4> temp;
        for (const auto &ind : indices(eps3eps4)) {
          // Contract
          temp(mu1, nu1) = iDf2(mu1, nu1, rho1, rho2) * eps3eps4[ind](rho1, rho2);
          iD.push_back(temp);
        }

        // Sequential spin correlated decay treatment [TBD]
        /*
        const Tensor2<std::complex<double>, 4,4> iGvv2psps = iG_vv2psps({}, lts.decaytree[0].p.pdg);

        // Contract in two steps
        Tensor2<std::complex<double>, 4,4> A;
        A(alpha1, beta1) = iGf2vv(rho1, rho2, alpha1, beta1) * iGvv2psps(rho1, rho2);
        iD(mu1, nu1) = iDf2(mu1, nu1, alpha1, beta1) * A(alpha1, beta1);

        // --------------------------------------------------------------
        // *** CONTROL CASCADE SAMPLING ***
        lts.FORCE_FLATMASS2 = true;
        lts.FORCE_OFFSHELL  = 3.0;

        lts.decaytree[0].PS_active = true;
        lts.decaytree[1].PS_active = true;
        // --------------------------------------------------------------
        */

        // [y  y] Gamma pair decay
      } else if (lts.decaytree[0].p.pdg == 22 && lts.decaytree[1].p.pdg == 22) {
        if (res.hel.g_decay_tensor.size() != 2) {
          throw std::invalid_argument(
              "MTensorPomeron:: T->VV Coupling array [size 2] 'hel.g_decay_tensor' not in "
              "BRANCHING.json for resonance PDG = " +
              std::to_string(res.p.pdg));
        }
        Tensor4<std::complex<double>, 4, 4, 4, 4> iGf2yy =
            iG_f2yy(p3, p4, M0, res.hel.g_decay_tensor[0], res.hel.g_decay_tensor[1]);
        const std::vector<Tensor2<std::complex<double>, 4, 4>> eps3eps4 =
            MasslessSpin1PolSum(iGf2yy, p3, p4);

        // Over all helicity combinations
        Tensor2<std::complex<double>, 4, 4> temp;
        for (const auto &ind : indices(eps3eps4)) {
          // Contract
          temp(mu1, nu1) = iDf2(mu1, nu1, rho1, rho2) * eps3eps4[ind](rho1, rho2);
          iD.push_back(temp);
        }

      } else {
        throw std::invalid_argument("MTensorPomeron::ME3: Unknown decay mode for tensor resonance");
      }

      // Over all central helicity combinations
      Tensor4<std::complex<double>, 4, 4, 4, 4>              temp;
      std::vector<Tensor4<std::complex<double>, 4, 4, 4, 4>> cvtx;
      for (const auto &ind : indices(iD)) {
        // Construct central vertex
        FOR_EACH_4(LI);
        temp(u, v, k, l) = 0.0;
        for (auto const &rho : LI) {
          for (auto const &sigma : LI) {
            temp(u, v, k, l) += iGPPf2({u, v, k, l, rho, sigma}) * iD[ind](rho, sigma);
          }
        }
        FOR_EACH_4_END;
        cvtx.push_back(temp);
      }

      // Two helicity states for incoming and outgoing protons
      std::size_t index = 0;
      FOR_PP_HELICITY;

      // Apply proton leg helicity conservation / No helicity flip (high energy limit)
      if (ha != h1 || hb != h2) { continue; }

      // Full proton-Pomeron-proton spinor structure (upper and lower vertex)
      // const Tensor2<std::complex<double>,4,4> iG_1a = iG_Ppp(p1, pa, ubar_1[h1], u_a[ha]);
      // const Tensor2<std::complex<double>,4,4> iG_2b = iG_Ppp(p2, pb, ubar_2[h2], u_b[hb]);

      // s-channel amplitude
      for (const auto &ind : indices(cvtx)) {
        const std::complex<double> A = (-zi) * iG_1a(mu1, nu1) * iDP_1(mu1, nu1, alpha1, beta1) *
                                       cvtx[ind](alpha1, beta1, alpha2, beta2) *
                                       iDP_2(alpha2, beta2, mu2, nu2) * iG_2b(mu2, nu2);
        // Add it
        lts.hamp[index] += A;  // Note +=
        ++index;
      }

      FOR_PP_HELICITY_END;
      maxindex = index > maxindex ? index : maxindex;

    } else {
      throw std::invalid_argument("MTensorPomeron::ME3: Unknown spin-parity input");
    }
    // ====================================================================

  }  // Loop over resonances
  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  // Remove empty
  lts.hamp.resize(maxindex);

  // Get total amplitude squared 1/4 \sum_h |A_h|^2
  double SumAmp2 = 0.0;
  for (const auto &i : indices(lts.hamp)) { SumAmp2 += gra::math::abs2(lts.hamp[i]); }
  SumAmp2 /= 4;  // Initial state helicity average


  return SumAmp2;  // Amplitude squared
}


// 2 -> 4 amplitudes
//
// return value: matrix element squared with helicities summed over
// lts.hamp:     individual helicity amplitudes
//
double MTensorPomeron::ME4(gra::LORENTZSCALAR &lts) const {
  // Kinematics
  const M4Vec pa = lts.pbeam1;
  const M4Vec pb = lts.pbeam2;

  const M4Vec p1 = lts.pfinal[1];
  const M4Vec p2 = lts.pfinal[2];
  M4Vec       p3 = lts.decaytree[0].p4;
  M4Vec       p4 = lts.decaytree[1].p4;

  // Intermediate boson/fermion mass
  const double M_ = lts.decaytree[0].p.mass;

  // Momentum convention of sub-diagrams
  //
  // ------<------ anti-particle p3
  // |
  // | \hat{t} (arrow down)
  // |
  // ------>------ particle p4
  //
  // ------<------ particle p4
  // |
  // | \hat{u} (arrow up)
  // |
  // ------>------ anti-particle p3
  //
  const M4Vec pt = pa - p1 - p3;  // => q1 = pt + p3, q2 = pt - p4
  const M4Vec pu = p4 - pa + p1;  // => q2 = pu + p3, q1 = pu - p3

  // ------------------------------------------------------------------

  // Incoming and outgoing proton spinors (2 helicities)
  const std::array<std::vector<std::complex<double>>, 2> u_a = SpinorStates(pa, "u");
  const std::array<std::vector<std::complex<double>>, 2> u_b = SpinorStates(pb, "u");

  const std::array<std::vector<std::complex<double>>, 2> ubar_1 = SpinorStates(p1, "ubar");
  const std::array<std::vector<std::complex<double>>, 2> ubar_2 = SpinorStates(p2, "ubar");


  // t-channel Pomeron propagators
  const Tensor4<std::complex<double>, 4, 4, 4, 4> iDP_13 = iD_P(lts.ss[1][3], lts.t1);
  const Tensor4<std::complex<double>, 4, 4, 4, 4> iDP_24 = iD_P(lts.ss[2][4], lts.t2);

  // u-channel Pomeron propagators
  const Tensor4<std::complex<double>, 4, 4, 4, 4> iDP_14 = iD_P(lts.ss[1][4], lts.t1);
  const Tensor4<std::complex<double>, 4, 4, 4, 4> iDP_23 = iD_P(lts.ss[2][3], lts.t2);


  // High Energy Limit proton-Pomeron-proton spinor structure
  const Tensor2<std::complex<double>, 4, 4> iG_1a = iG_PppHE(p1, pa);
  const Tensor2<std::complex<double>, 4, 4> iG_2b = iG_PppHE(p2, pb);

  // ------------------------------------------------------------------

  // Reset
  lts.hamp.clear();

  // Free Lorentz indices [second parameter denotes the range of index]
  FTensor::Index<'a', 4> mu1;
  FTensor::Index<'b', 4> nu1;
  FTensor::Index<'c', 4> rho1;
  FTensor::Index<'d', 4> rho2;
  FTensor::Index<'g', 4> alpha1;
  FTensor::Index<'h', 4> beta1;
  FTensor::Index<'i', 4> alpha2;
  FTensor::Index<'j', 4> beta2;
  FTensor::Index<'k', 4> mu2;
  FTensor::Index<'l', 4> nu2;

  // DEDUCE subprocess
  std::string SPINMODE;

  if (lts.decaytree[0].p.spinX2 == 0 && lts.decaytree[1].p.spinX2 == 0) {
    SPINMODE = "2xS";
  } else if (lts.decaytree[0].p.spinX2 == 1 && lts.decaytree[1].p.spinX2 == 1) {
    SPINMODE = "2xF";
  } else if (lts.decaytree[0].p.spinX2 == 2 && lts.decaytree[1].p.spinX2 == 2) {
    SPINMODE = "2xV";
  } else {
    throw std::invalid_argument(
        "MTensorPomeron::ME4: Invalid daughter spin (J = 0, 1/2, 1 pairs supported)");
  }


  // Two helicity states for incoming and outgoing protons
  FOR_PP_HELICITY;
  // Apply proton leg helicity conservation / No helicity flip
  // (high energy limit)
  // This gives at least 4 x speed improvement
  if (ha != h1 || hb != h2) { continue; }

  // ==============================================================
  // Lepton or quark pair via two photon fusion
  if (SPINMODE == "2xF" && std::abs(lts.decaytree[0].p.pdg) <= 15) {
    // Full proton-gamma-proton spinor structure (upper and lower vertex)
    const Tensor1<std::complex<double>, 4> iG_1 = iG_ypp(p1, pa, ubar_1[h1], u_a[ha]);
    const Tensor1<std::complex<double>, 4> iG_2 = iG_ypp(p2, pb, ubar_2[h2], u_b[hb]);

    // Photon propagators
    const Tensor2<std::complex<double>, 4, 4> iD_1 = iD_y(lts.t1);
    const Tensor2<std::complex<double>, 4, 4> iD_2 = iD_y(lts.t2);

    // Fermion propagator
    const MMatrix<std::complex<double>> iSF_t = iD_F(pt, M_);
    const MMatrix<std::complex<double>> iSF_u = iD_F(pu, M_);

    // Central spinors (2 helicities)
    const std::array<std::vector<std::complex<double>>, 2> v_3    = SpinorStates(p3, "v");
    const std::array<std::vector<std::complex<double>>, 2> ubar_4 = SpinorStates(p4, "ubar");


    double FACTOR = 1.0;
    // ------------------------------------------------------------------
    // quark pair (charge 1/3 or 2/3), apply charge and color factors
    if (std::abs(lts.decaytree[0].p.pdg) <= 6) {  // we have a quark

      const double Q  = lts.decaytree[0].p.chargeX3 / 3.0;
      const double NC = 3.0;                        // quarks come in three colors
      FACTOR          = msqrt(math::pow4(Q) * NC);  // sqrt to "amplitude level"
    }
    // ------------------------------------------------------------------

    for (const auto &h3 : indices(v_3)) {
      for (const auto &h4 : indices(ubar_4)) {
        // Vertex functions
        const Tensor2<std::complex<double>, 4, 4> iyFy_t = iG_yeebary(ubar_4[h4], iSF_t, v_3[h3]);
        const Tensor2<std::complex<double>, 4, 4> iyFy_u = iG_yeebary(ubar_4[h4], iSF_u, v_3[h3]);

        // Full amplitude: t-channel + u_channel
        std::complex<double> M_tu = (-zi) * iG_1(mu1) * iD_1(mu1, nu1) *
                                    (iyFy_t(nu2, nu1) + iyFy_u(nu1, nu2)) * iD_2(nu2, mu2) *
                                    iG_2(mu2);

        M_tu *= FACTOR;

        lts.hamp.push_back(M_tu);
      }
    }

    continue;  // skip parts below, only for Pomeron amplitudes
  }

  // ------------------------------------------------------------------
  // ------------------------------------------------------------------

  // Full proton-Pomeron-proton spinor structure (upper and lower vertex)
  /*
  const Tensor2<std::complex<double>,4,4> iG_1a = iG_Ppp(p1, pa, ubar_1[h1], u_a[ha]);
  const Tensor2<std::complex<double>,4,4> iG_2b = iG_Ppp(p2, pb, ubar_2[h2], u_b[hb]);
  */

  // ==============================================================
  // 2 x pseudoscalar (pion pair, kaon pair ...)
  if (SPINMODE == "2xS") {
    double gP = 0.0;

    // Pion or Kaon coupling
    gP = (lts.decaytree[0].p.mass < 0.2) ? Param.gPpipi : Param.gPKK;

    // t-channel blocks
    const Tensor2<std::complex<double>, 4, 4> iG_ta   = iG_Ppsps(pt, -p3, gP);
    const std::complex<double>                iDMES_t = iD_MES0(pt, M_);
    const Tensor2<std::complex<double>, 4, 4> iG_tb   = iG_Ppsps(p4, pt, gP);

    // u-channel blocks
    const Tensor2<std::complex<double>, 4, 4> iG_ua   = iG_Ppsps(p4, pu, gP);
    const std::complex<double>                iDMES_u = iD_MES0(pu, M_);
    const Tensor2<std::complex<double>, 4, 4> iG_ub   = iG_Ppsps(pu, -p3, gP);

    std::complex<double> M_t;
    std::complex<double> M_u;
    // t-channel
    {
      M_t = iG_1a(mu1, nu1) * iDP_13(mu1, nu1, alpha1, beta1) * iG_ta(alpha1, beta1) *
            iG_tb(alpha2, beta2) * iDP_24(alpha2, beta2, mu2, nu2) * iG_2b(mu2, nu2);
      // Apply propagator and off-shell form factors (two of them)
      M_t *= iDMES_t * pow2(PARAM_REGGE::Meson_FF(pt.M2(), pow2(M_)));
    }

    // u-channel
    {
      M_u = iG_1a(mu1, nu1) * iDP_14(mu1, nu1, alpha1, beta1) * iG_ua(alpha1, beta1) *
            iG_ub(alpha2, beta2) * iDP_23(alpha2, beta2, mu2, nu2) * iG_2b(mu2, nu2);
      // Apply propagator and off-shell form factors (two of them)
      M_u *= iDMES_u * pow2(PARAM_REGGE::Meson_FF(pu.M2(), pow2(M_)));
    }

    // Full amplitude: iM = [ ... ]  <-> M = (-i)*[ ... ]
    const std::complex<double> M = (-zi) * (M_t + M_u);
    lts.hamp.push_back(M);
  }

  // ==============================================================
  // 2 x fermion (proton-antiproton pair, lambda pair ...)
  else if (SPINMODE == "2xF") {
    // Fermion propagator
    const MMatrix<std::complex<double>> iSF_t = iD_F(pt, M_);
    const MMatrix<std::complex<double>> iSF_u = iD_F(pu, M_);

    // Central spinors (2 helicities)
    const std::array<std::vector<std::complex<double>>, 2> v_3    = SpinorStates(p3, "v");
    const std::array<std::vector<std::complex<double>>, 2> ubar_4 = SpinorStates(p4, "ubar");

    // Central fermion helicities
    for (const auto &h3 : indices(v_3)) {
      for (const auto &h4 : indices(ubar_4)) {
        // Fermion propagator and connected parts
        const Tensor4<std::complex<double>, 4, 4, 4, 4> iG_t =
            iG_PppbarP(p4, ubar_4[h4], pt, iSF_t, v_3[h3], -p3);
        const Tensor4<std::complex<double>, 4, 4, 4, 4> iG_u =
            iG_PppbarP(p4, ubar_4[h4], pu, iSF_u, v_3[h3], -p3);

        // t-channel
        std::complex<double> M_t = iG_1a(mu1, nu1) * iDP_13(mu1, nu1, alpha1, beta1) *
                                   iG_t(alpha2, beta2, alpha1, beta1) *
                                   iDP_24(alpha2, beta2, mu2, nu2) * iG_2b(mu2, nu2);
        // Apply off-shell form factors (two of them)
        M_t *= pow2(PARAM_REGGE::Baryon_FF(pt.M2(), pow2(M_)));

        // u-channel
        std::complex<double> M_u = iG_1a(mu1, nu1) * iDP_14(mu1, nu1, alpha1, beta1) *
                                   iG_u(alpha1, beta1, alpha2, beta2) *
                                   iDP_23(alpha2, beta2, mu2, nu2) * iG_2b(mu2, nu2);
        // Apply off-shell form factors (two of them)
        M_u *= pow2(PARAM_REGGE::Baryon_FF(pu.M2(), pow2(M_)));

        // Full amplitude: iM = [ ... ]  <-> M = (-i)*[ ... ]
        const std::complex<double> M = (-zi) * (M_t + M_u);
        lts.hamp.push_back(M);
      }
    }
  }

  // ==============================================================
  // 2 x vector meson (rho pair, phi pair ...)
  else if (SPINMODE == "2xV") {
    const int pdg = lts.decaytree[0].p.pdg;

    double g1 = 0.0;
    double g2 = 0.0;

    // Couplings (rho770 meson)
    if (pdg == 113) {
      g1 = Param.gPrhorho[0];
      g2 = Param.gPrhorho[1];
    }
    // Couplings (phi1020 meson)
    else if (pdg == 333) {
      g1 = Param.gPphiphi[0];
      g2 = Param.gPphiphi[1];
    }

    FTensor::Index<'e', 4> rho3;
    FTensor::Index<'f', 4> rho4;

    // t-channel blocks
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iG_tA = iG_Pvv(pt, -p3, g1, g2);
    const Tensor2<std::complex<double>, 4, 4>       iDV_t = iD_V(pt, M_, lts.pfinal[0].M2());
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iG_tB = iG_Pvv(p4, pt, g1, g2);

    // u-channel blocks
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iG_uA = iG_Pvv(p4, pu, g1, g2);
    const Tensor2<std::complex<double>, 4, 4>       iDV_u = iD_V(pu, M_, lts.pfinal[0].M2());
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iG_uB = iG_Pvv(pu, -p3, g1, g2);

    // -------------------------------------------------------------------------
    // TENSOR LORENTZ INDEX CONTRACTION BLOCK
    // This is done in pieces, due to template <>
    // argument deduction constraints

    Tensor2<std::complex<double>, 4, 4> M_t;
    Tensor2<std::complex<double>, 4, 4> M_u;

    // t-channel
    {
      // Upper block
      Tensor2<std::complex<double>, 4, 4> A;
      A(rho1, rho3) =
          iG_1a(mu1, nu1) * iDP_13(mu1, nu1, alpha1, beta1) * iG_tA(rho1, rho3, alpha1, beta1);
      // Lower block
      Tensor2<std::complex<double>, 4, 4> B;
      B(rho4, rho2) =
          iG_2b(mu2, nu2) * iDP_24(alpha2, beta2, mu2, nu2) * iG_tB(rho4, rho2, alpha2, beta2);
      // Apply off-shell form factors (two of them)
      M_t(rho3, rho4) = A(rho1, rho3) * iDV_t(rho1, rho2) * B(rho4, rho2) *
                        pow2(PARAM_REGGE::Meson_FF(pt.M2(), pow2(M_)));
    }

    // u-channel
    {
      // Upper block
      Tensor2<std::complex<double>, 4, 4> A;
      A(rho4, rho1) =
          iG_1a(mu1, nu1) * iDP_14(mu1, nu1, alpha1, beta1) * iG_uA(rho4, rho1, alpha1, beta1);
      // Lower block
      Tensor2<std::complex<double>, 4, 4> B;
      B(rho2, rho3) =
          iG_2b(mu2, nu2) * iDP_23(alpha2, beta2, mu2, nu2) * iG_uB(rho2, rho3, alpha2, beta2);
      // Apply off-shell form factors (two of them)
      M_u(rho3, rho4) = A(rho4, rho1) * iDV_u(rho1, rho2) * B(rho2, rho3) *
                        pow2(PARAM_REGGE::Meson_FF(pu.M2(), pow2(M_)));
    }

    // Total amplitude: iM = [ ... ]  <-> M = (-i)*[ ... ]
    Tensor2<std::complex<double>, 4, 4> M;
    FOR_EACH_2(LI);
    M(u, v) = (-zi) * (M_t(u, v) + M_u(u, v));
    FOR_EACH_2_END;

    // Calculate helicity amplitudes

    // Switch both Lorentz indices up
    // M(alpha1, beta1) = M(mu1, nu1) * gT(mu1, alpha1) * gT(nu1, beta1);
    const std::vector<std::complex<double>> helamp = MassiveSpin1PolSum(M, p3, p4);
    lts.hamp.insert(lts.hamp.end(), helamp.begin(), helamp.end());
  }  // 2xV

  FOR_PP_HELICITY_END;

  // Get total amplitude squared 1/4 \sum_h |A_h|^2
  double SumAmp2 = 0.0;
  for (const auto &i : indices(lts.hamp)) { SumAmp2 += math::abs2(lts.hamp[i]); }
  SumAmp2 /= 4;  // Initial state helicity average

  return SumAmp2;  // Amplitude squared
}


// [!!! THIS FUNCTION UNDER DEVELOPMENT >>>]
//
// 2 -> 6 amplitudes
//
// return value: matrix element squared with helicities summed over
// lts.hamp:     individual helicity amplitudes
//
double MTensorPomeron::ME6(gra::LORENTZSCALAR &lts) const {
  // Free Lorentz indices [second parameter denotes the range of index]
  FTensor::Index<'a', 4> mu1;
  FTensor::Index<'b', 4> nu1;
  FTensor::Index<'c', 4> rho1;
  FTensor::Index<'d', 4> rho2;
  FTensor::Index<'g', 4> alpha1;
  FTensor::Index<'h', 4> beta1;
  FTensor::Index<'i', 4> alpha2;
  FTensor::Index<'j', 4> beta2;
  FTensor::Index<'k', 4> mu2;
  FTensor::Index<'l', 4> nu2;

  // Kinematics
  const M4Vec pa = lts.pbeam1;
  const M4Vec pb = lts.pbeam2;

  const M4Vec p1 = lts.pfinal[1];
  const M4Vec p2 = lts.pfinal[2];

  std::vector<M4Vec> pf(4);

  // Intermediate boson/fermion mass
  double M_ = 0.0;

  if (lts.decaytree.size() == 2) {
    pf[0] = lts.decaytree[0].legs[0].p4;
    pf[1] = lts.decaytree[0].legs[1].p4;
    pf[2] = lts.decaytree[1].legs[0].p4;
    pf[3] = lts.decaytree[1].legs[1].p4;

    M_ = lts.decaytree[0].legs[0].p.mass;
  }
  if (lts.decaytree.size() == 4) {
    pf[0] = lts.decaytree[0].p4;
    pf[1] = lts.decaytree[1].p4;
    pf[2] = lts.decaytree[2].p4;
    pf[3] = lts.decaytree[3].p4;

    M_ = lts.decaytree[0].p.mass;
  }

  // ------------------------------------------------------------------

  // Incoming and outgoing proton spinors (2 helicities)
  const std::array<std::vector<std::complex<double>>, 2> u_a = SpinorStates(pa, "u");
  const std::array<std::vector<std::complex<double>>, 2> u_b = SpinorStates(pb, "u");

  const std::array<std::vector<std::complex<double>>, 2> ubar_1 = SpinorStates(p1, "ubar");
  const std::array<std::vector<std::complex<double>>, 2> ubar_2 = SpinorStates(p2, "ubar");

  // ------------------------------------------------------------------

  // Reset
  lts.hamp.clear();

  // High Energy Limit proton-Pomeron-proton spinor structure
  const Tensor2<std::complex<double>, 4, 4> iG_1a = iG_PppHE(p1, pa);
  const Tensor2<std::complex<double>, 4, 4> iG_2b = iG_PppHE(p2, pb);

  int PDG = 0;
  if (lts.decaytree.size() == 2) { PDG = lts.decaytree[0].p.pdg; }
  if (lts.decaytree.size() == 4) {
    // rho->pipi or phi->KK, based on daughters
    PDG = (lts.decaytree[0].p.mass) < 0.2 ? 113 : 333;
  }

  double g1 = 0.0;
  double g2 = 0.0;

  // Couplings (rho770 meson)
  if (PDG == 113) {
    g1 = Param.gPrhorho[0];
    g2 = Param.gPrhorho[1];
  }
  // Couplings (phi1020 meson)
  else if (PDG == 333) {
    g1 = Param.gPphiphi[0];
    g2 = Param.gPphiphi[1];
  }

  FTensor::Index<'e', 4> rho3;
  FTensor::Index<'f', 4> rho4;

  // Two helicity states for incoming and outgoing protons
  FOR_PP_HELICITY;

  // Apply proton leg helicity conservation / No helicity flip (high energy limit)
  // This gives at least 4 x speed improvement
  if (ha != h1 || hb != h2) { continue; }

  std::complex<double> amp = 0;

  // Different permutations
  const MMatrix<int> R = {{0, 1, 2, 3}, {0, 2, 1, 3}, {0, 3, 2, 1}, {2, 1, 0, 3}};

  for (std::size_t pind = 0; pind < 1; ++pind) {
    // ------------------------------------------------------------------

    // Full proton-Pomeron-proton spinor structure (upper and lower vertex)
    /*
    const Tensor2<std::complex<double>,4,4> iG_1a = iG_Ppp(p1, pa, ubar_1[h1], u_a[ha]);
    const Tensor2<std::complex<double>,4,4> iG_2b = iG_Ppp(p2, pb, ubar_2[h2], u_b[hb]);
    */

    const M4Vec p3 = pf[R[pind][0]] + pf[R[pind][1]];
    const M4Vec p4 = pf[R[pind][2]] + pf[R[pind][3]];

    // Momentum convention of sub-diagrams
    //
    // ------<------ anti-particle p3
    // |
    // | \hat{t} (arrow down)
    // |
    // ------>------ particle p4
    //
    // ------<------ particle p4
    // |
    // | \hat{u} (arrow up)
    // |
    // ------>------ anti-particle p3
    //
    const M4Vec pt = pa - p1 - p3;  // => q1 = pt + p3, q2 = pt - p4
    const M4Vec pu = p4 - pa + p1;  // => q2 = pu + p3, q1 = pu - p3


    const double s13 = (lts.pfinal[1] + p3).M2();
    const double s24 = (lts.pfinal[2] + p4).M2();
    const double s14 = (lts.pfinal[1] + p4).M2();
    const double s23 = (lts.pfinal[2] + p3).M2();


    // t-channel Pomeron propagators
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iDP_13 = iD_P(s13, lts.t1);
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iDP_24 = iD_P(s24, lts.t2);

    // u-channel Pomeron propagators
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iDP_14 = iD_P(s14, lts.t1);
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iDP_23 = iD_P(s23, lts.t2);


    // t-channel blocks
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iG_tA = iG_Pvv(pt, -p3, g1, g2);
    const Tensor2<std::complex<double>, 4, 4>       iDV_t = iD_V(pt, M_, lts.pfinal[0].M2());
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iG_tB = iG_Pvv(p4, pt, g1, g2);

    // u-channel blocks
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iG_uA = iG_Pvv(p4, pu, g1, g2);
    const Tensor2<std::complex<double>, 4, 4>       iDV_u = iD_V(pu, M_, lts.pfinal[0].M2());
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iG_uB = iG_Pvv(pu, -p3, g1, g2);


    // -------------------------------------------------------------------------
    // TENSOR LORENTZ INDEX CONTRACTION BLOCK
    // This is done in pieces, due to template <>
    // argument deduction constraints
    Tensor2<std::complex<double>, 4, 4> M_t;
    Tensor2<std::complex<double>, 4, 4> M_u;

    // t-channel
    {
      // Upper block
      Tensor2<std::complex<double>, 4, 4> A;
      A(rho1, rho3) =
          iG_1a(mu1, nu1) * iDP_13(mu1, nu1, alpha1, beta1) * iG_tA(rho1, rho3, alpha1, beta1);
      // Lower block
      Tensor2<std::complex<double>, 4, 4> B;
      B(rho4, rho2) =
          iG_2b(mu2, nu2) * iDP_24(alpha2, beta2, mu2, nu2) * iG_tB(rho4, rho2, alpha2, beta2);
      // Apply off-shell form factors (two of them)
      M_t(rho3, rho4) = A(rho1, rho3) * iDV_t(rho1, rho2) * B(rho4, rho2) *
                        pow2(PARAM_REGGE::Meson_FF(pt.M2(), pow2(M_)));
    }

    // u-channel
    {
      // Upper block
      Tensor2<std::complex<double>, 4, 4> A;
      A(rho4, rho1) =
          iG_1a(mu1, nu1) * iDP_14(mu1, nu1, alpha1, beta1) * iG_uA(rho4, rho1, alpha1, beta1);
      // Lower block
      Tensor2<std::complex<double>, 4, 4> B;
      B(rho2, rho3) =
          iG_2b(mu2, nu2) * iDP_23(alpha2, beta2, mu2, nu2) * iG_uB(rho2, rho3, alpha2, beta2);
      // Apply off-shell form factors (two of them)
      M_u(rho3, rho4) = A(rho4, rho1) * iDV_u(rho1, rho2) * B(rho2, rho3) *
                        pow2(PARAM_REGGE::Meson_FF(pu.M2(), pow2(M_)));
    }

    // Vector -> PS Decay block
    Tensor2<std::complex<double>, 4, 4> DECAY =
        iG_vv2psps({pf[R[pind][0]], pf[R[pind][1]], pf[R[pind][2]], pf[R[pind][3]]}, PDG);

    // Add
    amp += (-zi) * (M_t(rho3, rho4) + M_u(rho3, rho4)) * DECAY(rho3, rho4);

  }  // Permutations

  lts.hamp.push_back(amp);
  FOR_PP_HELICITY_END;

  // Get total amplitude squared 1/4 \sum_h |A_h|^2
  double SumAmp2 = 0.0;
  for (const auto &i : indices(lts.hamp)) { SumAmp2 += math::abs2(lts.hamp[i]); }
  SumAmp2 /= 4;  // Initial state helicity average

  // --------------------------------------------------------------------
  // *** CONTROL CASCADE SAMPLING ***
  lts.FORCE_FLATMASS2 = true;
  lts.FORCE_OFFSHELL  = 3.0;

  lts.decaytree[0].PS_active = true;
  lts.decaytree[1].PS_active = true;
  // --------------------------------------------------------------------

  return SumAmp2;  // Amplitude squared
}
// [<<< THIS FUNCTION UNDER DEVELOPMENT !!!]


// External massless spin-1 polarization sum for outgoing states
//
// Input tensor M_{\mu \nu \kappa \rho} (indices down)
//
std::vector<Tensor2<std::complex<double>, 4, 4>> MTensorPomeron::MasslessSpin1PolSum(
    const Tensor4<std::complex<double>, 4, 4, 4, 4> &M, const M4Vec &p3, const M4Vec &p4) const {
  FTensor::Index<'a', 4> rho3;
  FTensor::Index<'b', 4> rho4;
  FTensor::Index<'c', 4> mu3;
  FTensor::Index<'d', 4> mu4;

  std::vector<Tensor2<std::complex<double>, 4, 4>> hamp;
  Tensor2<std::complex<double>, 4, 4>              temp;

  // Massless polarization vectors (2 helicities)
  const bool                                            INDEX_UP = true;
  const std::array<Tensor1<std::complex<double>, 4>, 2> eps_3_conj =
      MasslessSpin1States(p3, "conj", INDEX_UP);
  const std::array<Tensor1<std::complex<double>, 4>, 2> eps_4_conj =
      MasslessSpin1States(p4, "conj", INDEX_UP);

  // Loop over states
  for (const auto &h3 : indices(eps_3_conj)) {
    for (const auto &h4 : indices(eps_4_conj)) {
      // Autocontraction
      temp(mu3, mu4) = eps_3_conj[h3](rho3) * eps_4_conj[h4](rho4) * M(rho3, rho4, mu3, mu4);
      hamp.push_back(temp);
    }
  }
  return hamp;
}


// External massless spin-1 polarization sum for outgoing states
//
// Input tensor M_{\mu \nu} (indices down)
//
std::vector<std::complex<double>> MTensorPomeron::MasslessSpin1PolSum(
    const Tensor2<std::complex<double>, 4, 4> &M, const M4Vec &p3, const M4Vec &p4) const {
  FTensor::Index<'a', 4>            rho3;
  FTensor::Index<'b', 4>            rho4;
  std::vector<std::complex<double>> hamp;

  // Massless polarization vectors (2 helicities)
  const bool                                            INDEX_UP = true;
  const std::array<Tensor1<std::complex<double>, 4>, 2> eps_3_conj =
      MasslessSpin1States(p3, "conj", INDEX_UP);
  const std::array<Tensor1<std::complex<double>, 4>, 2> eps_4_conj =
      MasslessSpin1States(p4, "conj", INDEX_UP);

  // Loop over massive Spin-1 helicity states
  for (const auto &h3 : indices(eps_3_conj)) {
    for (const auto &h4 : indices(eps_4_conj)) {
      // Autocontraction
      const std::complex<double> amp = eps_3_conj[h3](rho3) * eps_4_conj[h4](rho4) * M(rho3, rho4);
      hamp.push_back(amp);
    }
  }
  return hamp;
}


// External massive spin-1 (vector) polarization sum for outgoing states
//
// Input tensor M_{\mu \nu \kappa \rho} (indices down)
//
std::vector<Tensor2<std::complex<double>, 4, 4>> MTensorPomeron::MassiveSpin1PolSum(
    const Tensor4<std::complex<double>, 4, 4, 4, 4> &M, const M4Vec &p3, const M4Vec &p4) const {
  FTensor::Index<'a', 4> rho3;
  FTensor::Index<'b', 4> rho4;
  FTensor::Index<'c', 4> mu3;
  FTensor::Index<'d', 4> mu4;

  std::vector<Tensor2<std::complex<double>, 4, 4>> hamp;
  Tensor2<std::complex<double>, 4, 4>              temp;

  // Massive polarization vectors (3 helicities)
  const bool                                            INDEX_UP = true;
  const std::array<Tensor1<std::complex<double>, 4>, 3> eps_3_conj =
      MassiveSpin1States(p3, "conj", INDEX_UP);
  const std::array<Tensor1<std::complex<double>, 4>, 3> eps_4_conj =
      MassiveSpin1States(p4, "conj", INDEX_UP);

  // Loop over states
  for (const auto &h3 : indices(eps_3_conj)) {
    for (const auto &h4 : indices(eps_4_conj)) {
      // Autocontraction
      temp(mu3, mu4) = eps_3_conj[h3](rho3) * eps_4_conj[h4](rho4) * M(rho3, rho4, mu3, mu4);
      hamp.push_back(temp);
    }
  }
  return hamp;
}


// External massive spin-1 (vector) polarization sum for outgoing states
//
// Input tensor M_{\mu \nu \kappa \rho} (indices down)
//
std::vector<std::complex<double>> MTensorPomeron::MassiveSpin1PolSum(
    const Tensor2<std::complex<double>, 4, 4> &M, const M4Vec &p3, const M4Vec &p4) const {
  FTensor::Index<'a', 4>            rho3;
  FTensor::Index<'b', 4>            rho4;
  std::vector<std::complex<double>> hamp;

  // Massive polarization vectors (3 helicities)
  const bool                                            INDEX_UP = true;
  const std::array<Tensor1<std::complex<double>, 4>, 3> eps_3_conj =
      MassiveSpin1States(p3, "conj", INDEX_UP);
  const std::array<Tensor1<std::complex<double>, 4>, 3> eps_4_conj =
      MassiveSpin1States(p4, "conj", INDEX_UP);

  // Loop over massive Spin-1 helicity states
  for (const auto &h3 : indices(eps_3_conj)) {
    for (const auto &h4 : indices(eps_4_conj)) {
      // Autocontraction
      const std::complex<double> amp = eps_3_conj[h3](rho3) * eps_4_conj[h4](rho4) * M(rho3, rho4);
      hamp.push_back(amp);
    }
  }

  /*
  // Polarization sum (completeness relation applied)
  // already simplified expression => second terms vanishes and leaves only g[u][k] * g[v][l]
  double Amp2 = 0.0;

  FOR_EACH_4(LI);
  const std::complex<double> contract = std::conj(M(u, v)) * M(k, l) * g[u][k] * g[v][l];
  Amp2 += std::real(contract);     // real for casting to double
  FOR_EACH_4_END;

  hamp.push_back(msqrt(Amp2)); // Phase information lost here
  */

  return hamp;
}


// 2xvector -> 2xpseudoscalar decay block
//
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iG_vv2psps(const std::vector<M4Vec> &p,
                                                               int PDG) const {
  FTensor::Index<'e', 4> rho3;
  FTensor::Index<'f', 4> rho4;
  FTensor::Index<'g', 4> kappa3;
  FTensor::Index<'h', 4> kappa4;

  double g1 = 0.0;
  double M  = 0.0;
  double W  = 0.0;
  if (PDG == 113) {         // rho(770)0
    const double BR = 1.0;  // pi+pi-
    M               = 7.7526E-01;
    W               = 1.491E-01;
    g1              = GDecay(1, M, W, 1.3957061E-01, BR);
  } else if (PDG == 333) {    // phi(1020)0
    const double BR = 0.489;  // K+K-
    M               = 1.019461E+00;
    W               = 4.249E-03;
    g1              = GDecay(1, M, W, 4.93677E-01, BR);
  } else {
    throw std::invalid_argument("MTensorPomeron::iG_vv2psps: Unsupported vector meson PDG = " +
                                std::to_string(PDG));
  }

  // Vector propagators
  const bool                                INDEX_UP = true;
  const Tensor2<std::complex<double>, 4, 4> iD_V1    = iD_VMES(p[0] + p[1], M, W, INDEX_UP);
  const Tensor2<std::complex<double>, 4, 4> iD_V2    = iD_VMES(p[2] + p[3], M, W, INDEX_UP);

  // Decay couplings
  const Tensor1<std::complex<double>, 4> iG_D1 = iG_vpsps(p[0], p[1], M, g1);
  const Tensor1<std::complex<double>, 4> iG_D2 = iG_vpsps(p[2], p[3], M, g1);

  Tensor2<std::complex<double>, 4, 4> BLOCK;
  BLOCK(rho3, rho4) = iD_V1(rho3, kappa3) * iG_D1(kappa3) * iD_V2(rho4, kappa4) * iG_D2(kappa4);

  return BLOCK;
}


// -------------------------------------------------------------------------
// Vertex functions

// Gamma-Lepton-Lepton vertex
// contracted in \bar{spinor} G_\mu \bar{spinor}
//
// iGamma_\mu (p',p)
//
// High Energy Limit:
// ubar(p',\lambda') \gamma_\mu u(p,\lambda ~= (p' + p)_\mu
// \delta_{\lambda',\lambda}
//
// Input as contravariant (upper index) 4-vectors
//
Tensor1<std::complex<double>, 4> MTensorPomeron::iG_yee(
    const M4Vec &prime, const M4Vec &p, const std::vector<std::complex<double>> &ubar,
    const std::vector<std::complex<double>> &u) const {
  // const double q2 = (prime-p).M2();
  const double e = msqrt(qed::alpha_QED() * 4.0 * PI);  // ~ 0.3, no running

  Tensor1<std::complex<double>, 4> T;
  for (const auto &mu : LI) {
    // \bar{spinor} [Gamma Matrix] \spinor product
    T(mu) = zi * e * matoper::VecMatVecMultiply(ubar, gamma_lo[mu], u);
  }
  return T;
}

// Gamma-Proton-Proton vertex iGamma_\mu (p', p)
// contracted in \bar{spinor} iG_\mu \bar{spinor}
//
// Input as contravariant (upper index) 4-vectors
//
Tensor1<std::complex<double>, 4> MTensorPomeron::iG_ypp(
    const M4Vec &prime, const M4Vec &p, const std::vector<std::complex<double>> &ubar,
    const std::vector<std::complex<double>> &u) const {
  const double t    = (prime - p).M2();
  const double e    = msqrt(qed::alpha_QED() * 4.0 * PI);  // ~ 0.3, no running
  const M4Vec  psum = prime - p;

  Tensor1<std::complex<double>, 4> T;
  for (const auto &mu : LI) {
    MMatrix<std::complex<double>> SUM(4, 4, 0.0);
    for (const auto &nu : LI) { SUM += sigma_lo[mu][nu] * psum[nu]; }
    const MMatrix<std::complex<double>> A = gamma_lo[mu] * F1(t);
    const MMatrix<std::complex<double>> B = SUM * zi / (2 * PDG::mp) * F2(t);
    const MMatrix<std::complex<double>> G = (A + B) * (-zi * e);

    // \bar{spinor} [Gamma Matrix] \spinor product
    T(mu) = matoper::VecMatVecMultiply(ubar, G, u);
  }

  return T;
}

// gamma - electron - fermion propagator - positron - gamma vertex
// iGamma_{\mu \nu}
//
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iG_yeebary(
    const std::vector<std::complex<double>> &ubar, const MMatrix<std::complex<double>> &iSF,
    const std::vector<std::complex<double>> &v) const {
  const double               e      = msqrt(qed::alpha_QED() * 4.0 * PI);  // ~ 0.3, no running
  const std::complex<double> vertex = zi * e;

  Tensor2<std::complex<double>, 4, 4> T;
  for (const auto &mu : LI) {
    const std::vector<std::complex<double>> lhs = matoper::VecMatMultiply(ubar, gamma_lo[mu] * iSF);

    for (const auto &nu : LI) {
      T(mu, nu) = vertex * matoper::VecMatVecMultiply(lhs, gamma_lo[nu], v) * vertex;
    }
  }
  return T;
}


// High-Energy limit proton-Pomeron-proton vertex times \delta^{lambda_prime,
// \lambda} (helicity conservation)
// (~x 2 faster evaluation than the exact spinor structure)
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iG_PppHE(const M4Vec &prime,
                                                             const M4Vec  p) const {
  const M4Vec                psum   = prime + p;
  const std::complex<double> FACTOR = -zi * 3.0 * Param.gPNN * F1((prime - p).M2());

  Tensor2<std::complex<double>, 4, 4> T;
  FOR_EACH_2(LI);
  T(u, v) = FACTOR * (psum % u) * (psum % v);
  FOR_EACH_2_END;

  return T;
}

// Pomeron-Proton-Proton [-Neutron-Neutron, -Antiproton-Antiproton] vertex
// contracted in \bar{spinor} G_{\mu\nu} \bar{spinor}
//
// i\Gamma_{\mu\nu} (p', p)
//
// Input as contravariant (upper index) 4-vectors
//
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iG_Ppp(
    const M4Vec &prime, const M4Vec &p, const std::vector<std::complex<double>> &ubar,
    const std::vector<std::complex<double>> &u) const {
  const double t    = (prime - p).M2();
  const M4Vec  psum = prime + p;

  Tensor2<std::complex<double>, 4, 4> T;
  const std::complex<double>          FACTOR = -zi * 3.0 * Param.gPNN * F1(t);

  // Feynman slash
  const MMatrix<std::complex<double>> slash = FSlash(psum);

  // Aux matrix
  MMatrix<std::complex<double>> A;

  for (const auto &mu : LI) {
    for (const auto &nu : LI) {
      A = (gamma_lo[mu] * (psum % nu) + gamma_lo[nu] * (psum % mu)) * 0.5;
      if (mu == nu) { A = A - slash * 0.25 * g[mu][nu]; }

      // \bar{spinor} [Gamma Matrix] \spinor product
      T(mu, nu) = FACTOR * gra::matoper::VecMatVecMultiply(ubar, A, u);
    }
  }
  return T;
}

// Pomeron x outgoing proton spinor ubar x
//           <antiproton/proton fermion propagator>
//         x outgoing antiproton spinor v x Pomeron vertex function
//
// iG_{\mu_2\nu_2\mu_1\nu_1}
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::iG_PppbarP(
    const M4Vec &prime, const std::vector<std::complex<double>> &ubar, const M4Vec &pt,
    const MMatrix<std::complex<double>> &iSF, const std::vector<std::complex<double>> &v,
    const M4Vec &p) const {
  const std::complex<double> lhs_FACTOR = -zi * 3.0 * Param.gPNN * F1((prime - pt).M2());
  const std::complex<double> rhs_FACTOR = -zi * 3.0 * Param.gPNN * F1((pt - p).M2());

  const M4Vec lhs_psum = prime + pt;
  const M4Vec rhs_psum = pt + p;

  // Feynman slashes
  const MMatrix<std::complex<double>> lhs_slash = FSlash(lhs_psum);
  const MMatrix<std::complex<double>> rhs_slash = FSlash(rhs_psum);

  // Aux matrices
  MMatrix<std::complex<double>> lhs_A;
  MMatrix<std::complex<double>> rhs_A;

  Tensor4<std::complex<double>, 4, 4, 4, 4> T;

  for (const auto &mu2 : LI) {
    for (const auto &nu2 : LI) {
      lhs_A = (gamma_lo[mu2] * (lhs_psum % nu2) + gamma_lo[nu2] * (lhs_psum % mu2)) * 0.5;
      if (mu2 == nu2) { lhs_A = lhs_A - lhs_slash * 0.25 * g[mu2][nu2]; }

      // Fermion propagator applied in the middle
      const std::vector<std::complex<double>> ubarM = matoper::VecMatMultiply(ubar, lhs_A * iSF);

      for (const auto &mu1 : LI) {
        for (const auto &nu1 : LI) {
          rhs_A = (gamma_lo[mu1] * (rhs_psum % nu1) + gamma_lo[nu1] * (rhs_psum % mu1)) * 0.5;
          if (mu1 == nu1) { rhs_A = rhs_A - rhs_slash * 0.25 * g[mu1][nu1]; }

          // \bar{spinor} [Gamma Matrix] \spinor product
          T(mu2, nu2, mu1, nu1) =
              lhs_FACTOR * matoper::VecMatVecMultiply(ubarM, rhs_A, v) * rhs_FACTOR;
        }
      }
    }
  }
  return T;
}


// ======================================================================

// Pomeron (\mu\nu) - Pomeron (\kappa\lambda) - Scalar/Pseudoscalar resonance
// vertex function: i\Gamma_{\mu\nu,\kappa\lambda,\rho\sigma}
//
// Input as contravariant (upper index) 4-vector, M0 the resonance peak mass
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::iG_PPS_total(
    const M4Vec &q1, const M4Vec &q2, double M0, const std::string &mode,
    const std::vector<double> &g_PPS) const {
  // Form FACTOR
  const double LAMBDA  = 1.0;  // GeV
  const double F_tilde = FM(q1.M2(), 1.0) * FM(q2.M2(), 1.0) * F((q1 + q2).M2(), M0, LAMBDA);

  // Tensor structures
  Tensor4<std::complex<double>, 4, 4, 4, 4> T;

  if (mode == "scalar") {
    static const Tensor4<std::complex<double>, 4, 4, 4, 4> iG0 = MTensorPomeron::iG_PPS_0();
    const Tensor4<std::complex<double>, 4, 4, 4, 4>        iG1 =
        MTensorPomeron::iG_PPS_1(q1, q2, g_PPS[1]);

    FOR_EACH_4(LI);
    T(u, v, k, l) = (g_PPS[0] * iG0(u, v, k, l) + iG1(u, v, k, l)) * F_tilde;
    FOR_EACH_4_END;

  } else if (mode == "pseudoscalar") {
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iG0 =
        MTensorPomeron::iG_PPPS_0(q1, q2, g_PPS[0]);
    const Tensor4<std::complex<double>, 4, 4, 4, 4> iG1 =
        MTensorPomeron::iG_PPPS_1(q1, q2, g_PPS[1]);

    FOR_EACH_4(LI);
    T(u, v, k, l) = (iG0(u, v, k, l) + iG1(u, v, k, l)) * F_tilde;
    FOR_EACH_4_END;

  } else {
    throw std::invalid_argument(
        "MTensorPomeron::iG_PPS_total: Unknown mode (should be scalar or pseudoscalar)");
  }

  return T;
}

// Pomeron-Pomeron-Scalar coupling structure #0
// iG_{\mu \nu \kappa \lambda} ~ (l,s) = (0,0)
//
// Input as contravariant (upper index) 4-vectors
//
// Apply coupling g_PPS[0] outside this!
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::iG_PPS_0() const {
  const double               S0     = 1.0;  // Mass scale (GeV)
  const std::complex<double> FACTOR = zi * S0;

  Tensor4<std::complex<double>, 4, 4, 4, 4> T;
  FOR_EACH_4(LI);
  T(u, v, k, l) = FACTOR * (g[u][k] * g[v][l] + g[u][l] * g[v][k] - 0.5 * g[u][v] * g[k][l]);
  FOR_EACH_4_END;

  return T;
}

// Pomeron-Pomeron-Scalar coupling structure #1
// iG_{\mu \nu \kappa \lambda} ~ (l,s) = (2,2)
//
// Input as contravariant (upper index) 4-vectors
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::iG_PPS_1(const M4Vec &q1, const M4Vec &q2,
                                                                   double g_PPS) const {
  const double               S0     = 1.0;  // Mass scale (GeV)
  const double               q1q2   = q1 * q2;
  const std::complex<double> FACTOR = zi * g_PPS / (2 * S0);

  Tensor4<std::complex<double>, 4, 4, 4, 4> T;
  FOR_EACH_4(LI);
  T(u, v, k, l) = FACTOR * ((q1 % k) * (q2 % u) * g[v][l] + (q1 % k) * (q2 % v) * g[u][l] +
                            (q1 % l) * (q2 % u) * g[v][k] + (q1 % l) * (q2 % v) * g[u][k] -
                            2.0 * q1q2 * (g[u][k] * g[v][l] + g[v][k] * g[u][l]));
  FOR_EACH_4_END;

  return T;
}

// Pomeron-Pomeron-Pseudoscalar coupling structure #0
// iG_{\mu \nu \kappa \lambda} ~ (l,s) = (1,1)
//
// Input as contravariant (upper index) 4-vectors
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::iG_PPPS_0(const M4Vec &q1,
                                                                    const M4Vec &q2,
                                                                    double       g_PPPS) const {
  const double S0 = 1.0;  // Mass scale (GeV)

  const M4Vec q1_q2 = q1 - q2;
  const M4Vec q     = q1 + q2;

  const std::complex<double>                FACTOR = zi * g_PPPS / (2.0 * S0);
  Tensor4<std::complex<double>, 4, 4, 4, 4> T;

  FOR_EACH_4(LI);
  T(u, v, k, l) = 0.0;

  // Contract r and s indices
  for (const auto &r : LI) {
    for (const auto &s : LI) {
      // Note +=
      T(u, v, k, l) += FACTOR *
                       (g[u][k] * eps_lo(v, l, r, s) + g[v][k] * eps_lo(u, l, r, s) +
                        g[u][l] * eps_lo(v, k, r, s) + g[v][l] * eps_lo(u, k, r, s)) *
                       q1_q2[r] * q[s];
    }
  }
  FOR_EACH_4_END;

  return T;
}

// Pomeron-Pomeron-Pseudoscalar coupling structure #1
// iG_{\mu \nu \kappa \lambda} ~ (l,s) = (3,3)
//
// Input as contravariant (upper index) 4-vectors
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::iG_PPPS_1(const M4Vec &q1,
                                                                    const M4Vec &q2,
                                                                    double       g_PPPS) const {
  const double               S0     = 1.0;  // Mass scale (GeV)
  const M4Vec                q1_q2  = q1 - q2;
  const M4Vec                q      = q1 + q2;
  const double               q1q2   = q1 * q2;
  const std::complex<double> FACTOR = zi * g_PPPS / gra::math::pow3(S0);

  Tensor4<std::complex<double>, 4, 4, 4, 4> T;

  FOR_EACH_4(LI);
  T(u, v, k, l) = 0.0;

  // Contract r and s indices
  for (const auto &r : LI) {
    for (const auto &s : LI) {
      // Note +=
      T(u, v, k, l) += FACTOR *
                       (eps_lo(v, l, r, s) * ((q1 % k) * (q2 % u) - q1q2 * g[u][k]) +
                        eps_lo(u, l, r, s) * ((q1 % k) * (q2 % v) - q1q2 * g[v][k]) +
                        eps_lo(v, k, r, s) * ((q1 % l) * (q2 % u) - q1q2 * g[u][l]) +
                        eps_lo(u, k, r, s) * ((q1 % l) * (q2 % v) - q1q2 * g[v][l])) *
                       q1_q2[r] * q[s];
    }
  }
  FOR_EACH_4_END;

  return T;
}

// ======================================================================

// Pomeron (\mu\nu) - Pomeron (\kappa\lambda) - Tensor resonance (\rho\sigma)
// vertex function: i\Gamma_{\mu\nu,\kappa\lambda,\rho\sigma}
//
// Input as contravariant (upper index) 4-vector, M0 the resonance peak mass
//
MTensor<std::complex<double>> MTensorPomeron::iG_PPT_total(const M4Vec &q1, const M4Vec &q2,
                                                           double                     M0,
                                                           const std::vector<double> &g_PPT) const {
  // Form FACTOR
  const double LAMBDA  = 1.0;  // GeV
  const double F_tilde = FM(q1.M2(), 1.0) * FM(q2.M2(), 1.0) * F((q1 + q2).M2(), M0, LAMBDA);

  // Kinematics independent tensor structure [0]
  static const MTensor<std::complex<double>> iG0 = iG_PPT_00();

  // Kinematics dependent tensor structures  [1 ... 6]
  std::vector<MTensor<std::complex<double>>> iGx;

  const double EPSILON = 1e-6;
  // For speed, we do not calculate unnecessary here
  if (std::abs(g_PPT[1]) > EPSILON) iGx.push_back(iG_PPT_12(q1, q2, g_PPT[1], 1));

  if (std::abs(g_PPT[2]) > EPSILON) iGx.push_back(iG_PPT_12(q1, q2, g_PPT[2], 2));

  if (std::abs(g_PPT[3]) > EPSILON) iGx.push_back(iG_PPT_03(q1, q2, g_PPT[3]));

  if (std::abs(g_PPT[4]) > EPSILON) iGx.push_back(iG_PPT_04(q1, q2, g_PPT[4]));

  if (std::abs(g_PPT[5]) > EPSILON) iGx.push_back(iG_PPT_05(q1, q2, g_PPT[5]));

  if (std::abs(g_PPT[6]) > EPSILON) iGx.push_back(iG_PPT_06(q1, q2, g_PPT[6]));

  // Construct total vertex by summing up the tensors
  MTensor<std::complex<double>> T = MTensor({4, 4, 4, 4, 4, 4}, std::complex<double>(0.0));

  FOR_EACH_6(LI);

  // Indexing
  const std::vector<size_t> ind = {u, v, k, l, r, s};

  // For number [0], we apply coupling here
  T(ind) = iG0(ind) * g_PPT[0];

  // For number [1-6], couplings have been already applied, just loop over
  for (const auto &i : indices(iGx)) { T(ind) += iGx[i](ind); }

  // Apply form factors
  T(ind) *= F_tilde;
  FOR_EACH_6_END;

  return T;
}

// Pomeron-Pomeron-Tensor coupling structure #0
// iG_{\mu \nu \kappa \lambda \rho \sigma} ~ (l,s) = (0,2)
//
// This is kinematics independent, can be pre-calculated.
//
// Remember to apply g_PPT (coupling constant) outside this
//
MTensor<std::complex<double>> MTensorPomeron::iG_PPT_00() const {
  const double               S0     = 1.0;  // Mass scale (GeV)
  const std::complex<double> FACTOR = 2.0 * zi * S0;

  // Init with zeros!
  MTensor<std::complex<double>> T = MTensor({4, 4, 4, 4, 4, 4}, std::complex<double>(0.0));

  FOR_EACH_6(LI);
  const std::vector<size_t> ind = {u, v, k, l, r, s};

  for (auto const &v1 : LI) {
    for (auto const &a1 : LI) {
      for (auto const &l1 : LI) {
        for (auto const &r1 : LI) {
          for (auto const &s1 : LI) {
            for (auto const &u1 : LI) {
              if (v1 != a1 || l1 != r1 || s1 != u1) { continue; }  // speed it up

              // Note +=
              T(ind) += FACTOR * R_DDDD(u, v, u1, v1) * R_DDDD(k, l, a1, l1) *
                        R_DDDD(r, s, r1, s1) * g[v1][a1] * g[l1][r1] * g[s1][u1];
            }
          }
        }
      }
    }
  }
  FOR_EACH_6_END;

  return T;
}

// Pomeron-Pomeron-Tensor coupling structures #1 and #2
// iG_{\mu \nu \kappa \lambda \rho \sigma}
// ~ (l,s) = (2,0) - (2,2)
// ~ (l,s) = (2,0) + (2,2)
//
MTensor<std::complex<double>> MTensorPomeron::iG_PPT_12(const M4Vec &q1, const M4Vec &q2,
                                                        double g_PPT, int mode) const {
  if (!(mode == 1 || mode == 2)) {
    throw std::invalid_argument("MTensorPomeron::iG_PPT_12: Error, mode should be 1 or 2");
  }
  const double               S0     = 1.0;  // Mass scale (GeV)
  const double               q1q2   = q1 * q2;
  const std::complex<double> FACTOR = -2.0 * zi / S0 * g_PPT;

  // Coupling structure 1 or 2
  const double sign = (mode == 1) ? -1.0 : 1.0;

  // Init with zeros!
  MTensor<std::complex<double>> T = MTensor({4, 4, 4, 4, 4, 4}, std::complex<double>(0.0));
  FOR_EACH_6(LI);
  const std::vector<size_t> ind = {u, v, k, l, r, s};

  // Pre-calculated tensor contraction structures T1,T2,T2 used here
  T(ind) += q1q2 * T1(ind);

  for (auto const &u1 : LI) {
    for (auto const &r1 : LI) {
      // Note +=
      T(ind) += sign * q2[u1] * (q1 % r1) * T2({u, v, k, l, r, s, u1, r1});
    }
  }
  for (auto const &u1 : LI) {
    for (auto const &s1 : LI) {
      // Note +=
      T(ind) += sign * q1[u1] * (q2 % s1) * T3({u, v, k, l, r, s, u1, s1});
    }
  }
  for (auto const &r1 : LI) {
    for (auto const &s1 : LI) {
      // Note +=
      T(ind) += (q1 % r1) * (q2 % s1) * R_DDDD(u, v, k, l) * R_DDUU(r, s, r1, s1);
    }
  }

  T(ind) *= FACTOR;
  FOR_EACH_6_END;

  return T;
}

// Pomeron-Pomeron-Tensor coupling structure #3
// iG_{\mu \nu \kappa \lambda \rho \sigma} ~ (l,s) = (2,4)
//
MTensor<std::complex<double>> MTensorPomeron::iG_PPT_03(const M4Vec &q1, const M4Vec &q2,
                                                        double g_PPT) const {
  const double               S0     = 1.0;  // Mass scale (GeV)
  const std::complex<double> FACTOR = -zi / S0 * g_PPT;

  Tensor3<double, 4, 4, 4> A;
  Tensor3<double, 4, 4, 4> B;
  Tensor3<double, 4, 4, 4> C;
  Tensor3<double, 4, 4, 4> D;

  // Manual contractions
  FOR_EACH_3(LI);
  A(u, v, k) = 0.0;
  B(u, v, k) = 0.0;
  for (auto const &x : LI) {
    // Note +=
    A(u, v, k) += q2[x] * R_DDDD(u, v, x, k);
    B(u, v, k) += q1[x] * R_DDDD(u, v, x, k);
  }
  FOR_EACH_3_END;

  // Final rank-6 tensor
  MTensor<std::complex<double>> T = MTensor({4, 4, 4, 4, 4, 4}, std::complex<double>(0.0));
  FOR_EACH_6(LI);
  const std::vector<size_t> ind = {u, v, k, l, r, s};
  for (auto const &v1 : LI) {
    for (auto const &l1 : LI) {
      // Note +=
      T(ind) += (A(u, v, v1) * B(k, l, l1) + A(u, v, l1) * B(k, l, v1)) * R_UUDD(v1, l1, r, s);
    }
  }
  T(ind) *= FACTOR;
  /*

  // NAIVE LOOP
  for (auto const &v1 : LI) {
    for (auto const &a1 : LI) {
      for (auto const &l1 : LI) {
          for (auto const &u1 : LI) {

            // Note +=
            T(ind) += FACTOR * (q2[u1] * R_DDDD(u,v,u1,v1) * q1[a1] * R_DDDD(k,l,a1,l1) +
                                q2[a1] * R_DDDD(u,v,a1,l1) * q1[u1] * R_DDDD(k,l,u1,v1)) *
  R_UUDD(v1,l1,r,s);
        }
      }
    }
  }
  */
  FOR_EACH_6_END;

  return T;
}

// Pomeron-Pomeron-Tensor coupling structure #4
// iG_{\mu \nu \kappa \lambda \rho \sigma} ~ (l,s) = (4,2)
//
MTensor<std::complex<double>> MTensorPomeron::iG_PPT_04(const M4Vec &q1, const M4Vec &q2,
                                                        double g_PPT) const {
  const double               S0     = 1.0;  // Mass scale (GeV)
  const std::complex<double> FACTOR = -2.0 * zi / math::pow3(S0) * g_PPT;
  const double               q1q2   = q1 * q2;

  Tensor1<double, 4> q1_D = {q1 % 0, q1 % 1, q1 % 2, q1 % 3};
  Tensor1<double, 4> q2_D = {q2 % 0, q2 % 1, q2 % 2, q2 % 3};

  Tensor4<double, 4, 4, 4, 4> A;
  Tensor4<double, 4, 4, 4, 4> B;
  Tensor2<double, 4, 4>       C;

  // Manual contractions (not possible with autocontraction)
  FOR_EACH_4(LI);
  A(u, v, k, l) = 0.0;
  B(u, v, k, l) = 0.0;

  for (auto const &u1 : LI) {
    for (auto const &v1 : LI) {
      for (auto const &a1 : LI) {
        // Note +=
        A(u, v, k, l) += q2[v1] * R_DDDD(u, v, v1, a1) * q1[u1] * R_DDDU(k, l, u1, a1);
        B(u, v, k, l) += q2[u1] * R_DDDD(u, v, u1, a1) * q1[v1] * R_DDDU(k, l, v1, a1);
      }
    }
  }
  FOR_EACH_4_END;

  // Autocontractions
  {
    FTensor::Index<'a', 4> r;
    FTensor::Index<'b', 4> s;
    FTensor::Index<'c', 4> a1;
    FTensor::Index<'d', 4> l1;

    C(r, s) = q1_D(a1) * q2_D(l1) * R_UUDD(a1, l1, r, s);
  }

  // Final rank-6 expression
  MTensor<std::complex<double>> T = MTensor({4, 4, 4, 4, 4, 4}, std::complex<double>(0.0));
  FOR_EACH_6(LI);
  T({u, v, k, l, r, s}) =
      FACTOR * (A(u, v, k, l) + B(u, v, k, l) - 2.0 * q1q2 * R_DDDD(u, v, k, l)) * C(r, s);

  /*
  // NAIVE LOOP
  for (auto const &v1 : LI) {
    for (auto const &a1 : LI) {
      for (auto const &l1 : LI) {
        for (auto const &u1 : LI) {

          // Note +=
          T(ind) += FACTOR * (q2[v1] * R_DDDD(u,v,v1,a1) * q1[u1] * R_DDDU(k,l,u1,a1)
                            + q2[u1] * R_DDDD(u,v,u1,a1) * q1[v1] * R_DDDU(k,l,v1,a1)
                            - 2.0 * q1q2 * R_DDDD(u,v,k,l)) * (q1 % a1) * (q2 % l1) *
  R_UUDD(a1,l1,r,s);
        }
      }
    }
  }
  */
  FOR_EACH_6_END;
  return T;
}

// Pomeron-Pomeron-Tensor coupling structure #05
// iG_{\mu \nu \kappa \lambda \rho \sigma} ~ (l,s) = (4,4)
//
MTensor<std::complex<double>> MTensorPomeron::iG_PPT_05(const M4Vec &q1, const M4Vec &q2,
                                                        double g_PPT) const {
  const double               S0     = 1.0;  // Mass scale (GeV)
  const std::complex<double> FACTOR = zi / math::pow3(S0) * g_PPT;

  Tensor1<double, 4> q1_U = {q1[0], q1[1], q1[2], q1[3]};
  Tensor1<double, 4> q2_U = {q2[0], q2[1], q2[2], q2[3]};
  Tensor1<double, 4> q1_D = {q1 % 0, q1 % 1, q1 % 2, q1 % 3};
  Tensor1<double, 4> q2_D = {q2 % 0, q2 % 1, q2 % 2, q2 % 3};

  Tensor4<double, 4, 4, 4, 4> A;
  Tensor2<double, 4, 4>       B;
  Tensor4<double, 4, 4, 4, 4> C;
  Tensor2<double, 4, 4>       D;

  // Manual contractions (not possible with autocontraction)
  MTensor<std::complex<double>> T = MTensor({4, 4, 4, 4, 4, 4}, std::complex<double>(0.0));
  FOR_EACH_6(LI);
  A(u, v, r, s) = 0.0;
  C(k, l, r, s) = 0.0;

  for (auto const &u1 : LI) {
    for (auto const &v1 : LI) {
      for (auto const &r1 : LI) {
        // Note +=
        A(u, v, r, s) += q2_U(u1) * R_DDDD(u, v, u1, v1) * q2_D(r1) * R_UUDD(v1, r1, r, s);
        C(k, l, r, s) += q1_U(u1) * R_DDDD(k, l, u1, v1) * q1_D(r1) * R_UUDD(v1, r1, r, s);
      }
    }
  }
  FOR_EACH_6_END;

  // Autocontractions
  {
    FTensor::Index<'a', 4> u;
    FTensor::Index<'b', 4> v;
    FTensor::Index<'c', 4> k;
    FTensor::Index<'d', 4> l;
    FTensor::Index<'j', 4> a1;
    FTensor::Index<'k', 4> l1;

    B(k, l) = q1_U(a1) * q1_U(l1) * R_DDDD(k, l, a1, l1);
    D(u, v) = q2_U(a1) * q2_U(l1) * R_DDDD(u, v, a1, l1);
  }

  // Final rank-6 expression
  FOR_EACH_6(LI);
  T({u, v, k, l, r, s}) = FACTOR * (A(u, v, r, s) * B(k, l) + C(k, l, r, s) * D(u, v));

  /*
  // NAIVE LOOP
  for (auto const &v1 : LI) {
    for (auto const &a1 : LI) {
      for (auto const &l1 : LI) {
        for (auto const &u1 : LI) {
          for (auto const &r1 : LI) {

            // Note +=
            T(ind) += FACTOR * (q2[u1] * (q2 % r1) * R_DDDD(u,v,u1,v1) * R_UUDD(v1,r1,r,s) * q1[a1]
  * q1[l1] * R_DDDD(k,l,a1,l1)
                             +  q1[u1] * (q1 % r1) * R_DDDD(k,l,u1,v1) * R_UUDD(v1,r1,r,s) * q2[a1]
  * q2[l1] * R_DDDD(u,v,a1,l1));
          }
        }
      }
    }
  }
  */
  FOR_EACH_6_END;
  return T;
}

// Pomeron-Pomeron-Tensor coupling structure #6
// iG_{\mu \nu \kappa \lambda \rho \sigma} ~ (l,s) = (6,4)
//
MTensor<std::complex<double>> MTensorPomeron::iG_PPT_06(const M4Vec &q1, const M4Vec &q2,
                                                        double g_PPT) const {
  const double               S0     = 1.0;  // Mass scale (GeV)
  const std::complex<double> FACTOR = -2.0 * zi / math::pow5(S0) * g_PPT;

  FTensor::Index<'a', 4> a;
  FTensor::Index<'b', 4> b;
  FTensor::Index<'c', 4> c;
  FTensor::Index<'d', 4> d;

  Tensor1<double, 4> q1_U = {q1[0], q1[1], q1[2], q1[3]};
  Tensor1<double, 4> q2_U = {q2[0], q2[1], q2[2], q2[3]};

  Tensor2<double, 4, 4> A;
  Tensor2<double, 4, 4> B;
  Tensor2<double, 4, 4> C;

  // Autocontractions
  A(a, b) = q2_U(c) * q2_U(d) * R_DDDD(a, b, c, d);
  B(a, b) = q1_U(c) * q1_U(d) * R_DDDD(a, b, c, d);
  C(a, b) = q1_U(c) * q2_U(d) * R_DDDD(a, b, c, d);

  MTensor<std::complex<double>> T = MTensor({4, 4, 4, 4, 4, 4}, std::complex<double>(0.0));
  FOR_EACH_6(LI);
  T({u, v, k, l, r, s}) = FACTOR * A(u, v) * B(k, l) * C(r, s);
  FOR_EACH_6_END;

  return T;
}


// ======================================================================


// f0 (Scalar) - (Pseudo)Scalar - (Pseudo)Scalar vertex function
// i\Gamma
//
// Input as contravariant (upper index) 4-vectors, meson on-shell mass M0
//
// Relations: p3_\mu iG^{\mu \nu} = 0, p4_\nu iG^{\mu \nu} = 0
//
std::complex<double> MTensorPomeron::iG_f0ss(const M4Vec &p3, const M4Vec &p4, double M0,
                                             double g1) const {
  const double S0 = 1.0;  // Mass scale (GeV)

  // Form FACTOR
  const double lambda4 = 1.0;  // GeV^4 (normalization scale)
  auto         F_f0ss  = [&](double q2) { return std::exp(-pow2(q2 - M0 * M0) / lambda4); };

  return zi * g1 * S0 * F_f0ss((p3 + p4).M2());
}

// f0 (Scalar) - Vector (Massive) - Vector (Massive) vertex function
// i\Gamma_{\mu \nu}
//
// Input as contravariant (upper index) 4-vectors, meson on-shell mass M0
//
// Relations: p3_\mu iG^{\mu \nu} = 0, p4_\nu iG^{\mu \nu} = 0
//
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iG_f0vv(const M4Vec &p3, const M4Vec &p4,
                                                            double M0, double g1, double g2) const {
  const double S0     = 1.0;  // Mass scale (GeV)
  const double LAMBDA = 1.0;  // GeV

  const double p3p4 = p3 * p4;
  const double p3sq = p3.M2();
  const double p4sq = p4.M2();

  const double               F_ = FM(p3sq, 0.5) * FM(p4sq, 0.5) * F((p3 + p4).M2(), M0, LAMBDA);
  const std::complex<double> FACTOR1 = zi * g1 * 2.0 / math::pow3(S0) * F_;
  const std::complex<double> FACTOR2 = zi * g2 * 2.0 / S0 * F_;

  Tensor2<std::complex<double>, 4, 4> T;
  FOR_EACH_2(LI);
  T(u, v) = FACTOR1 * (p3sq * p4sq * g[u][v] - p4sq * (p3 % u) * (p3 % v) -
                       p3sq * (p4 % u) * (p4 % v) + p3p4 * (p3 % u) * (p4 % v)) +
            FACTOR2 * ((p4 % u) * (p3 % v) - p3p4 * g[u][v]);
  FOR_EACH_2_END;

  return T;
}

// Vector (Massive) - Pseudoscalar - Pseudoscalar vertex function
// i\Gamma_\mu(k1,k2)
//
// Input as contravariant (upper index) 4-vectors
//
Tensor1<std::complex<double>, 4> MTensorPomeron::iG_vpsps(const M4Vec &k1, const M4Vec &k2,
                                                          double M0, double g1) const {
  const M4Vec p = k1 - k2;

  // Form FACTOR
  const double LAMBDA = 1.0;  // GeV
  const double F_     = FM(k1.M2(), 0.5) * FM(k2.M2(), 0.5) * F((k1 + k2).M2(), M0, LAMBDA);
  const std::complex<double> FACTOR = -0.5 * zi * g1 * F_;

  Tensor1<std::complex<double>, 4> T;
  for (const auto &mu : LI) { T(mu) = FACTOR * (p % mu); }
  return T;
}

// pseudoscalar - vector - vector vertex function
// i\Gamma_{\kappa\lambda}(k1,k2)
//
// Input as contravariant (upper index) 4-vectors, M0 is the pseudoscalar on-shell mass
//
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iG_psvv(const M4Vec &p3, const M4Vec &p4,
                                                            double M0, double g1) const {
  // Form FACTOR
  const double LAMBDA = 1.0;  // GeV

  const double               S0     = 1.0;  // Mass scale (GeV)
  const std::complex<double> FACTOR = zi * g1 / (2 * S0) * F((p3 + p4).M2(), M0, LAMBDA);

  Tensor1<std::complex<double>, 4> p3_ = {p3[0], p3[1], p3[2], p3[3]};
  Tensor1<std::complex<double>, 4> p4_ = {p4[0], p4[1], p4[2], p4[3]};

  FTensor::Index<'a', 4> mu;
  FTensor::Index<'b', 4> nu;
  FTensor::Index<'c', 4> rho;
  FTensor::Index<'d', 4> sigma;

  // Contract
  Tensor2<std::complex<double>, 4, 4> T;
  T(mu, nu) = FACTOR * p3_(rho) * p4_(sigma) * eps_lo(mu, nu, rho, sigma);

  return T;
}

// f2 - pseudoscalar - pseudoscalar vertex function
// i\Gamma_{\kappa\lambda}(k1,k2)
//
// Input as contravariant (upper index) 4-vectors, M0 is the f2 meson on-shell mass
//
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iG_f2psps(const M4Vec &k1, const M4Vec &k2,
                                                              double M0, double g1) const {
  // Form FACTOR
  const double               LAMBDA = 1.0;  // GeV
  const double               S0     = 1.0;  // Mass scale (GeV)
  const M4Vec                k1_k2  = k1 - k2;
  const std::complex<double> FACTOR = -zi * g1 / (2 * S0) * F((k1 + k2).M2(), M0, LAMBDA);

  const double k1_k2_sq = k1_k2.M2();

  Tensor2<std::complex<double>, 4, 4> T;
  FOR_EACH_2(LI);
  T(u, v) = FACTOR * ((k1_k2 % u) * (k1_k2 % v) - 0.25 * g[u][v] * k1_k2_sq);
  FOR_EACH_2_END;

  return T;
}

// f2 - Vector (massive) - Vector (Massive) vertex function
// i\Gamma_{\mu\nu\kappa\lambda}(k1,k2)
//
// Input as contravariant (upper index) 4-vectors, M0 is the f2 meson on-shell mass
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::iG_f2vv(const M4Vec &k1, const M4Vec &k2,
                                                                  double M0, double g1,
                                                                  double g2) const {
  const double S0     = 1.0;  // Mass scale (GeV)
  const double LAMBDA = 1.0;  // GeV

  const Tensor4<std::complex<double>, 4, 4, 4, 4> G0 = Gamma0(k1, k2);
  const Tensor4<std::complex<double>, 4, 4, 4, 4> G2 = Gamma2(k1, k2);

  const std::complex<double> F_ =
      FM(k1.M2(), 0.5) * FM(k2.M2(), 0.5) * F((k1 + k2).M2(), M0, LAMBDA);
  const std::complex<double> FACTOR1 = zi * 2.0 / math::pow3(S0) * g1 * F_;
  const std::complex<double> FACTOR2 = -zi * 1.0 / S0 * g2 * F_;

  Tensor4<std::complex<double>, 4, 4, 4, 4> T;
  FOR_EACH_4(LI);
  T(u, v, k, l) = FACTOR1 * G0(u, v, k, l) + FACTOR2 * G2(u, v, k, l);
  FOR_EACH_4_END;

  return T;
}

// f2 - gamma - gamma vertex function
// i\Gamma_{\mu\nu\kappa\lambda}(k1,k2)
//
// Input as contravariant (upper index) 4-vectors, M0 is the f2 meson on-shell mass
// Couplings g1,g2
//
// Example couplings:
//
// const double e = msqrt(qed::alpha_QED() * 4.0 * PI);     // ~ 0.3, no running
// const double a_f2yy = pow2(e) / (4 * PI) * 1.45;  // GeV^{-3}
// const double b_f2yy = pow2(e) / (4 * PI) * 2.49;  // GeV^{-1}

Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::iG_f2yy(const M4Vec &k1, const M4Vec &k2,
                                                                  double M0, double g1,
                                                                  double g2) const {
  // Form FACTOR
  const double LAMBDA = 1.0;  // GeV

  const Tensor4<std::complex<double>, 4, 4, 4, 4> G0 = Gamma0(k1, k2);
  const Tensor4<std::complex<double>, 4, 4, 4, 4> G2 = Gamma2(k1, k2);

  const std::complex<double> FACTOR =
      zi * FM(k1.M2(), 0.5) * FM(k2.M2(), 0.5) * F((k1 + k2).M2(), M0, LAMBDA);

  Tensor4<std::complex<double>, 4, 4, 4, 4> T;
  FOR_EACH_4(LI);
  T(u, v, k, l) = FACTOR * (2.0 * g1 * G0(u, v, k, l) - g2 * G2(u, v, k, l));
  FOR_EACH_4_END;

  return T;
}

// Pomeron-Pseudoscalar-Pseudoscalar vertex function
// i\Gamma_{\mu\nu}(p',p)
//
// Input as contravariant (upper index) 4-vectors
//
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iG_Ppsps(const M4Vec &prime, const M4Vec &p,
                                                             double g1) const {
  const M4Vec                psum   = prime + p;
  const double               M2     = psum.M2();
  const std::complex<double> FACTOR = -zi * 2.0 * g1 * FM((prime - p).M2(), 0.5);

  Tensor2<std::complex<double>, 4, 4> T;
  FOR_EACH_2(LI);
  T(u, v) = FACTOR * ((psum % u) * (psum % v) - 0.25 * g[u][v] * M2);
  FOR_EACH_2_END;

  return T;
}

// Pomeron-Vector(massive)-Vector(massive) vertex function
// i\Gamma_{\alpha\beta\gamma\delta}(p',p)
//
// Input M0 is the vector meson on-shell mass
//
// Input as contravariant (upper index) 4-vectors
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::iG_Pvv(const M4Vec &prime, const M4Vec &p,
                                                                 double g1, double g2) const {
  const Tensor4<std::complex<double>, 4, 4, 4, 4> G0 = Gamma0(prime, -p);
  const Tensor4<std::complex<double>, 4, 4, 4, 4> G2 = Gamma2(prime, -p);

  const std::complex<double> FACTOR = zi * FM((prime - p).M2(), 0.5);

  Tensor4<std::complex<double>, 4, 4, 4, 4> T;
  FOR_EACH_4(LI);
  T(u, v, k, l) = FACTOR * (2.0 * g1 * G0(u, v, k, l) - g2 * G2(u, v, k, l));
  FOR_EACH_4_END;

  return T;
}


// Gamma-Vector meson transition vertex
// iGamma_{\mu \nu}
//
// pdg = 113 / "rho0", 223 / "omega0", 333 / "phi0"
//
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iG_yV(double q2, int pdg) const {
  const double e      = msqrt(qed::alpha_QED() * 4.0 * PI);  // ~ 0.3, no running
  double       gammaV = 0.0;
  double       mV     = 0.0;

  // Vector Meson Dominance (VMD) type parameters
  if (pdg == 113) {  // rho
    gammaV = msqrt(4 * PI / 0.496);
    mV     = 0.770;
  } else if (pdg == 223) {  // omega
    gammaV = msqrt(4 * PI / 0.042);
    mV     = 0.785;
  } else if (pdg == 333) {  // phi
    gammaV = (-1.0) * msqrt(4 * PI / 0.0716);
    mV     = 1.020;
  } else {
    throw std::invalid_argument("MTensorPomeron::iG_yV: Unknown vector meson pdg = " +
                                std::to_string(pdg));
  }

  Tensor2<std::complex<double>, 4, 4> T;
  FOR_EACH_2(LI);
  T(u, v) = -zi * e * pow2(mV) / gammaV * g[u][v];
  FOR_EACH_2_END;

  return T;
}

// ----------------------------------------------------------------------
// Propagators

// Tensor Pomeron propagator: (covariant == contravariant)
//
// i\Delta_{\mu\nu,\kappa\lambda}(s,t) = i\Delta^{\mu\nu,\kappa\lambda}(s,t)
//
// Input as (sub)-Mandelstam invariants: s,t
//
// Symmetry relations:
// \Delta_{\mu\nu,\kappa\lambda} = \Delta_{\nu\mu,\kappa\lambda} =
// \Delta_{\mu\nu,\lambda\kappa} = \Delta_{\kappa\lambda,\mu\nu}
//
// Contraction with g^{\mu\nu} or g^{\kappa\lambda} gives 0
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::iD_P(double s, double t) const {
  const std::complex<double> FACTOR =
      1.0 / (4.0 * s) * std::pow(-zi * s * Param.ap_P, alpha_P(t) - 1.0);

  Tensor4<std::complex<double>, 4, 4, 4, 4> T;
  FOR_EACH_4(LI);
  T(u, v, k, l) = FACTOR * (g[u][k] * g[v][l] + g[u][l] * g[v][k] - 0.5 * g[u][v] * g[k][l]);
  FOR_EACH_4_END;

  return T;
}

// Tensor Reggeon propagator iD (f2-a2-reggeon)
//
// Input as (sub)-Mandelstam invariants: s,t
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::iD_2R(double s, double t) const {
  const std::complex<double> FACTOR =
      1.0 / (4.0 * s) * std::pow(-zi * s * Param.ap_2R, alpha_2R(t) - 1.0);

  Tensor4<std::complex<double>, 4, 4, 4, 4> T;
  FOR_EACH_4(LI);
  T(u, v, k, l) = FACTOR * (g[u][k] * g[v][l] + g[u][l] * g[v][k] - 0.5 * g[u][v] * g[k][l]);
  FOR_EACH_4_END;

  return T;
}

// Vector Reggeon propagator iD (rho-reggeon, omega-reggeon)
//
// Input as (sub)-Mandelstam invariants s,t
//
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iD_1R(double s, double t) const {
  const std::complex<double> FACTOR =
      zi / pow2(Param.M_1R) * std::pow(-zi * s * Param.ap_1R, alpha_1R(t) - 1.0);

  Tensor2<std::complex<double>, 4, 4> T;
  FOR_EACH_2(LI);
  T(u, v) = FACTOR * g[u][v];
  FOR_EACH_2_END;

  return T;
}

// Odderon propagator iD
//
// Input as (sub)-Mandelstam invariants s,t
//
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iD_O(double s, double t) const {
  const std::complex<double> FACTOR =
      -zi * Param.eta_O / pow2(Param.M_O) * std::pow(-zi * s * Param.ap_O, alpha_O(t) - 1.0);

  Tensor2<std::complex<double>, 4, 4> T;
  FOR_EACH_2(LI);
  T(u, v) = FACTOR * g[u][v];
  FOR_EACH_2_END;

  return T;
}


// Vector propagator iD_{\mu \nu} == iD^{\mu \nu} with (naive) Reggeization
//
// Input as contravariant 4-vector and the particle peak mass M0
//
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iD_V(const M4Vec &p, double M0,
                                                         double s34) const {
  const double m2       = p.M2();
  const double alpha    = 0.5 + 0.9 * m2;  // Typical Reggeon trajectory
  const double s_thresh = 4 * pow2(M0);
  const double phi      = PI / 2.0 * std::exp((s_thresh - s34) / s_thresh) - PI / 2.0;

  // Reggeization
  std::complex<double> REGGEIZE = std::pow(std::exp(zi * phi) * s34 / s_thresh, alpha - 1.0);

  Tensor2<std::complex<double>, 4, 4> T;
  FOR_EACH_2(LI);
  T(u, v) = -g[u][v] / (m2 - pow2(M0)) * REGGEIZE;
  FOR_EACH_2_END;

  return T;
}

// Scalar propagator iD with zero width
//
// Input as 4-vector and the particle peak mass M0
//
std::complex<double> MTensorPomeron::iD_MES0(const M4Vec &p, double M0) const {
  return zi / (p.M2() - pow2(M0));
}

// Scalar propagator iD with finite Breit-Wigner width
//
// Input as 4-vector and the particle peak mass M0 and full width
//
std::complex<double> MTensorPomeron::iD_MES(const M4Vec &p, double M0, double Gamma) const {
  const std::complex<double> Delta = 1.0 / (p.M2() - pow2(M0) + zi * M0 * Gamma);

  return zi * Delta;
}

// Massive vector propagator iD_{\mu \nu} or iD^{\mu \nu} with INDEX_UP == true
//
// Input as contravariant 4-vector and the particle peak mass M0 and full width Gamma
//
Tensor2<std::complex<double>, 4, 4> MTensorPomeron::iD_VMES(const M4Vec &p, double M0, double Gamma,
                                                            bool INDEX_UP) const {
  // Transverse part
  const double               m2      = p.M2();
  const std::complex<double> Delta_T = 1.0 / (m2 - pow2(M0) + zi * M0 * Gamma);

  // Longitudinal part (does not enter here)
  const double Delta_L = 0.0;

  Tensor2<std::complex<double>, 4, 4> T;
  if (INDEX_UP) {
    FOR_EACH_2(LI);
    T(u, v) =
        zi * (-g[u][v] + (p[u]) * (p[v]) / m2) * Delta_T - zi * (p[u]) * (p[v]) / m2 * Delta_L;
    FOR_EACH_2_END;
  } else {
    FOR_EACH_2(LI);
    T(u, v) =
        zi * (-g[u][v] + (p % u) * (p % v) / m2) * Delta_T - zi * (p % u) * (p % v) / m2 * Delta_L;
    FOR_EACH_2_END;
  }
  return T;
}

// Massive tensor propagator iD_{\mu \nu \kappa \lambda} or iD^{} with INDEX_UP == true
//
// Input as contravariant (upper index) 4-vector and the particle peak mass M0
// and full width Gamma
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::iD_TMES(const M4Vec &p, double M0,
                                                                  double Gamma,
                                                                  bool   INDEX_UP) const {
  const double m2 = p.M2();

  // Inline auxialary function
  auto ghat = [&](int u, int v) {
    if (INDEX_UP) {
      return -g[u][v] + (p[u]) * (p[v]) / m2;
    } else {
      return -g[u][v] + (p % u) * (p % v) / m2;
    }
  };

  const std::complex<double> Delta = 1.0 / (m2 - pow2(M0) + zi * M0 * Gamma);

  Tensor4<std::complex<double>, 4, 4, 4, 4> T;
  FOR_EACH_4(LI);
  T(u, v, k, l) = zi * Delta *
                  (0.5 * (ghat(u, k) * ghat(v, l) + ghat(u, l) * ghat(v, k)) -
                   1.0 / 3.0 * ghat(u, v) * ghat(k, l));
  FOR_EACH_4_END;

  return T;
}

// ----------------------------------------------------------------------
// Trajectories (simple linear/affine here)

// Pomeron trajectory
double MTensorPomeron::alpha_P(double t) const { return 1.0 + Param.delta_P + Param.ap_P * t; }
// Odderon trajectory
double MTensorPomeron::alpha_O(double t) const { return 1.0 + Param.delta_O + Param.ap_O * t; }
// Reggeon (rho,omega) trajectory
double MTensorPomeron::alpha_1R(double t) const { return 1.0 + Param.delta_1R + Param.ap_1R * t; }
// Reggeon (f2,a2) trajectory
double MTensorPomeron::alpha_2R(double t) const { return 1.0 + Param.delta_2R + Param.ap_2R * t; }

// ----------------------------------------------------------------------
// Form factors

// Proton electromagnetic Dirac form FACTOR
double MTensorPomeron::F1(double t) const { return form::F1(t); }

// Proton electromagnetic Pauli form FACTOR
double MTensorPomeron::F2(double t) const { return form::F2(t); }

// Proton electromagnetic Dirac form FACTOR
double MTensorPomeron::F1_(double t) const {
  return (1 - (t / (4 * pow2(PDG::mp))) * form::mu_ratio()) / (1 - t / (4 * pow2(PDG::mp))) * GD(t);
}

// Proton electromagnetic Pauli form FACTOR
double MTensorPomeron::F2_(double t) const {
  return (form::mu_ratio() - 1) / (1 - t / (4 * pow2(PDG::mp))) * GD(t);
}

// Dipole form FACTOR
double MTensorPomeron::GD(double t) const {
  const double m2D = 0.71;  // GeV^2
  return 1.0 / pow2(1 - t / m2D);
}

// Meson form FACTOR (pion electromagnetic type)
double MTensorPomeron::FM(double q2, double LAMBDA2) const { return 1.0 / (1.0 - q2 / LAMBDA2); }

// Resonance form factor
double MTensorPomeron::F(double q2, double M0, double LAMBDA) const {
  return std::exp(-pow2(q2 - pow2(M0)) / math::pow4(LAMBDA));
}

// ----------------------------------------------------------------------
// Tensor functions

// Output: \Gamma^{(0)}_{\mu\nu\kappa\lambda}
//
// Input must be contravariant (upper index) 4-vectors
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::Gamma0(const M4Vec &k1,
                                                                 const M4Vec &k2) const {
  const double                              k1k2 = k1 * k2;  // 4-dot product
  Tensor4<std::complex<double>, 4, 4, 4, 4> T;
  FOR_EACH_4(LI);
  T(u, v, k, l) = (k1k2 * g[u][v] - (k2 % u) * (k1 % v)) *
                  ((k1 % k) * (k2 % l) + (k2 % k) * (k1 % l) - 0.5 * k1k2 * g[k][l]);
  FOR_EACH_4_END;

  return T;
}

// Output: \Gamma^{(2)}_{\mu\nu\kappa\lambda}
//
// Input must be contravariant (upper index) 4-vectors
//
Tensor4<std::complex<double>, 4, 4, 4, 4> MTensorPomeron::Gamma2(const M4Vec &k1,
                                                                 const M4Vec &k2) const {
  const double                              k1k2 = k1 * k2;  // 4-dot product
  Tensor4<std::complex<double>, 4, 4, 4, 4> T;
  FOR_EACH_4(LI);
  T(u, v, k, l) = k1k2 * (g[u][k] * g[v][l] + g[u][l] * g[v][k]) +
                  g[u][v] * ((k1 % k) * (k2 % l) + (k2 % k) * (k1 % l)) -
                  (k1 % v) * (k2 % l) * g[u][k] - (k1 % v) * (k2 % k) * g[u][l] -
                  (k2 % u) * (k1 % l) * g[v][k] - (k2 % u) * (k1 % k) * g[v][l] -
                  (k1k2 * g[u][v] - (k2 % u) * (k1 % v)) * g[k][l];
  FOR_EACH_4_END;

  return T;
}

// Pre-calculate tensors for speed
//
// 1. Minkowski metric tensor
// 2. Auxialary (helper) tensor
// 1/2 g_{\mu\kappa} g_{\nu\lambda} + 1/2 g_{\mu\lambda}g_{\nu\kappa} - 1/4
// g_{\mu\nu}g_{kappa\lambda}
//
void MTensorPomeron::CalcRTensor() {
  // Minkowski metric tensor (+1,-1,-1,-1)
  FOR_EACH_2(LI);
  gT(u, v) = g[u][v];
  FOR_EACH_2_END;

  // ------------------------------------------------------------------
  // Covariant [lower index] 4D-epsilon tensor

  const MTensor<int> etensor = math::EpsTensor(4);

  FOR_EACH_4(LI);
  std::vector<std::size_t> ind = {u, v, k, l};
  eps_lo(u, v, k, l)           = static_cast<double>(etensor(ind));
  FOR_EACH_4_END;

  // Free Lorentz indices [second parameter denotes the range of index]
  FTensor::Index<'a', 4> a;
  FTensor::Index<'b', 4> b;
  FTensor::Index<'c', 4> c;
  FTensor::Index<'d', 4> d;
  FTensor::Index<'g', 4> alfa;
  FTensor::Index<'h', 4> beta;

  // Contravariant version (make it in two steps)
  eps_hi(a, b, c, d) = eps_lo(alfa, beta, c, d) * gT(a, alfa) * gT(b, beta);
  eps_hi(a, b, c, d) = eps_hi(a, b, alfa, beta) * gT(c, alfa) * gT(d, beta);

  // ------------------------------------------------------------------

  // Aux tensor R
  FTensor::Tensor4<double, 4, 4, 4, 4> R;

  R(a, b, c, d) =
      0.5 * gT(a, c) * gT(b, d) + 0.5 * gT(a, d) * gT(b, c) + 0.25 * gT(a, b) * gT(c, d);

  // Different mixed index covariant/contravariant versions
  // by contraction with g_{\mu\nu}
  R_DDDD             = R;
  R_DDDU(a, b, c, d) = R(a, b, c, alfa) * gT(d, alfa);
  R_DDUU(a, b, c, d) = R(a, b, alfa, beta) * gT(c, alfa) * gT(d, beta);
  R_UUDD(a, b, c, d) = R(alfa, beta, c, d) * gT(a, alfa) * gT(b, beta);

  // -------------------------------------------------------------------
  // Pre-calculated tensor contractions for tensor resonances

  auto FIX1 = [&]() {
    MTensor<double> A = MTensor({4, 4, 4, 4, 4, 4}, double(0.0));  // Init with zeros!
    FOR_EACH_6(LI);
    const std::vector<size_t> ind = {u, v, k, l, r, s};
    for (auto const &a1 : LI) {
      for (auto const &r1 : LI) {
        for (auto const &s1 : LI) {
          A(ind) += R_DDDD(u, v, r1, a1) * R_DDDU(k, l, s1, a1) * R_DDUU(r, s, r1, s1);
        }
      }
    }
    FOR_EACH_6_END;
    return A;
  };

  auto FIX2 = [&]() {
    MTensor<double> A = MTensor({4, 4, 4, 4, 4, 4, 4, 4}, double(0.0));  // Init with zeros!
    FOR_EACH_6(LI);
    for (auto const &a1 : LI) {
      for (auto const &r1 : LI) {
        for (auto const &s1 : LI) {
          for (auto const &u1 : LI) {
            A({u, v, k, l, r, s, u1, r1}) +=
                R_DDDD(u, v, u1, a1) * R_DDDU(k, l, s1, a1) * R_DDUU(r, s, r1, s1);
          }
        }
      }
    }
    FOR_EACH_6_END;
    return A;
  };

  auto FIX3 = [&]() {
    MTensor<double> A = MTensor({4, 4, 4, 4, 4, 4, 4, 4}, double(0.0));  // Init with zeros!
    FOR_EACH_6(LI);
    for (auto const &a1 : LI) {
      for (auto const &r1 : LI) {
        for (auto const &s1 : LI) {
          for (auto const &u1 : LI) {
            A({u, v, k, l, r, s, u1, s1}) +=
                R_DDDD(u, v, r1, a1) * R_DDDU(k, l, u1, a1) * R_DDUU(r, s, r1, s1);
          }
        }
      }
    }
    FOR_EACH_6_END;
    return A;
  };

  T1 = FIX1();
  T2 = FIX2();
  T3 = FIX3();
}

}  // namespace gra
