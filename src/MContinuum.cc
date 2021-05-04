// 'Continuum' i.e. directly constructed phase space class
//
// (c) 2017-2021 Mikael Mieskolainen
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

// C++
#include <algorithm>
#include <complex>
#include <iostream>
#include <random>
#include <vector>

// OWN
#include "Graniitti/MAux.h"
#include "Graniitti/MContinuum.h"
#include "Graniitti/MForm.h"
#include "Graniitti/MFragment.h"
#include "Graniitti/MKinematics.h"
#include "Graniitti/MMath.h"
#include "Graniitti/MMatrix.h"
#include "Graniitti/MProcess.h"
#include "Graniitti/MSpin.h"
#include "Graniitti/MUserCuts.h"

// Libraries
#include "rang.hpp"

using gra::aux::indices;
using gra::math::abs2;
using gra::math::msqrt;
using gra::math::PI;
using gra::math::pow2;
using gra::math::pow3;
using gra::math::pow4;
using gra::math::zi;
using gra::PDG::GeV2barn;

namespace gra {
// This is needed by construction
MContinuum::MContinuum() { Initialize(); }

// Constructor
MContinuum::MContinuum(std::string process, const std::vector<aux::OneCMD> &syntax) {
  Initialize();
  InitHistograms();
  SetProcess(process, syntax);

  // Init final states
  M4Vec zerovec(0, 0, 0, 0);
  for (std::size_t i = 0; i < 10; ++i) { lts.pfinal.push_back(zerovec); }
  std::cout << "MContinuum:: [Constructor done]" << std::endl;
}

void MContinuum::Initialize() {
  const std::vector<std::string> supported = {"PP", "yP", "OP", "yy", "gg"};
  CID                                      = "C";
  ProcPtr                                  = MSubProc(supported, CID);
}

// Destructor
MContinuum::~MContinuum() {}

// Initialize cut and process spesific postsetup
void MContinuum::post_Constructor() {
  std::cout << "MContinuum::post_Constructor: " << EXCITATION << std::endl;

  // Set sampling boundaries
  SetTechnicalBoundaries(gcuts, EXCITATION);

  // Technical cuts here
  if (gcuts.kt_max < 0.0) {  // not set by user

    gcuts.kt_min = 0.0;

    // Pomeron-Pomeron continuum (low-pt guaranteed by form factors)
    gcuts.kt_max = 10.0 / lts.decaytree.size();

    // QED/EW/Durham-QCD processes (high pt)
    if (ProcPtr.ISTATE != "PP" && ProcPtr.ISTATE != "yP" && ProcPtr.ISTATE != "OP") {
      gcuts.kt_max = lts.sqrt_s / 2.0;  // kinematic maximum
    }
  }

  // Initialize phase space dimension (3*Nf - 4)
  ProcPtr.LIPSDIM = 3 * (lts.decaytree.size() + 2) - 4;

  if (EXCITATION == 1) { ProcPtr.LIPSDIM += 1; }
  if (EXCITATION == 2) { ProcPtr.LIPSDIM += 2; }
}

// Update kinematics (screening kT loop calls this)
// Exact 4-momentum conservation at loop vertices, that is, by using this one
// does not assume vanishing external momenta in the screening loop calculation
bool MContinuum::LoopKinematics(const std::vector<double> &p1p, const std::vector<double> &p2p) {
  const M4Vec        beamsum = lts.pbeam1 + lts.pbeam2;
  const unsigned int Kf      = lts.decaytree.size();  // Number of central particles
  const unsigned int Nf      = Kf + 2;                // Number of final states

  // SET new proton pT degrees of freedom
  lts.pfinal[1].SetPxPy(p1p[0], p1p[1]);
  lts.pfinal[2].SetPxPy(p2p[0], p2p[1]);

  // Get central final states pT degrees of freedom
  std::vector<M4Vec> p(Kf, M4Vec(0, 0, 0, 0));
  BLinearSystem(p, pkt_, lts.pfinal[1], lts.pfinal[2]);

  // Set central particles px,py,pz,e
  const unsigned int offset = 3;  // indexing
  M4Vec              sumP(0, 0, 0, 0);
  for (std::size_t i = 0; i < Kf; ++i) {
    const double y = lts.pfinal_orig[i + offset].Rap();
    const double m = lts.pfinal_orig[i + offset].M();

    // px,py
    lts.pfinal[i + offset].SetPxPy(p[i].Px(), p[i].Py());

    // pz,E
    const double mt = msqrt(pow2(m) + lts.pfinal[i + offset].Pt2());
    lts.pfinal[i + offset].SetPzE(mt * std::sinh(y), mt * std::cosh(y));

    sumP += lts.pfinal[i + offset];
  }

  // pz and E of forward protons/N*
  const double pt1 = lts.pfinal[1].Pt();
  const double pt2 = lts.pfinal[2].Pt();
  const double m1  = lts.pfinal_orig[1].M();
  const double m2  = lts.pfinal_orig[2].M();

  double p1z = gra::kinematics::SolvePz(m1, m2, pt1, pt2, sumP.Pz(), sumP.E(), lts.s);
  double p2z = -(sumP.Pz() + p1z);  // by momentum conservation

  // Enforce scattering direction +p -> +p, -p -> -p (VERY RARE POLYNOMIAL
  // BRANCH FLIP)
  if (p1z < 0 || p2z > 0) { return false; }

  lts.pfinal[1].SetPzE(p1z, msqrt(pow2(m1) + pow2(pt1) + pow2(p1z)));
  lts.pfinal[2].SetPzE(p2z, msqrt(pow2(m2) + pow2(pt2) + pow2(p2z)));
  lts.pfinal[0] = sumP;

  // ------------------------------------------------------------------
  // Now boost if asymmetric beams
  if (std::abs(beamsum.Pz()) > 1e-6) {
    constexpr int sign = 1;  // positive -> boost to the lab
    kinematics::LorentzBoost(beamsum, lts.sqrt_s, lts.pfinal[1], sign);
    kinematics::LorentzBoost(beamsum, lts.sqrt_s, lts.pfinal[2], sign);
    kinematics::LorentzBoost(beamsum, lts.sqrt_s, lts.pfinal[0], sign);

    for (std::size_t i = 0; i < Kf; ++i) {
      kinematics::LorentzBoost(beamsum, lts.sqrt_s, lts.pfinal[i + offset], sign);
    }
  }
  // ------------------------------------------------------------------

  // Check Energy-Momentum
  if (!gra::math::CheckEMC(beamsum - (lts.pfinal[1] + lts.pfinal[2] + lts.pfinal[0]))) {
    return false;
  }

  return GetLorentzScalars(Nf);
}

// Get weight
double MContinuum::EventWeight(const std::vector<double> &randvec, AuxIntData &aux) {
  double W = 0.0;

  // Kinematics
  aux.kinematics_ok = BNRandomKin(lts.decaytree.size() + 2, randvec);
  aux.fidcuts_ok    = FiducialCuts();
  aux.vetocuts_ok   = VetoCuts();

  if (aux.Valid()) {
    // Matrix element squared
    const double MatESQ = GetAmp2();

    // --------------------------------------------------------------------
    // Cascade resonances phase-space
    const double C_space = CascadePS();
    // --------------------------------------------------------------------

    // ** EVENT WEIGHT **
    W = C_space * (1.0 / S_factor) * BNPhaseSpaceWeight() * BNIntegralVolume() * MatESQ * GeV2barn /
        MollerFlux();
  }

  aux.amplitude_ok = CheckInfNan(W);

  // As the last step: Histograms
  const double totalweight = W * aux.vegasweight;
  FillHistograms(totalweight, lts);

  return W;
}

// Fiducial cuts
bool MContinuum::FiducialCuts() const { return CommonCuts(); }

// Record event
bool MContinuum::EventRecord(HepMC3::GenEvent &evt) { return CommonRecord(evt); }

void MContinuum::PrintInit(bool silent) const {
  if (!silent) {
    PrintSetup();

    // Construct prettyprint diagram
    std::string proton1 = "-----------EL--------->";
    std::string proton2 = "-----------EL--------->";

    if (EXCITATION == 1) { proton1 = "-----------F2-xxxxxxxx>"; }
    if (EXCITATION == 2) {
      proton1 = "-----------F2-xxxxxxxx>";
      proton2 = "-----------F2-xxxxxxxx>";
    }

    std::string pomeronloop = (SCREENING == true) ? "     **    " : "           ";

    std::vector<std::string> legs;
    for (const auto &i : indices(lts.decaytree)) {
      char buff[250];
      snprintf(buff, sizeof(buff), "x---------> %d (%s) [Q=%s, J=%s]", lts.decaytree[i].p.pdg,
               lts.decaytree[i].p.name.c_str(),
               gra::aux::Charge3XtoString(lts.decaytree[i].p.chargeX3).c_str(),
               gra::aux::Spin2XtoString(lts.decaytree[i].p.spinX2).c_str());
      std::string leg = buff;
      legs.push_back(leg);
    }

    std::vector<std::string> feynmangraph;
    feynmangraph.push_back("||         ");
    for (const auto &i : indices(lts.decaytree)) {
      feynmangraph.push_back(legs[i]);
      if (i < lts.decaytree.size() - 1) feynmangraph.push_back("|          ");
    }
    feynmangraph.push_back("||         ");

    // Print diagram
    std::cout << proton1 << std::endl;
    for (const auto &i : indices(feynmangraph)) {
      if (SCREENING) {  // Put red
        std::cout << rang::fg::red << pomeronloop << rang::style::reset;
      } else {
        std::cout << rang::fg::red << pomeronloop << rang::style::reset;
      }
      std::cout << feynmangraph[i] << std::endl;
    }
    std::cout << proton2 << std::endl;
    std::cout << std::endl;

    // Generation cuts
    std::cout << std::endl;
    std::cout << rang::style::bold << "Generation cuts:" << rang::style::reset << std::endl
              << std::endl;
    printf(
        "- Rap : Final state rapidity [min, max] = [%0.2f, %0.2f]     "
        "\t(user)       \n"
        "- Kt  : Intermediate         [min, max] = [%0.2f, %0.2f] GeV "
        "\t(fixed/user) \n"
        "- Pt  : Forward leg          [min, max] = [%0.2f, %0.2f] GeV "
        "\t(fixed/user) "
        "\n",
        gcuts.rap_min, gcuts.rap_max, gcuts.kt_min, gcuts.kt_max, gcuts.forward_pt_min,
        gcuts.forward_pt_max);

    if (EXCITATION != 0) {
      printf(
          "- Xi  : Forward leg (M^2/s)  [min, max] = [%0.2E, %0.2E]     "
          "\t(fixed/user) \n",
          gcuts.XI_min, gcuts.XI_max);
    }

    PrintFiducialCuts();
  }
}

// (3*Nf-4)-dimensional phase space vector initialization
bool MContinuum::BNRandomKin(unsigned int Nf, const std::vector<double> &randvec) {
  const unsigned int Kf = Nf - 2;  // Central system multiplicity

  // log-change of variables for pt
  const double u1 =
      std::log(gcuts.forward_pt_min + ZERO_EPS) +
      (std::log(gcuts.forward_pt_max) - std::log(gcuts.forward_pt_min + ZERO_EPS)) * randvec[0];
  const double u2 =
      std::log(gcuts.forward_pt_min + ZERO_EPS) +
      (std::log(gcuts.forward_pt_max) - std::log(gcuts.forward_pt_min + ZERO_EPS)) * randvec[1];

  const double pt1 = std::exp(u1);
  const double pt2 = std::exp(u2);

  const double       phi1   = 2.0 * gra::math::PI * randvec[2];
  const double       phi2   = 2.0 * gra::math::PI * randvec[3];
  const unsigned int offset = 4;  // 4 variables above

  // Decay product masses
  // ==============================================================
  for (const auto &i : indices(lts.decaytree)) {
    GetOffShellMass(lts.decaytree[i], lts.decaytree[i].m_offshell);
  }
  // ==============================================================

  // Intermediate kt
  std::vector<double> kt(Kf - 1, 0.0);  // Kf-1
  size_t              ind = offset;
  for (const auto &i : indices(kt)) {
    kt[i] = gcuts.kt_min + (gcuts.kt_max - gcuts.kt_min) * randvec[ind];
    ++ind;
  }

  // Intermediate phi
  std::vector<double> phi(Kf - 1, 0.0);  // Kf-1
  for (const auto &i : indices(phi)) {
    phi[i] = 2.0 * PI * randvec[ind];
    ++ind;
  }

  // Final state rapidity
  std::vector<double> y(Kf, 0.0);  // Kf
  for (const auto &i : indices(y)) {
    y[i] = gcuts.rap_min + (gcuts.rap_max - gcuts.rap_min) * randvec[ind];
    ++ind;
  }

  // Forward N* system masses
  std::vector<double> mvec;
  std::vector<double> rvec;
  if (EXCITATION == 1) { rvec = {randvec[ind]}; }
  if (EXCITATION == 2) { rvec = {randvec[ind], randvec[ind + 1]}; }
  SampleForwardMasses(mvec, rvec);

  return BNBuildKin(Nf, pt1, pt2, phi1, phi2, kt, phi, y, mvec[0], mvec[1]);
}

// Build kinematics of 2->N
bool MContinuum::BNBuildKin(unsigned int Nf, double pt1, double pt2, double phi1, double phi2,
                            const std::vector<double> &kt, const std::vector<double> &phi,
                            const std::vector<double> &y, double m1, double m2) {
  const unsigned int Kf      = Nf - 2;  // Central system multiplicity
  const M4Vec        beamsum = lts.pbeam1 + lts.pbeam2;

  if (lts.decaytree.size() != Kf) {
    std::string str =
        "MContinuum::BNBuildKin: Decaytree first level topology "
        "= " +
        std::to_string(lts.decaytree.size()) + " (should be " + std::to_string(Kf) +
        " for this process)!";
    throw std::invalid_argument(str);
  }

  // Forward protons px,py
  M4Vec p1(pt1 * std::cos(phi1), pt1 * std::sin(phi1), 0, 0);
  M4Vec p2(pt2 * std::cos(phi2), pt2 * std::sin(phi2), 0, 0);

  // Auxialary "difference momentum" q0 = p0 - p1 ...
  pkt_.resize(Kf - 1);
  for (const auto &i : indices(pkt_)) {
    pkt_[i] = M4Vec(kt[i] * std::cos(phi[i]), kt[i] * std::sin(phi[i]), 0, 0);
  }

  // Apply linear system to get p
  std::vector<M4Vec> p(Kf, M4Vec(0, 0, 0, 0));
  BLinearSystem(p, pkt_, p1, p2);

  // Set pz and E for central final states
  M4Vec sumP(0, 0, 0, 0);
  for (const auto &i : indices(p)) {
    const double m  = lts.decaytree[i].m_offshell;  // Note offshell!
    const double mt = msqrt(pow2(m) + p[i].Pt2());
    p[i].SetPzE(mt * std::sinh(y[i]), mt * std::cosh(y[i]));
    sumP += p[i];
  }

  // Check crude energy overflow
  if (sumP.E() > lts.sqrt_s) { return false; }

  double p1z = gra::kinematics::SolvePz(m1, m2, pt1, pt2, sumP.Pz(), sumP.E(), lts.s);
  double p2z = -(sumP.Pz() + p1z);  // by momentum conservation

  // Enforce scattering direction +p -> +p, -p -> -p (VERY RARE POLYNOMIAL
  // BRANCH FLIP)
  if (p1z < 0 || p2z > 0) { return false; }

  // pz and E of protons/N*
  p1.SetPzE(p1z, msqrt(pow2(m1) + pow2(pt1) + pow2(p1z)));
  p2.SetPzE(p2z, msqrt(pow2(m2) + pow2(pt2) + pow2(p2z)));

  // ------------------------------------------------------------------
  // Now boost if asymmetric beams
  if (std::abs(beamsum.Pz()) > 1e-6) {
    constexpr int sign = 1;  // positive -> boost to the lab
    kinematics::LorentzBoost(beamsum, lts.sqrt_s, p1, sign);
    kinematics::LorentzBoost(beamsum, lts.sqrt_s, p2, sign);
    kinematics::LorentzBoost(beamsum, lts.sqrt_s, sumP, sign);

    for (const auto &i : indices(p)) { kinematics::LorentzBoost(beamsum, lts.sqrt_s, p[i], sign); }
  }
  // ------------------------------------------------------------------

  // First branch kinematics
  lts.pfinal[1] = p1;  // Forward systems
  lts.pfinal[2] = p2;
  lts.pfinal[0] = sumP;  // Central system

  double             sumM   = 0;
  const unsigned int offset = 3;
  for (const auto &i : indices(p)) {
    lts.decaytree[i].p4    = p[i];
    lts.pfinal[i + offset] = p[i];
    sumM += p[i].M();
  }

  // -------------------------------------------------------------------
  // Kinematic checks

  // Check we are above mass threshold
  if (sumP.M() < sumM) { return false; }

  // Total 4-momentum conservation
  if (!gra::math::CheckEMC(beamsum - (lts.pfinal[1] + lts.pfinal[2] + lts.pfinal[0]))) {
    return false;
  }

  // -------------------------------------------------------------------

  // Treat decaytree recursively
  for (const auto &i : indices(lts.decaytree)) {
    if (!ConstructDecayKinematics(lts.decaytree[i])) { return false; }
  }

  // Forward system fragmentation
  if (vetocuts.active) {  // N.B. need to fragment already now!, if cuts active
    if (!CEPForwardFragment()) { return false; }
  }

  return GetLorentzScalars(Nf);
}

// Construct the linear system
void MContinuum::BLinearSystem(std::vector<M4Vec> &p, const std::vector<M4Vec> &q, const M4Vec &p1f,
                               const M4Vec &p2f) const {
  /*

  w = p1f + p2f

  -------------------------------
  KF = 4

  2p0 = ( q0 - w - p2 - p3)
  2p1 = (-q0 - w - p2 - p3)
  2p2 = (-q1 - w - p0 - p3)
  2p3 = (-q2 - w - p0 - p1)

  [2 0 1 1
   0 2 1 1
   1 0 2 1
   1 1 0 2]

  -------------------------------
  KF = 3

  2p0 = ( q0 - w - p2)
  2p1 = (-q0 - w - p2)
  2p2 = (-q1 - w - p0)

  [2 0 1
   0 2 1
   1 0 2]

  -------------------------------
  KF = 2

  2p0 = ( q0 - w)
  2p1 = (-q0 - w)

  [2 0
   0 2]

  -------------------------------
  */

  static const std::vector<std::vector<std::vector<double>>> A = {
      {{1.0 / 2.0, 0.0}, {0.0, 1.0 / 2.0}},

      {{2.0 / 3.0, 0.0, -1.0 / 3.0},
       {1.0 / 6.0, 1.0 / 2.0, -1.0 / 3.0},
       {-1.0 / 3.0, 0.0, 2.0 / 3.0}},

      {{7.0 / 8.0, 1.0 / 8.0, -1.0 / 2.0, -1.0 / 4.0},
       {3.0 / 8.0, 5.0 / 8.0, -1.0 / 2.0, -1.0 / 4.0},
       {-1.0 / 8.0, 1.0 / 8.0, 1.0 / 2.0, -1.0 / 4.0},
       {-5.0 / 8.0, -3.0 / 8.0, 1.0 / 2.0, 3.0 / 4.0}},

      {{11.0 / 10.0, 3.0 / 10.0, -3.0 / 5.0, -2.0 / 5.0, -1.0 / 5.0},
       {3.0 / 5.0, 4.0 / 5.0, -3.0 / 5.0, -2.0 / 5.0, -1.0 / 5.0},
       {1.0 / 10.0, 3.0 / 10.0, 2.0 / 5.0, -2.0 / 5.0, -1.0 / 5.0},
       {-2.0 / 5.0, -1.0 / 5.0, 2.0 / 5.0, 3.0 / 5.0, -1.0 / 5.0},
       {-9.0 / 10.0, -7.0 / 10.0, 2.0 / 5.0, 3.0 / 5.0, 4.0 / 5.0}},

      {{4.0 / 3.0, 1.0 / 2.0, -2.0 / 3.0, -1.0 / 2.0, -1.0 / 3.0, -1.0 / 6.0},
       {5.0 / 6.0, 1.0 / 1.0, -2.0 / 3.0, -1.0 / 2.0, -1.0 / 3.0, -1.0 / 6.0},
       {1.0 / 3.0, 1.0 / 2.0, 1.0 / 3.0, -1.0 / 2.0, -1.0 / 3.0, -1.0 / 6.0},
       {-1.0 / 6.0, 0.0, 1.0 / 3.0, 1.0 / 2.0, -1.0 / 3.0, -1.0 / 6.0},
       {-2.0 / 3.0, -1.0 / 2.0, 1.0 / 3.0, 1.0 / 2.0, 2.0 / 3.0, -1.0 / 6.0},
       {-7.0 / 6.0, -1.0 / 1.0, 1.0 / 3.0, 1.0 / 2.0, 2.0 / 3.0, 5.0 / 6.0}},

      {{11.0 / 7.0, 5.0 / 7.0, -5.0 / 7.0, -4.0 / 7.0, -3.0 / 7.0, -2.0 / 7.0, -1.0 / 7.0},
       {15.0 / 14.0, 17.0 / 14.0, -5.0 / 7.0, -4.0 / 7.0, -3.0 / 7.0, -2.0 / 7.0, -1.0 / 7.0},
       {4.0 / 7.0, 5.0 / 7.0, 2.0 / 7.0, -4.0 / 7.0, -3.0 / 7.0, -2.0 / 7.0, -1.0 / 7.0},
       {1.0 / 14.0, 3.0 / 14.0, 2.0 / 7.0, 3.0 / 7.0, -3.0 / 7.0, -2.0 / 7.0, -1.0 / 7.0},
       {-3.0 / 7.0, -2.0 / 7.0, 2.0 / 7.0, 3.0 / 7.0, 4.0 / 7.0, -2.0 / 7.0, -1.0 / 7.0},
       {-13.0 / 14.0, -11.0 / 14.0, 2.0 / 7.0, 3.0 / 7.0, 4.0 / 7.0, 5.0 / 7.0, -1.0 / 7.0},
       {-10.0 / 7.0, -9.0 / 7.0, 2.0 / 7.0, 3.0 / 7.0, 4.0 / 7.0, 5.0 / 7.0, 6.0 / 7.0}},

      {{29.0 / 16.0, 15.0 / 16.0, -3.0 / 4.0, -5.0 / 8.0, -1.0 / 2.0, -3.0 / 8.0, -1.0 / 4.0,
        -1.0 / 8.0},
       {21.0 / 16.0, 23.0 / 16.0, -3.0 / 4.0, -5.0 / 8.0, -1.0 / 2.0, -3.0 / 8.0, -1.0 / 4.0,
        -1.0 / 8.0},
       {13.0 / 16.0, 15.0 / 16.0, 1.0 / 4.0, -5.0 / 8.0, -1.0 / 2.0, -3.0 / 8.0, -1.0 / 4.0,
        -1.0 / 8.0},
       {5.0 / 16.0, 7.0 / 16.0, 1.0 / 4.0, 3.0 / 8.0, -1.0 / 2.0, -3.0 / 8.0, -1.0 / 4.0,
        -1.0 / 8.0},
       {-3.0 / 16.0, -1.0 / 16.0, 1.0 / 4.0, 3.0 / 8.0, 1.0 / 2.0, -3.0 / 8.0, -1.0 / 4.0,
        -1.0 / 8.0},
       {-11.0 / 16.0, -9.0 / 16.0, 1.0 / 4.0, 3.0 / 8.0, 1.0 / 2.0, 5.0 / 8.0, -1.0 / 4.0,
        -1.0 / 8.0},
       {-19.0 / 16.0, -17.0 / 16.0, 1.0 / 4.0, 3.0 / 8.0, 1.0 / 2.0, 5.0 / 8.0, 3.0 / 4.0,
        -1.0 / 8.0},
       {-27.0 / 16.0, -25.0 / 16.0, 1.0 / 4.0, 3.0 / 8.0, 1.0 / 2.0, 5.0 / 8.0, 3.0 / 4.0,
        7.0 / 8.0}}};

  // Construct vector b
  const unsigned int Kf = p.size();  // Number of central system particles
  std::vector<M4Vec> b(Kf);
  const M4Vec        p1p2sum = p1f + p2f;

  for (const auto &i : indices(b)) {
    if (i == 0) {
      b[i] = q[0] - p1p2sum;
    } else {
      b[i] = q[i - 1] * (-1.0) - p1p2sum;
    }
  }

  // Apply linear system p = Ab to get px,py components for each p[i]
  const unsigned int index = Kf - 2;  // -2 because of C++
  for (const auto &i : indices(p)) {
    for (const auto &j : indices(b)) {
      p[i] += b[j] * A[index][i][j];  // notice plus
    }
  }
}

// ----------------------------------------------------------------------
// MATLAB symbolic code to print out the system
// ----------------------------------------------------------------------
/*
% Number of particles in the central system
KFMAX = 8;

fprintf('static const std::vector<std::vector<std::vector<double>>> A = \n{ ');
for KF = 2:KFMAX

% Create system matrix A
A = ones(KF);
A(1:2,1:2) = [2 0; 0 2]; % Starting corner

% Lower triangle
for i = 1:KF-2
   m = ones(1,KF);
   m(i+1) = 0;
   m(i+2) = 2;
   A(2+i,:) = m;
end

% Invert
I = inv(sym(A));

% Plot C++ style
fprintf('\n');
fprintf('{');
for i = 1:size(A,1)
        fprintf('{');
        for j = 1:size(A,2)
                if (j < size(A,2))
                        mark = ', ';
                else
                        mark = '';
                end
                [N,D] = numden(I(i,j));
                if (N == 0)
                fprintf('0.0%s', mark);
                else
                fprintf('%0.1f/%0.1f%s', N, D, mark);
                end
        end
        if (i < size(A,2))
                mark = '},';
        else
                if (KF < KFMAX)
                        emark = ',';
                else
                        emark = '';
                end
                mark = sprintf('}}%s', emark);
        end
        fprintf('%s\n', mark);
end

end
fprintf('};\n');
*/

// (3*Nf-4)-Dim Integral Volume [phi1] x [phi2] x [phi_k1] x [phi_k2] x ... x
// [phi_{Kf-1}]
//                        [pt1] x [pt2] x [kt1] x [kt2] x ... x [kt{Kf-1}]
//                        x [y3] x [y4] x ... x [yN]
//
// For reference, Nf = 4 special case in:
// [REFERENCE: Lebiedowicz, Szczurek, arxiv.org/abs/0912.0190]
double MContinuum::BNIntegralVolume() const {
  // Number of central states
  const unsigned int Kf = lts.decaytree.size();

  // Forward leg integration
  const double forward_volume = ForwardVolume();

  // Intermediate kt volume
  double KT_vol = std::pow(gcuts.kt_max - gcuts.kt_min, Kf - 1);

  // phi angle volume
  KT_vol *= std::pow(2.0 * PI, Kf - 1);

  // rapidity volume
  KT_vol *= std::pow(gcuts.rap_max - gcuts.rap_min, Kf);

  return KT_vol * forward_volume;
}


double MContinuum::BNPhaseSpaceWeight() const {
  // Jacobian, close to constant 0.5
  const double J = 1.0 / std::abs(lts.pfinal[1].Pz() / lts.pfinal[1].E() -
                                  lts.pfinal[2].Pz() / lts.pfinal[2].E());

  // Number of final states
  const unsigned int Nf = lts.decaytree.size() + 2;

  // Intermediate "difference"
  // kt factors from \prod_i d^2 k_i = \prod_i dphi_i kt_i dkt_i
  double PROD = 1.0;
  for (const auto &i : indices(pkt_)) { PROD *= pkt_[i].Pt(); }

  const double factor = (1.0 / std::pow(2, 2 * (Nf - 2))) * (1.0 / std::pow(2.0 * PI, 3 * Nf - 4)) *
                        (lts.pfinal[1].Pt() / (2.0 * lts.pfinal[1].E())) *
                        (lts.pfinal[2].Pt() / (2.0 * lts.pfinal[2].E())) * J * PROD;
  return factor;
}

}  // namespace gra
