// QuasiElastic (EL,SD,DD) and soft ND (simplified) class
//
// (c) 2017-2021 Mikael Mieskolainen
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

// C++
#include <complex>
#include <iostream>
#include <random>
#include <vector>

// HepMC3
#include "HepMC3/FourVector.h"
#include "HepMC3/GenEvent.h"
#include "HepMC3/GenParticle.h"
#include "HepMC3/GenVertex.h"

// Own
#include "Graniitti/MAux.h"
#include "Graniitti/MForm.h"
#include "Graniitti/MKinematics.h"
#include "Graniitti/MMath.h"
#include "Graniitti/MProcess.h"
#include "Graniitti/MQuasiElastic.h"
#include "Graniitti/MUserCuts.h"

using gra::aux::indices;
using gra::math::abs2;
using gra::math::msqrt;
using gra::math::PI;
using gra::math::pow2;
using gra::math::zi;
using gra::PDG::GeV2barn;
using gra::PDG::mp;

namespace gra {
// This is needed by construction
MQuasiElastic::MQuasiElastic() { Initialize(); }

// Constructor
MQuasiElastic::MQuasiElastic(std::string process, const std::vector<aux::OneCMD> &syntax) {
  Initialize();
  InitHistograms();
  SetProcess(process, syntax);

  // Init final states
  M4Vec zerovec(0, 0, 0, 0);
  for (std::size_t i = 0; i < 10; ++i) { lts.pfinal.push_back(zerovec); }

  // Pomeron weights
  MAXPOMW = std::vector<double>(100, 0.0);
  std::cout << "MQuasiElastic:: [Constructor done]" << std::endl;
}

void MQuasiElastic::Initialize() {
  const std::vector<std::string> supported = {"X"};
  CID                                      = "Q";
  ProcPtr                                  = MSubProc(supported, CID);
}

// Destructor
MQuasiElastic::~MQuasiElastic() {}

// Initialize cut and process spesific postsetup
void MQuasiElastic::post_Constructor() {
  // Set phase space dimension
  if (ProcPtr.CHANNEL == "EL") { ProcPtr.LIPSDIM = 1; };
  if (ProcPtr.CHANNEL == "SD") { ProcPtr.LIPSDIM = 2; };
  if (ProcPtr.CHANNEL == "DD") { ProcPtr.LIPSDIM = 3; };
  if (ProcPtr.CHANNEL == "ND") { ProcPtr.LIPSDIM = 1; };  // Keep it 1

  Eikonal.Numerics.MaxLoopKT = 3.0;
}

// Fiducial user cuts
bool MQuasiElastic::FiducialCuts() const {
  if (fcuts.active == true) {
    // EL cuts
    if (ProcPtr.CHANNEL == "EL") {
      if (fcuts.forward_t_min < std::abs(lts.t) && std::abs(lts.t) < fcuts.forward_t_max) {
        // fine
      } else {
        return false;  // not fine
      }
    }

    // SD cuts
    if (ProcPtr.CHANNEL == "SD") {
      if (lts.pfinal[1].M() > 1.0) {  // this one is excited system
        if (fcuts.forward_M_min < lts.pfinal[1].M() && lts.pfinal[1].M() < fcuts.forward_M_max &&
            fcuts.forward_t_min < std::abs(lts.t) && std::abs(lts.t) < fcuts.forward_t_max) {
          // fine
        } else {
          return false;  // not fine
        }
      } else {
        if (fcuts.forward_M_min < lts.pfinal[2].M() && lts.pfinal[2].M() < fcuts.forward_M_max &&
            fcuts.forward_t_min < std::abs(lts.t) && std::abs(lts.t) < fcuts.forward_t_max) {
          // fine
        } else {
          return false;  // not fine
        }
      }
    }

    // DD cuts
    if (ProcPtr.CHANNEL == "DD") {
      if (fcuts.forward_M_min < lts.pfinal[1].M() && lts.pfinal[1].M() < fcuts.forward_M_max &&
          fcuts.forward_M_min < lts.pfinal[2].M() && lts.pfinal[2].M() < fcuts.forward_M_max &&
          fcuts.forward_t_min < std::abs(lts.t) && std::abs(lts.t) < fcuts.forward_t_max) {
        // fine
      } else {
        return false;  // not fine
      }
    }

    // Check user cuts (do not substitute to kinematics =
    // UserCut...)
    if (!UserCut(USERCUTS, lts)) {
      return false;  // not fine
    }
  }
  return true;  // fine
}

bool MQuasiElastic::LoopKinematics(const std::vector<double> &p1p, const std::vector<double> &p2p) {
  const M4Vec beamsum = lts.pbeam1 + lts.pbeam2;

  const double m1 = lts.pfinal[1].M();
  const double m2 = lts.pfinal[2].M();

  // SET new proton pT degrees of freedom
  lts.pfinal[1].SetPxPy(p1p[0], p1p[1]);
  lts.pfinal[2].SetPxPy(p2p[0], p2p[1]);

  // SOLVE pz and E
  const double p1z =
      kinematics::SolvePz(m1, m2, lts.pfinal[1].Pt(), lts.pfinal[2].Pt(), 0, 0, lts.s);
  const double p2z = -p1z;  // by momentum conservation

  // pz and E of protons/N*
  lts.pfinal[1].SetPzE(p1z, msqrt(pow2(m1) + pow2(lts.pfinal[1].Pt()) + pow2(p1z)));
  lts.pfinal[2].SetPzE(p2z, msqrt(pow2(m2) + pow2(lts.pfinal[2].Pt()) + pow2(p2z)));

  // ------------------------------------------------------------------
  // Now boost if asymmetric beams
  if (std::abs(beamsum.Pz()) > 1e-6) {
    constexpr int sign = 1;  // positive -> boost to the lab
    kinematics::LorentzBoost(beamsum, lts.sqrt_s, lts.pfinal[1], sign);
    kinematics::LorentzBoost(beamsum, lts.sqrt_s, lts.pfinal[2], sign);
  }
  // ------------------------------------------------------------------

  if (!gra::math::CheckEMC(beamsum - (lts.pfinal[1] + lts.pfinal[2]))) { return false; }

  return B3GetLorentzScalars();
}

// Get weight
double MQuasiElastic::EventWeight(const std::vector<double> &randvec, AuxIntData &aux) {
  double W = 0.0;

  if (ProcPtr.CHANNEL != "ND") {  // Diffractive

    aux.kinematics_ok = B3RandomKin(randvec);
    aux.fidcuts_ok    = FiducialCuts();
    aux.vetocuts_ok   = VetoCuts();

    if (aux.Valid()) {
      // ** EVENT WEIGHT **
      const double LIPS   = B3PhaseSpaceWeight();  // Phase-space weight
      const double MatESQ = GetAmp2();             // Matrix element squared

      // Total weight: phase-space x |M|^2 x barn units
      W = LIPS * B3IntegralVolume() * MatESQ * GeV2barn;
    }

    aux.amplitude_ok = CheckInfNan(W);

    // Fill Histograms
    // const double totalweight = W * aux.vegasweight;
    // FillHistograms(totalweight, lts); // This seqfaults, because we do not
    // have
    // enough observables
  } else {  // Non-Diffractive

    aux.kinematics_ok = true;  // This is always the case
    aux.fidcuts_ok    = true;
    aux.vetocuts_ok   = true;

    const double LIPS   = B3PhaseSpaceWeight();     // Phase-space weight
    const double MatESQ = abs2(PolySoft(randvec));  // Matrix element squared

    // W = LIPS * MatESQ * GeV2barn;  // Total weight: phase-space x |M|^2 x barn units
    W = LIPS * MatESQ;  // Total weight:

    aux.amplitude_ok = CheckInfNan(W);

    // Trigger forced event generation (using last component of auxvar)
    // (no need for outside acceptance-rejection with this process)
    if (W > 0) {
      aux.forced_accept = true;
    } else {
      aux.forced_accept = false;  // Failed event
    }
  }

  return W;
}

// Record HepMC3 event
bool MQuasiElastic::EventRecord(HepMC3::GenEvent &evt) {
  // Non-Diffractive
  if (ProcPtr.CHANNEL == "ND") {
    HepMC3::GenParticlePtr gen_p1;
    HepMC3::GenParticlePtr gen_p2;
    HepMC3::GenParticlePtr gen_p1f;
    HepMC3::GenParticlePtr gen_p2f;

    // Loop over multiple "cut pomerons"
    for (const auto &i : indices(etree)) {
      if (i == 0) {  // First

        // Initial state protons (4-momentum, pdg-id, status code)
        gen_p1 = std::make_shared<HepMC3::GenParticle>(
            gra::aux::M4Vec2HepMC3(etree[i].p1i), (i == 0) ? lts.beam1.pdg : PDG::PDG_fragment,
            (i == 0) ? PDG::PDG_BEAM : PDG::PDG_INTERMEDIATE);

        gen_p2 = std::make_shared<HepMC3::GenParticle>(
            gra::aux::M4Vec2HepMC3(etree[i].p2i), (i == 0) ? lts.beam2.pdg : PDG::PDG_fragment,
            (i == 0) ? PDG::PDG_BEAM : PDG::PDG_INTERMEDIATE);

      } else {  // Others recursively

        gen_p1 = gen_p1f;
        gen_p2 = gen_p2f;
      }

      // Final state (or intermediate in the chain) fragments
      gen_p1f = std::make_shared<HepMC3::GenParticle>(gra::aux::M4Vec2HepMC3(etree[i].p1f),
                                                      PDG::PDG_fragment, PDG::PDG_INTERMEDIATE);

      gen_p2f = std::make_shared<HepMC3::GenParticle>(gra::aux::M4Vec2HepMC3(etree[i].p2f),
                                                      PDG::PDG_fragment, PDG::PDG_INTERMEDIATE);

      // Exchange objects
      HepMC3::GenParticlePtr gen_q1 = std::make_shared<HepMC3::GenParticle>(
          gra::aux::M4Vec2HepMC3(etree[i].q1), PDG::PDG_propagator, PDG::PDG_INTERMEDIATE);
      HepMC3::GenParticlePtr gen_q2 = std::make_shared<HepMC3::GenParticle>(
          gra::aux::M4Vec2HepMC3(etree[i].q2), PDG::PDG_propagator, PDG::PDG_INTERMEDIATE);

      // Virtual state
      M4Vec system4vec(etree[i].k.Px(), etree[i].k.Py(), etree[i].k.Pz(), etree[i].k.E());

      HepMC3::GenParticlePtr gen_X = std::make_shared<HepMC3::GenParticle>(
          gra::aux::M4Vec2HepMC3(system4vec), PDG::PDG_system, PDG::PDG_INTERMEDIATE);

      HepMC3::GenVertexPtr vX = std::make_shared<HepMC3::GenVertex>();

      if (i != etree.size() - 1) {
        // Upper vertex
        HepMC3::GenVertexPtr vXUP = std::make_shared<HepMC3::GenVertex>();
        vXUP->add_particle_in(gen_p1);
        vXUP->add_particle_out(gen_p1f);
        vXUP->add_particle_out(gen_q1);

        // Lower vertex
        HepMC3::GenVertexPtr vXDO = std::make_shared<HepMC3::GenVertex>();
        vXDO->add_particle_in(gen_p2);
        vXDO->add_particle_out(gen_p2f);
        vXDO->add_particle_out(gen_q2);

        // Add vertices
        evt.add_vertex(vXUP);
        evt.add_vertex(vXDO);

        // Pomeron-Pomeron-Virtual state vertex
        vX->add_particle_in(gen_q1);
        vX->add_particle_in(gen_q2);
        vX->add_particle_out(gen_X);

      } else {  // Last MPI, fuse remnant + remnant -> system

        // Proton-Proton-Virtual state vertex
        vX = std::make_shared<HepMC3::GenVertex>();
        vX->add_particle_in(gen_p1);
        vX->add_particle_in(gen_p2);
        vX->add_particle_out(gen_X);
      }

      // Add vertex to the event
      evt.add_vertex(vX);

      // -----------------------------------------------------------------
      // Quantum numbers distributed here
      int    B        = 0;  // Baryon number
      int    Q        = 0;  // Charge
      double Q2_scale = system4vec.M2();

      // Beam fragments carry the initial state quantum numbers,
      // we distribute them here for the second last and the last MPI
      if (i == etree.size() - 2) {
        B = math::sign(lts.beam2.pdg);  // Proton(anti) proton beams only
        Q = B;
      }
      if (i == etree.size() - 1) {
        B = math::sign(lts.beam1.pdg);
        Q = B;
      }

      // Excite it
      MDecayBranch branch;
      if (!ExciteContinuum(system4vec, branch, Q2_scale, B, Q)) {
        std::cout << "ND failed with ExciteContinuum" << std::endl;
        return false;  // failed
      }

      // Save it
      SaveBranch(evt, branch, gen_X);
    }

    return true;
  }

  // Diffractive processes

  // Initial states (4-momentum, pdg-id, status code)
  HepMC3::GenParticlePtr gen_p1 = std::make_shared<HepMC3::GenParticle>(
      gra::aux::M4Vec2HepMC3(lts.pbeam1), lts.beam1.pdg, PDG::PDG_BEAM);
  HepMC3::GenParticlePtr gen_p2 = std::make_shared<HepMC3::GenParticle>(
      gra::aux::M4Vec2HepMC3(lts.pbeam2), lts.beam2.pdg, PDG::PDG_BEAM);

  // Propagator 4-vector and generator particle
  M4Vec                  q1(lts.pbeam1 - lts.pfinal[1]);
  HepMC3::GenParticlePtr gen_q1 = std::make_shared<HepMC3::GenParticle>(
      gra::aux::M4Vec2HepMC3(q1), PDG::PDG_propagator, PDG::PDG_INTERMEDIATE);

  // Final state protons/excited systems
  int PDG_ID1     = lts.beam1.pdg;
  int PDG_ID2     = lts.beam2.pdg;
  int PDG_status1 = PDG::PDG_STABLE;
  int PDG_status2 = PDG::PDG_STABLE;

  // EL
  // already fine

  // SD
  if (ProcPtr.CHANNEL == "SD") {
    if (lts.excite1) {  // proton 1 excited
      PDG_ID1     = std::abs(PDG::PDG_NSTAR) * math::sign(lts.beam1.pdg);
      PDG_status1 = PDG::PDG_INTERMEDIATE;
    } else {
      PDG_ID2     = std::abs(PDG::PDG_NSTAR) * math::sign(lts.beam2.pdg);
      PDG_status2 = PDG::PDG_INTERMEDIATE;
    }
  }
  // DD
  if (ProcPtr.CHANNEL == "DD") {
    PDG_ID1     = std::abs(PDG::PDG_NSTAR) * math::sign(lts.beam1.pdg);
    PDG_status1 = PDG::PDG_INTERMEDIATE;
    PDG_ID2     = std::abs(PDG::PDG_NSTAR) * math::sign(lts.beam2.pdg);
    PDG_status2 = PDG::PDG_INTERMEDIATE;
  }

  HepMC3::GenParticlePtr gen_p1f = std::make_shared<HepMC3::GenParticle>(
      gra::aux::M4Vec2HepMC3(lts.pfinal[1]), PDG_ID1, PDG_status1);
  HepMC3::GenParticlePtr gen_p2f = std::make_shared<HepMC3::GenParticle>(
      gra::aux::M4Vec2HepMC3(lts.pfinal[2]), PDG_ID2, PDG_status2);

  // Construct vertices

  // Upper proton-pomeron-proton
  HepMC3::GenVertexPtr v1 = std::make_shared<HepMC3::GenVertex>();
  v1->add_particle_in(gen_p1);
  v1->add_particle_out(gen_p1f);
  v1->add_particle_out(gen_q1);

  // Lower proton-pomeron-proton
  HepMC3::GenVertexPtr v2 = std::make_shared<HepMC3::GenVertex>();
  v2->add_particle_in(gen_p2);
  v2->add_particle_out(gen_p2f);
  v2->add_particle_in(gen_q1);

  // Finally add all vertices
  evt.add_vertex(v1);
  evt.add_vertex(v2);

  // Upper proton excitation
  if (lts.excite1) {
    const int B = math::sign(lts.beam1.pdg);  // baryon number the same as charge
    const int Q = B;

    if (!ExciteContinuum(lts.pfinal[1], lts.decayforward1, lts.pfinal[1].M2(), B, Q)) {
      return false;  // failed
    }
    SaveBranch(evt, lts.decayforward1, gen_p1f);
  }
  // Lower proton excitation
  if (lts.excite2) {
    const int B = math::sign(lts.beam2.pdg);  // baryon number the same as charge
    const int Q = B;

    if (!ExciteContinuum(lts.pfinal[2], lts.decayforward2, lts.pfinal[2].M2(), B, Q)) {
      return false;  // failed
    }
    SaveBranch(evt, lts.decayforward2, gen_p2f);
  }
  return true;
}


// Print out setup
void MQuasiElastic::PrintInit(bool silent) const {
  if (!silent) {
    PrintSetup();

    // Diffractive processes
    if (ProcPtr.CHANNEL != "ND") {
      std::string proton1 = "-----------EL--------->";
      std::string proton2 = "-----------EL--------->";

      if (ProcPtr.CHANNEL == "SD") { proton1 = "-----------F2-xxxxxxxx>"; }
      if (ProcPtr.CHANNEL == "DD") {
        proton1 = "-----------F2-xxxxxxxx>";
        proton2 = "-----------F2-xxxxxxxx>";
      }

      std::vector<std::string> feynmangraph;
      feynmangraph = {"||          ", "||          ", "||          ", "||          ",
                      "||          "};

      // Print diagram
      std::cout << proton1 << std::endl;
      for (const auto &i : indices(feynmangraph)) {
        if (SCREENING) {  // Put red
          std::cout << rang::fg::red << "     **    " << rang::style::reset;
        } else {
          std::cout << rang::fg::red << "           " << rang::style::reset;
        }
        std::cout << feynmangraph[i] << std::endl;
      }
      std::cout << proton2 << std::endl << std::endl;

      // Generation cuts
      std::cout << rang::style::bold << "Generation cuts:" << rang::style::reset << std::endl
                << std::endl;

      if (ProcPtr.CHANNEL != "EL") {
        printf("- Xi  [min, max]   = [%0.3E, %0.3E] (Xi == M^2/s) \n", gcuts.XI_min, gcuts.XI_max);
      }
      printf("- |t| [max]        = %0.3f GeV^2 \n", pow2(Eikonal.Numerics.MaxLoopKT));
      std::cout << std::endl;

      // Fiducial cuts
      std::cout << rang::style::bold << "Fiducial cuts:" << rang::style::reset << std::endl
                << std::endl;
      if (fcuts.active) {
        printf("- M   [min, max]   = [%0.2f, %0.2f] GeV^2 \n", fcuts.forward_M_min,
               fcuts.forward_M_max);
        printf("- |t| [min, max]   = [%0.2f, %0.2f] GeV^2 \n", fcuts.forward_t_min,
               fcuts.forward_t_max);
        std::cout << std::endl;
      } else {
        std::cout << "- Not active" << std::endl;
      }

      // Non-Diffractive
    } else {
      for (std::size_t i = 0; i < 7; ++i) {
        if (i > 0 && i < 6) {
          if (SCREENING) {
            std::cout << rang::fg::red << "     **    " << rang::style::reset;
          } else {
            std::cout << rang::fg::red << "           " << rang::style::reset;
          }
        } else {
          std::cout << "-----------";
        }
        std::cout << "|x|--------->" << std::endl;
      }
    }
    std::cout << std::endl;
  }
}

// N semi-independent (soft) Pomeron exchanges, 4-momentum conserved
//
// ------------------------------------> remnant 1
//      |      |       |          |
//      -- s0  |       -- s2      -- s(N-1)
// s    |      |       |          |
//      |      -- s1   |     ...  |
//      |      |       |          |
// ------------------------------------> remnant 2
//
//
// 1. Note that this problem is indeed quite sensitive to the (ad-hoc)
// beam-fragment
// treatment -> visible for example in multiplicity (double-NBD like)
// spectrum peak and shape etc. Here, we do not try to handle any color dof, but
// just
// put some boundary conditions and simple Regge-like x-dependence.
//
// 2. The energy evolution of the multiplicity spectrum density dN/deta is
// very sensitive to the Poisson-ansatz-eikonal process, naturally. Often seen
// easy powerlaw
// parametrization is not easy to obtain simultaneously with other observables.
// This just reminds that a very wide simultaneous set of observables is always
// needed
// <=> Rivet analysis.
//
std::complex<double> MQuasiElastic::PolySoft(const std::vector<double> &randvec) {
  // Boundary conditions
  const double XMIN = 1e-6;
  const double XMAX = 1.0;
  // const double XSTEP = 1e-5;
  const double MMIN       = 3.0;
  const double REM_M2_MIN = pow2(10.0);
  const double DELTA      = 0.10;

  int       outertrials = 0;
  const int MAXTRIAL    = 1e4;

  // Get the random number of inelastic cut Pomerons
  unsigned int N = 0;

  if (!Eikonal.IsInitialized()) {
    throw std::invalid_argument("MQuasiElastic::PolySoft: Set POMLOOP = true");
  }
  Eikonal.S3GetRandomCutsBt(N, bt, random.rng);

  // N = Eikonal.S3GetRandomCuts(random.rng);
  // printf("N = %d, bt = %0.3f \n", N, bt);

  // ------------------------------------------------------
  // etree.resize(N);
  etree.resize(N + 2);

  // Beams in
  etree[0].p1i = lts.pbeam1;  // @@@@@@
  etree[0].p2i = lts.pbeam2;  // @@@@@@

  // Dirichlet parameters
  const double        DIRALPHA = 0.85;
  std::vector<double> alphavec(N, DIRALPHA);  // "Uniform case"

  // Random integer from [1,NBins-1]
  // const double Q2 = 1.0;

  /*
  // Evaluate Proton Structure Function
  static std::vector<double> F2val(1/XSTEP, 0.0);
  static double MAXVAL = 0;
  for (const auto& n : indices(F2val)) {

                  const double xval = XMIN + n*XSTEP;
                  F2val[n] = gra::form::F2xQ2(xval, Q2);
                  MAXVAL = (F2val[n] > MAXVAL) ? F2val[n] : MAXVAL;
  }
  std::discrete_distribution<int> DIST(F2val.begin(), F2val.end());
  */

  while (true) {
    bool faulty = false;
    for (std::size_t i = 0; i < N; ++i) {
      const double s = (etree[i].p1i + etree[i].p2i).M2();

      // Draw Bjorken-x
      double x1 = 0;
      double x2 = 0;

      const double p1_m2 = etree[i].p1i.M2();
      const double p2_m2 = etree[i].p2i.M2();

      // No Q^2 dependence taken into account here
      // We assume xG(x) ~ 1/x^{DELTA} <=> G(x) = 1/x^{1+DELTA}
      x1 = random.PowerRandom(XMIN, XMAX, -(DELTA));
      x2 = random.PowerRandom(XMIN, XMAX, -(DELTA));

      /*
      // Structure function parametrization

      //x1 = XMIN + DIST(rng) * XSTEP;
      //x2 = XMIN + DIST(rng) * XSTEP;

      // Acceptance-Rejection
      while (true) {
              const int n = RANDI(rng);
              if (random.U(0,1) < F2val[n] / MAXVAL) {
                      x1 = XMIN + n * XSTEP;
                      break;
              }
      }
      while (true) {
              const int n = RANDI(rng);
              if (random.U(0,1) < F2val[n] / MAXVAL) {
                      x2 = XMIN + n * XSTEP;
                      break;
              }
      }
      */

      // Pick gaussian Fermi pt of Pomerons
      const double sigma = 0.4;  // GeV
      const double pt1   = msqrt(pow2(random.G(0, sigma)) + pow2(random.G(0, sigma)));
      const double pt2   = msqrt(pow2(random.G(0, sigma)) + pow2(random.G(0, sigma)));
      const double phi1  = random.U(0, 2.0 * PI);
      const double phi2  = random.U(0, 2.0 * PI);

      // Pick remnant mass^2
      double p3_m2 = p1_m2;
      double p4_m2 = p2_m2;

      if (i == N - 1) {  // Excite remnants (one could excite also intermediate)
        // Max operator for low energies
        p3_m2 = random.PowerRandom(REM_M2_MIN, pow2(etree[i].p1i.E()), -(1 + DELTA));
        p4_m2 = random.PowerRandom(REM_M2_MIN, pow2(etree[i].p2i.E()), -(1 + DELTA));
      }

      // Construct remnant protons
      etree[i].p1f.Set(pt1 * std::cos(phi1), pt1 * std::sin(phi1),
                       (x1 - 1) * msqrt(-4 * p1_m2 + s) / 2,
                       msqrt(p3_m2 + pow2(pt1) + (-p1_m2 + s / 4) * pow2(x1 - 1)));
      etree[i].p2f.Set(pt2 * std::cos(phi2), pt2 * std::sin(phi2),
                       -(x2 - 1) * msqrt(-4 * p2_m2 + s) / 2,
                       msqrt(p4_m2 + pow2(pt2) + (-p2_m2 + s / 4) * pow2(x2 - 1)));

      etree[i].q1 = etree[i].p1i - etree[i].p1f;
      etree[i].q2 = etree[i].p2i - etree[i].p2f;
      // etree[i].q1 = lts.pbeam1 - etree[i].p1f;
      // etree[i].q2 = lts.pbeam2 - etree[i].p2f;

      etree[i].k = etree[i].q1 + etree[i].q2;

      if (etree[i].k.M() < MMIN) {
        // printf("Faulty: M[i=%d/N=%d] = %0.1f [x1 = %0.4f, x2 = %0.4f]
        // \n", i, N, etree[i].k.M(), x1,x2);
        faulty = true;
        break;
      }

      // ----------------------------------------------------------------------
      // Evaluate matrix element (RESERVATION)
      // ...
      // ----------------------------------------------------------------------

      // Remnants out
      if (i < N - 1) {
        etree[i + 1].p1i = etree[i].p1f;  // @@@@@@
        etree[i + 1].p2i = etree[i].p2f;  // @@@@@@
      }
    }

    // Take forward and backward remnants energy-momentum
    etree[N].k     = etree[N - 1].p1f;
    etree[N + 1].k = etree[N - 1].p2f;

    // Check Energy-Momentum Conservation here explicitly !!!
    if (!faulty) {
      M4Vec p_sum;
      for (const auto &i : indices(etree)) {
        p_sum += etree[i].k;
        // printf("M[%i] = %0.3E \n", i, etree[i].k.M());
      }
      if (!gra::math::CheckEMC(p_sum - (lts.pbeam1 + lts.pbeam2))) {
        // printf("N = %d \n", N);
        p_sum.Print();
        faulty = true;
      }
    }

    if (faulty) {
      ++outertrials;
      if (outertrials > MAXTRIAL) {
        // std::cout << "MQuasiElastic::Random: Failed!" << std::endl;
        return 0.0;
      }
      continue;  // try again whole chain
    } else {
      // std::cout << "MQuasiElastic::Random: Success!" << std::endl;
      break;
    }
  }
  // printf("MQuasiElastic::PolySoft: OK with outertrials = %d \n",
  // outertrials);

  return 1.0;
}

// 3-dimensional phase space vector initialization
// The adaptive phase space boundaries here are taken into account by
// B3IntegralVolume() function.
bool MQuasiElastic::B3RandomKin(const std::vector<double> &randvec) {
  // Elastic case: s1 = s3, s2 = s4
  double s3 = lts.pbeam1.M2();
  double s4 = lts.pbeam2.M2();

  // Set Diffractive mass boundaries
  // neutron + piplus + safe margin
  M2_f_min = std::max(gcuts.XI_min * lts.s, gra::math::pow2(1.3));
  M2_f_max = gcuts.XI_max * lts.s;

  log_M2_f_min = std::log(M2_f_min);
  log_M2_f_max = std::log(M2_f_max);

  lts.excite1 = false;
  lts.excite2 = false;

  // Sample diffractive system masses
  if (ProcPtr.CHANNEL == "SD") {
    // Log-change of variable
    const double u = log_M2_f_min + (log_M2_f_max - log_M2_f_min) * randvec[1];
    const double r = std::exp(u);

    // Choose random permutation
    if (random.U(0, 1) < 0.5) {
      s3          = r;
      lts.excite1 = true;
      lts.excite2 = false;
    } else {
      s4          = r;
      lts.excite1 = false;
      lts.excite2 = true;
    }
  } else if (ProcPtr.CHANNEL == "DD") {
    // Log-change of variable
    const double u1 = log_M2_f_min + (log_M2_f_max - log_M2_f_min) * randvec[1];
    const double r1 = std::exp(u1);

    DD_M2_max     = gcuts.XI_max * (pow2(mp) * lts.s) / r1;
    log_DD_M2_max = std::log(DD_M2_max);

    // Log-change of variable
    const double u2 = log_M2_f_min + (log_DD_M2_max - log_M2_f_min) * randvec[2];
    const double r2 = std::exp(u2);

    // Choose random permutations
    if (random.U(0, 1) < 0.5) {
      s3 = r1;
      s4 = r2;
    } else {
      s4 = r1;
      s3 = r2;
    }
    lts.excite1 = true;
    lts.excite2 = true;
  }

  // Calculate kinematically valid t-range
  const double s1 = lts.pbeam1.M2();
  const double s2 = lts.pbeam2.M2();

  // Mandelstam t-range calculation
  gra::kinematics::Two2TwoLimit(lts.s, s1, s2, s3, s4, t_min, t_max);

  // Then limit the "diffraction cone" due to screening loop limit
  t_min = std::min(std::max(-pow2(Eikonal.Numerics.MaxLoopKT), t_min), t_max);

  // Logarithmic change of variable sampling
  const double A = std::abs(t_max);
  const double B = std::abs(t_min);

  const double r =
      std::log(A + ZERO_EPS) + (std::log(B + ZERO_EPS) - std::log(A + ZERO_EPS)) * randvec[0];
  const double t = -std::exp(r);

  return B3BuildKin(s3, s4, t);
}

// Build kinematics for elastic, single and double diffractive 2->2 quasielastic
bool MQuasiElastic::B3BuildKin(double s3, double s4, double t) {
  const double s1      = lts.pbeam1.M2();
  const double s2      = lts.pbeam2.M2();
  const M4Vec  beamsum = lts.pbeam1 + lts.pbeam2;

  // Scattering angle based on invariants
  double theta = std::acos(kinematics::CosthetaStar(lts.s, t, s1, s2, s3, s4));

  // Forward/backward solution flip (skip these, rare)
  if (std::cos(theta) < 0) { return false; }
  // theta = (std::cos(theta) < 0) ? gra::math::PI - theta : theta;

  // Outgoing 4-momentum by Kallen (triangle) function in the center-of-momentum
  // frame
  const double pnorm = kinematics::DecayMomentum(lts.sqrt_s, msqrt(s3), msqrt(s4));
  M4Vec        p3(0, 0, pnorm, 0.5 * (lts.s + s3 - s4) / lts.sqrt_s);
  M4Vec        p4(0, 0, -pnorm, 0.5 * (lts.s + s4 - s3) / lts.sqrt_s);

  // Transverse momentum by orienting with random rotation (theta,phi)
  const double phi = random.U(0.0, 2.0 * gra::math::PI);  // Flat phi
  gra::kinematics::Rotate(p3, theta, phi);
  gra::kinematics::Rotate(p4, theta, phi);

  // ------------------------------------------------------------------
  // Now boost if asymmetric beams
  if (std::abs(beamsum.Pz()) > 1e-6) {
    constexpr int sign = 1;  // positive -> boost to the lab
    kinematics::LorentzBoost(beamsum, lts.sqrt_s, p3, sign);
    kinematics::LorentzBoost(beamsum, lts.sqrt_s, p4, sign);
  }
  // ------------------------------------------------------------------

  lts.pfinal[1] = p3;
  lts.pfinal[2] = p4;

  // Check Energy-Momentum conservation
  if (!gra::math::CheckEMC(beamsum - (lts.pfinal[1] + lts.pfinal[2]))) { return false; }

  return B3GetLorentzScalars();
}

// Build and check scalars
bool MQuasiElastic::B3GetLorentzScalars() {
  // Calculate Lorentz scalars
  lts.ss[1][1] = lts.pfinal[1].M2();
  lts.ss[2][2] = lts.pfinal[2].M2();

  lts.t = (lts.pbeam1 - lts.pfinal[1]).M2();
  lts.u = (lts.pbeam1 - lts.pfinal[2]).M2();

  // Test scalars
  if (lts.ss[1][1] > lts.s) return false;
  if (lts.ss[2][2] > lts.s) return false;
  if (lts.t > 0) return false;
  if (lts.u > 0) return false;
  /*
  printf("s = %E, s^1/2 = %E \n", lts.s, msqrt(lts.s));
  printf("t = %E, u = %E \n", lts.t, lts.u);
  printf("s3 = %E, s4 = %E \n", lts.ss[1][1], lts.ss[2][2]);
  printf("\n");
  */
  return true;
}

// 1/2/3-Dim Integral Volume [t] x [M^2] x [M^2]
//
double MQuasiElastic::B3IntegralVolume() const {
  // Mandelstam t, log change of variable, volume times jacobian
  const double A     = std::abs(t_max);
  const double B     = std::abs(t_min);
  const double t_VOL = (std::log(B + ZERO_EPS) - std::log(A + ZERO_EPS)) * std::abs(lts.t);

  if (ProcPtr.CHANNEL == "EL") {
    return t_VOL;

  } else if (ProcPtr.CHANNEL == "SD") {
    // Jacobian from log-change of variable:
    // \int_a^b f(M2) dM2 = \int_{ln(a)}^{ln(b)} f(exp(u)) * exp(u) du, where u = ln(M2)
    const double J      = (lts.excite1) ? lts.ss[1][1] : lts.ss[2][2];
    const double M2_VOL = (log_M2_f_max - log_M2_f_min) * J;
    return t_VOL * M2_VOL;

  } else if (ProcPtr.CHANNEL == "DD") {
    // Jacobian from log-change of variables
    const double J      = lts.ss[1][1] * lts.ss[2][2];
    const double M2_VOL = (log_M2_f_max - log_M2_f_min) * (log_DD_M2_max - log_M2_f_min) * J;
    return t_VOL * M2_VOL;

  } else {
    return 0;
  }
}

// Standard phase space for m1 + m2 -> m3 + m4
//
double MQuasiElastic::B3PhaseSpaceWeight() const {
  // expression -> 16 * pi * s^2 (if s >> m1,m2)
  const double norm = 16.0 * gra::math::PI *
                      pow2(lts.s * gra::kinematics::beta12(lts.s, lts.beam1.mass, lts.beam2.mass));

  if (ProcPtr.CHANNEL == "EL") {
    return 1.0 / norm;
  } else if (ProcPtr.CHANNEL == "SD") {
    return 2.0 / norm;  // Factor of two in
                        // numerator from single
                        // diffraction left + right
  } else if (ProcPtr.CHANNEL == "DD") {
    return 1.0 / norm;
  } else if (ProcPtr.CHANNEL == "ND") {
    return 1.0;
  } else {
    return 0;
  }
}

}  // namespace gra
