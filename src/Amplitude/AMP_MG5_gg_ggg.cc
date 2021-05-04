//==========================================================================
// This file has been automatically generated for C++ Standalone by
// MadGraph5_aMC@NLO v. 2.6.7, 2019-10-16
// By the MadGraph5_aMC@NLO Development Team
// Visit launchpad.net/madgraph5 and amcatnlo.web.cern.ch
// @@@@ MadGraph to GRANIITTI conversion done @@@@
//==========================================================================

#include "Graniitti/Amplitude/AMP_MG5_gg_ggg.h"

#include "Graniitti/Amplitude/HelAmps_sm.h"

using namespace MG5_sm;

//==========================================================================
// Class member functions for calculating the matrix elements for
// Process: g g > g g g WEIGHTED<=3 @1

//--------------------------------------------------------------------------
// Initialize process.

void AMP_MG5_gg_ggg::initProc(string param_card_name) {
  // Instantiate the model class and set parameters that stay fixed during run
  pars = Parameters_sm();  // GRANIITTI
  SLHAReader slha(param_card_name);
  pars.setIndependentParameters(slha);
  pars.setIndependentCouplings();
  // pars.printIndependentParameters(); // GRANIITTI
  // pars.printIndependentCouplings(); // GRANIITTI
  // Set external particle masses for this matrix element
  mME.push_back(pars.ZERO);
  mME.push_back(pars.ZERO);
  mME.push_back(pars.ZERO);
  mME.push_back(pars.ZERO);
  mME.push_back(pars.ZERO);
  jamp2[0] = new double[24];
}

//--------------------------------------------------------------------------
// Evaluate |M|^2, part independent of incoming flavour.

double AMP_MG5_gg_ggg::CalcAmp2(gra::LORENTZSCALAR &lts, double alphas) {
  // Set the parameters which change event by event
  pars.setDependentParameters(alphas);
  if (gra::PARAM_STRUCTURE::QED_alpha == "ZERO")
    pars.setAlphaQEDZero();  // GRANIITTI: Set at scale alpha_QED(Q2=0)

  pars.setDependentCouplings();
  static bool firsttime = false;  // GRANIITTI
  if (firsttime) {
    pars.printDependentParameters();
    pars.printDependentCouplings();
    firsttime = false;
  }

  // Reset color flows
  for (int i = 0; i < 24; i++) jamp2[0][i] = 0.;

  // Local variables and constants

  // @@@@@@@@@@@@@@@@@@@@@@@@@@ GRANIITTI @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  // *** MADGRAPH CONVENTION IS [E,px,py,pz] ! ***

  // *** Set masses for HELAS ***
  std::vector<double>     masses = {0, 0};  // Massless two initial states
  std::vector<gra::M4Vec> pf;

  for (std::size_t i = 0; i < lts.decaytree.size(); ++i) {
    masses.push_back(lts.decaytree[i].p4.M());
    pf.push_back(lts.decaytree[i].p4);
  }
  mME = masses;

  gra::M4Vec p1_ = lts.q1;
  gra::M4Vec p2_ = lts.q2;


  // Do kinematic transform
  gra::kinematics::OffShell2LightCone(p1_, p2_, pf);

  // Set initial state 4-momentum
  p.clear();
  double p1[] = {p1_.E(), p1_.Px(), p1_.Py(), p1_.Pz()};
  p.push_back(&p1[0]);

  double p2[] = {p2_.E(), p2_.Px(), p2_.Py(), p2_.Pz()};
  p.push_back(&p2[0]);

  // Set final state 4-momentum
  double pthis[pf.size()][4];
  for (std::size_t i = 0; i < pf.size(); ++i) {
    pthis[i][0] = pf[i].E();
    pthis[i][1] = pf[i].Px();
    pthis[i][2] = pf[i].Py();
    pthis[i][3] = pf[i].Pz();
    p.push_back(&pthis[i][0]);
  }
  // @@@@@@@@@@@@@@@@@@@@@@@@@@ GRANIITTI @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  const int ncomb = 32;
  lts.hamp        = std::vector<std::complex<double>>(ncomb);  // GRANIITTI

  static bool            goodhel[ncomb] = {ncomb * false};
  static int             ntry = 0, sum_hel = 0, ngood = 0;
  static int             igood[ncomb];
  static int             jhel;
  std::complex<double> **wfs;
  double                 t[nprocesses];
  // Helicities for the process
  static const int helicities[ncomb][nexternal] = {
      {-1, -1, -1, -1, -1}, {-1, -1, -1, -1, 1}, {-1, -1, -1, 1, -1}, {-1, -1, -1, 1, 1},
      {-1, -1, 1, -1, -1},  {-1, -1, 1, -1, 1},  {-1, -1, 1, 1, -1},  {-1, -1, 1, 1, 1},
      {-1, 1, -1, -1, -1},  {-1, 1, -1, -1, 1},  {-1, 1, -1, 1, -1},  {-1, 1, -1, 1, 1},
      {-1, 1, 1, -1, -1},   {-1, 1, 1, -1, 1},   {-1, 1, 1, 1, -1},   {-1, 1, 1, 1, 1},
      {1, -1, -1, -1, -1},  {1, -1, -1, -1, 1},  {1, -1, -1, 1, -1},  {1, -1, -1, 1, 1},
      {1, -1, 1, -1, -1},   {1, -1, 1, -1, 1},   {1, -1, 1, 1, -1},   {1, -1, 1, 1, 1},
      {1, 1, -1, -1, -1},   {1, 1, -1, -1, 1},   {1, 1, -1, 1, -1},   {1, 1, -1, 1, 1},
      {1, 1, 1, -1, -1},    {1, 1, 1, -1, 1},    {1, 1, 1, 1, -1},    {1, 1, 1, 1, 1}};
  // Denominators: spins, colors and identical particles
  const int denominators[nprocesses] = {1536};

  ntry = 1;  // GRANIITTI

  // Reset the matrix elements
  for (int i = 0; i < nprocesses; i++) { matrix_element[i] = 0.; }
  // Define permutation
  int perm[nexternal];
  for (int i = 0; i < nexternal; i++) { perm[i] = i; }

  goto SKIPLABEL;  // GRANIITTI: Skip this block
  if (sum_hel == 0 || ntry < 10) {
    // Calculate the matrix element for all helicities
    for (int ihel = 0; ihel < ncomb; ihel++) {
      if (goodhel[ihel] || ntry < 2) {
        calculate_wavefunctions(perm, helicities[ihel]);
        t[0] = matrix_1_gg_ggg();

        double tsum = 0;
        for (int iproc = 0; iproc < nprocesses; iproc++) {
          matrix_element[iproc] += t[iproc];
          tsum += t[iproc];
        }
        // Store which helicities give non-zero result
        if (tsum != 0. && !goodhel[ihel]) {
          goodhel[ihel] = true;
          ngood++;
          igood[ngood] = ihel;
        }
      }
    }
    jhel    = 0;
    sum_hel = min(sum_hel, ngood);
  } else {
    // Only use the "good" helicities
    for (int j = 0; j < sum_hel; j++) {
      jhel++;
      if (jhel >= ngood) jhel = 0;
      double hwgt = double(ngood) / double(sum_hel);
      int    ihel = igood[jhel];
      calculate_wavefunctions(perm, helicities[ihel]);
      t[0] = matrix_1_gg_ggg();

      for (int iproc = 0; iproc < nprocesses; iproc++) { matrix_element[iproc] += t[iproc] * hwgt; }
    }
  }

  for (int i = 0; i < nprocesses; i++) matrix_element[i] /= denominators[i];

SKIPLABEL:

  // @@@@@@@@@@@@@@@@@@@@@@@@@@ GRANIITTI @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  // Define permutation
  for (int i = 0; i < nexternal; ++i) { perm[i] = i; }

  // Loop over helicity combinations
  for (int ihel = 0; ihel < ncomb; ++ihel) {
    calculate_wavefunctions(perm, helicities[ihel]);

    // Sum of subamplitudes (s,t,u,...)
    for (int k = 0; k < namplitudes; ++k) { lts.hamp[ihel] += amp[k]; }
  }

  // Total amplitude squared over all helicity combinations individually
  double amp2 = 0.0;
  for (int ihel = 0; ihel < ncomb; ++ihel) { amp2 += gra::math::abs2(lts.hamp[ihel]); }
  amp2 /= denominators[0];  // spin average matrix element squared

  return amp2;  // amplitude squared
                // @@@@@@@@@@@@@@@@@@@@@@@@@@ GRANIITTI @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
}

//--------------------------------------------------------------------------
// Evaluate |M|^2, including incoming flavour dependence.

double AMP_MG5_gg_ggg::sigmaHat() {
  // Select between the different processes
  if (id1 == 21 && id2 == 21) {
    // Add matrix elements for processes with beams (21, 21)
    return matrix_element[0];
  } else {
    // Return 0 if not correct initial state assignment
    return 0.;
  }
}

//==========================================================================
// Private class member functions

//--------------------------------------------------------------------------
// Evaluate |M|^2 for each subprocess

void AMP_MG5_gg_ggg::calculate_wavefunctions(const int perm[], const int hel[]) {
  // Calculate wavefunctions for all processes
  int i, j;

  // Calculate all wavefunctions
  vxxxxx(p[perm[0]], mME[0], hel[0], -1, w[0]);
  vxxxxx(p[perm[1]], mME[1], hel[1], -1, w[1]);
  vxxxxx(p[perm[2]], mME[2], hel[2], +1, w[2]);
  vxxxxx(p[perm[3]], mME[3], hel[3], +1, w[3]);
  vxxxxx(p[perm[4]], mME[4], hel[4], +1, w[4]);
  VVV1P0_1(w[0], w[1], pars.GC_10, pars.ZERO, pars.ZERO, w[5]);
  VVV1P0_1(w[2], w[3], pars.GC_10, pars.ZERO, pars.ZERO, w[6]);
  VVV1P0_1(w[2], w[4], pars.GC_10, pars.ZERO, pars.ZERO, w[7]);
  VVV1P0_1(w[3], w[4], pars.GC_10, pars.ZERO, pars.ZERO, w[8]);
  VVV1P0_1(w[0], w[2], pars.GC_10, pars.ZERO, pars.ZERO, w[9]);
  VVV1P0_1(w[1], w[3], pars.GC_10, pars.ZERO, pars.ZERO, w[10]);
  VVV1P0_1(w[1], w[4], pars.GC_10, pars.ZERO, pars.ZERO, w[11]);
  VVV1P0_1(w[0], w[3], pars.GC_10, pars.ZERO, pars.ZERO, w[12]);
  VVV1P0_1(w[1], w[2], pars.GC_10, pars.ZERO, pars.ZERO, w[13]);
  VVV1P0_1(w[0], w[4], pars.GC_10, pars.ZERO, pars.ZERO, w[14]);
  VVVV1P0_1(w[0], w[1], w[2], pars.GC_12, pars.ZERO, pars.ZERO, w[15]);
  VVVV3P0_1(w[0], w[1], w[2], pars.GC_12, pars.ZERO, pars.ZERO, w[16]);
  VVVV4P0_1(w[0], w[1], w[2], pars.GC_12, pars.ZERO, pars.ZERO, w[17]);
  VVVV1P0_1(w[0], w[1], w[3], pars.GC_12, pars.ZERO, pars.ZERO, w[18]);
  VVVV3P0_1(w[0], w[1], w[3], pars.GC_12, pars.ZERO, pars.ZERO, w[19]);
  VVVV4P0_1(w[0], w[1], w[3], pars.GC_12, pars.ZERO, pars.ZERO, w[20]);
  VVVV1P0_1(w[0], w[1], w[4], pars.GC_12, pars.ZERO, pars.ZERO, w[21]);
  VVVV3P0_1(w[0], w[1], w[4], pars.GC_12, pars.ZERO, pars.ZERO, w[22]);
  VVVV4P0_1(w[0], w[1], w[4], pars.GC_12, pars.ZERO, pars.ZERO, w[23]);
  VVVV1P0_1(w[0], w[2], w[3], pars.GC_12, pars.ZERO, pars.ZERO, w[24]);
  VVVV3P0_1(w[0], w[2], w[3], pars.GC_12, pars.ZERO, pars.ZERO, w[25]);
  VVVV4P0_1(w[0], w[2], w[3], pars.GC_12, pars.ZERO, pars.ZERO, w[26]);
  VVVV1P0_1(w[0], w[2], w[4], pars.GC_12, pars.ZERO, pars.ZERO, w[27]);
  VVVV3P0_1(w[0], w[2], w[4], pars.GC_12, pars.ZERO, pars.ZERO, w[28]);
  VVVV4P0_1(w[0], w[2], w[4], pars.GC_12, pars.ZERO, pars.ZERO, w[29]);
  VVVV1P0_1(w[0], w[3], w[4], pars.GC_12, pars.ZERO, pars.ZERO, w[30]);
  VVVV3P0_1(w[0], w[3], w[4], pars.GC_12, pars.ZERO, pars.ZERO, w[31]);
  VVVV4P0_1(w[0], w[3], w[4], pars.GC_12, pars.ZERO, pars.ZERO, w[32]);

  // Calculate all amplitudes
  // Amplitude(s) for diagram number 0
  VVV1_0(w[5], w[6], w[4], pars.GC_10, amp[0]);
  VVV1_0(w[5], w[7], w[3], pars.GC_10, amp[1]);
  VVV1_0(w[5], w[2], w[8], pars.GC_10, amp[2]);
  VVVV1_0(w[2], w[3], w[4], w[5], pars.GC_12, amp[3]);
  VVVV3_0(w[2], w[3], w[4], w[5], pars.GC_12, amp[4]);
  VVVV4_0(w[2], w[3], w[4], w[5], pars.GC_12, amp[5]);
  VVV1_0(w[9], w[10], w[4], pars.GC_10, amp[6]);
  VVV1_0(w[9], w[11], w[3], pars.GC_10, amp[7]);
  VVV1_0(w[9], w[1], w[8], pars.GC_10, amp[8]);
  VVVV1_0(w[1], w[3], w[4], w[9], pars.GC_12, amp[9]);
  VVVV3_0(w[1], w[3], w[4], w[9], pars.GC_12, amp[10]);
  VVVV4_0(w[1], w[3], w[4], w[9], pars.GC_12, amp[11]);
  VVV1_0(w[12], w[13], w[4], pars.GC_10, amp[12]);
  VVV1_0(w[12], w[11], w[2], pars.GC_10, amp[13]);
  VVV1_0(w[12], w[1], w[7], pars.GC_10, amp[14]);
  VVVV1_0(w[1], w[2], w[4], w[12], pars.GC_12, amp[15]);
  VVVV3_0(w[1], w[2], w[4], w[12], pars.GC_12, amp[16]);
  VVVV4_0(w[1], w[2], w[4], w[12], pars.GC_12, amp[17]);
  VVV1_0(w[14], w[13], w[3], pars.GC_10, amp[18]);
  VVV1_0(w[14], w[10], w[2], pars.GC_10, amp[19]);
  VVV1_0(w[14], w[1], w[6], pars.GC_10, amp[20]);
  VVVV1_0(w[1], w[2], w[3], w[14], pars.GC_12, amp[21]);
  VVVV3_0(w[1], w[2], w[3], w[14], pars.GC_12, amp[22]);
  VVVV4_0(w[1], w[2], w[3], w[14], pars.GC_12, amp[23]);
  VVV1_0(w[0], w[13], w[8], pars.GC_10, amp[24]);
  VVV1_0(w[0], w[10], w[7], pars.GC_10, amp[25]);
  VVV1_0(w[0], w[11], w[6], pars.GC_10, amp[26]);
  VVV1_0(w[3], w[4], w[15], pars.GC_10, amp[27]);
  VVV1_0(w[3], w[4], w[16], pars.GC_10, amp[28]);
  VVV1_0(w[3], w[4], w[17], pars.GC_10, amp[29]);
  VVV1_0(w[2], w[4], w[18], pars.GC_10, amp[30]);
  VVV1_0(w[2], w[4], w[19], pars.GC_10, amp[31]);
  VVV1_0(w[2], w[4], w[20], pars.GC_10, amp[32]);
  VVV1_0(w[2], w[3], w[21], pars.GC_10, amp[33]);
  VVV1_0(w[2], w[3], w[22], pars.GC_10, amp[34]);
  VVV1_0(w[2], w[3], w[23], pars.GC_10, amp[35]);
  VVV1_0(w[1], w[4], w[24], pars.GC_10, amp[36]);
  VVV1_0(w[1], w[4], w[25], pars.GC_10, amp[37]);
  VVV1_0(w[1], w[4], w[26], pars.GC_10, amp[38]);
  VVV1_0(w[1], w[3], w[27], pars.GC_10, amp[39]);
  VVV1_0(w[1], w[3], w[28], pars.GC_10, amp[40]);
  VVV1_0(w[1], w[3], w[29], pars.GC_10, amp[41]);
  VVV1_0(w[1], w[2], w[30], pars.GC_10, amp[42]);
  VVV1_0(w[1], w[2], w[31], pars.GC_10, amp[43]);
  VVV1_0(w[1], w[2], w[32], pars.GC_10, amp[44]);
}
double AMP_MG5_gg_ggg::matrix_1_gg_ggg() {
  int i, j;
  // Local variables
  const int            ngraphs = 45;
  const int            ncolor  = 24;
  std::complex<double> ztemp;
  std::complex<double> jamp[ncolor];
  // The color matrix;
  static const double denom[ncolor] = {108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108,
                                       108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108, 108};
  static const double cf[ncolor][ncolor] = {{455, -58, -58, 14, 14, 68, -58, -4, 14, -58, 5,  -4,
                                             14,  5,   68,  -4, 14, 68, -58, -4, -4, 68,  68, -40},
                                            {-58, 455, 14, 68, -58, 14,  -4, -58, 5,  -4, 14, -58,
                                             -58, -4,  -4, 68, 68,  -40, 14, 5,   68, -4, 14, 68},
                                            {-58, 14, 455, -58, 68, 14, 14, 5,   68, -4,  14, 68,
                                             -58, -4, 14,  -58, 5,  -4, -4, -58, 68, -40, -4, 68},
                                            {14, 68,  -58, 455, 14, -58, -58, -4, -4, 68, 68, -40,
                                             -4, -58, 5,   -4,  14, -58, 5,   14, 14, 68, 68, -4},
                                            {14, -58, 68, 14,  455, -58, 5,   14, 14, 68,  68, -4,
                                             -4, -58, 68, -40, -4,  68,  -58, -4, 14, -58, 5,  -4},
                                            {68, 14, 14, -58, -58, 455, -4, -58, 68, -40, -4, 68,
                                             5,  14, 14, 68,  68,  -4,  -4, -58, 5,  -4,  14, -58},
                                            {-58, -4, 14, -58, 5,  -4, 455, -58, -58, 14, 14,  68,
                                             68,  -4, 14, 5,   68, 14, -4,  68,  -58, -4, -40, 68},
                                            {-4, -58, 5,   -4, 14,  -58, -58, 455, 14, 68, -58, 14,
                                             -4, 68,  -58, -4, -40, 68,  68,  -4,  14, 5,  68,  14},
                                            {14, 5,   68,  -4, 14, 68, -58, 14,  455, -58, 68, 14,
                                             14, -58, -58, -4, -4, 5,  68,  -40, -4,  -58, 68, -4},
                                            {-58, -4, -4, 68,  68,  -40, 14, 68, -58, 455, 14, -58,
                                             5,   -4, -4, -58, -58, 14,  14, 68, 5,   14,  -4, 68},
                                            {5,  14,  14, 68,  68, -4, 14, -58, 68,  14, 455, -58,
                                             68, -40, -4, -58, 68, -4, 14, -58, -58, -4, -4,  5},
                                            {-4, -58, 68, -40, -4, 68, 68, 14, 14, -58, -58, 455,
                                             14, 68,  5,  14,  -4, 68, 5,  -4, -4, -58, -58, 14},
                                            {14,  -58, -58, -4, -4, 5,  68, -4, 14,  5,  68,  14,
                                             455, -58, -58, 14, 14, 68, 68, -4, -40, 68, -58, -4},
                                            {5,   -4,  -4, -58, -58, 14, -4, 68, -58, -4, -40, 68,
                                             -58, 455, 14, 68,  -58, 14, -4, 68, 68,  14, 14,  5},
                                            {68,  -4, 14,  5,   68, 14, 14,  -58, -58, -4, -4, 5,
                                             -58, 14, 455, -58, 68, 14, -40, 68,  68,  -4, -4, -58},
                                            {-4, 68, -58, -4,  -40, 68,  5,  -4, -4, -58, -58, 14,
                                             14, 68, -58, 455, 14,  -58, 68, 14, -4, 68,  5,   14},
                                            {14, 68,  5,  14, -4,  68,  68,  -40, -4, -58, 68,  -4,
                                             14, -58, 68, 14, 455, -58, -58, 14,  -4, 5,   -58, -4},
                                            {68, -40, -4, -58, 68,  -4,  14, 68, 5,   14, -4, 68,
                                             68, 14,  14, -58, -58, 455, -4, 5,  -58, 14, -4, -58},
                                            {-58, 14, -4,  5,  -58, -4, -4,  68,  68,  14, 14, 5,
                                             68,  -4, -40, 68, -58, -4, 455, -58, -58, 14, 14, 68},
                                            {-4, 5,  -58, 14, -4, -58, 68,  -4,  -40, 68, -58, -4,
                                             -4, 68, 68,  14, 14, 5,   -58, 455, 14,  68, -58, 14},
                                            {-4,  68, 68, 14, 14, 5,   -58, 14, -4,  5,   -58, -4,
                                             -40, 68, 68, -4, -4, -58, -58, 14, 455, -58, 68,  14},
                                            {68, -4, -40, 68, -58, -4, -4, 5,  -58, 14,  -4, -58,
                                             68, 14, -4,  68, 5,   14, 14, 68, -58, 455, 14, -58},
                                            {68,  14, -4, 68, 5,   14, -40, 68,  68, -4, -4,  -58,
                                             -58, 14, -4, 5,  -58, -4, 14,  -58, 68, 14, 455, -58},
                                            {-40, 68, 68,  -4, -4, -58, 68, 14, -4, 68,  5,   14,
                                             -4,  5,  -58, 14, -4, -58, 68, 14, 14, -58, -58, 455}};

  // Calculate color flows
  jamp[0]  = +2. * (+std::complex<double>(0, 1) * amp[0] + std::complex<double>(0, 1) * amp[2] +
                   std::complex<double>(0, 1) * amp[3] - std::complex<double>(0, 1) * amp[5] -
                   std::complex<double>(0, 1) * amp[18] - std::complex<double>(0, 1) * amp[20] -
                   std::complex<double>(0, 1) * amp[21] + std::complex<double>(0, 1) * amp[23] +
                   std::complex<double>(0, 1) * amp[24] - std::complex<double>(0, 1) * amp[29] +
                   std::complex<double>(0, 1) * amp[27] + std::complex<double>(0, 1) * amp[35] +
                   std::complex<double>(0, 1) * amp[34] - std::complex<double>(0, 1) * amp[43] -
                   std::complex<double>(0, 1) * amp[42]);
  jamp[1]  = +2. * (+std::complex<double>(0, 1) * amp[1] - std::complex<double>(0, 1) * amp[2] +
                   std::complex<double>(0, 1) * amp[4] + std::complex<double>(0, 1) * amp[5] -
                   std::complex<double>(0, 1) * amp[12] - std::complex<double>(0, 1) * amp[14] -
                   std::complex<double>(0, 1) * amp[15] + std::complex<double>(0, 1) * amp[17] -
                   std::complex<double>(0, 1) * amp[24] + std::complex<double>(0, 1) * amp[29] -
                   std::complex<double>(0, 1) * amp[27] + std::complex<double>(0, 1) * amp[32] +
                   std::complex<double>(0, 1) * amp[31] - std::complex<double>(0, 1) * amp[44] +
                   std::complex<double>(0, 1) * amp[42]);
  jamp[2]  = +2. * (-std::complex<double>(0, 1) * amp[0] - std::complex<double>(0, 1) * amp[1] -
                   std::complex<double>(0, 1) * amp[4] - std::complex<double>(0, 1) * amp[3] -
                   std::complex<double>(0, 1) * amp[19] + std::complex<double>(0, 1) * amp[20] -
                   std::complex<double>(0, 1) * amp[22] - std::complex<double>(0, 1) * amp[23] +
                   std::complex<double>(0, 1) * amp[25] - std::complex<double>(0, 1) * amp[32] +
                   std::complex<double>(0, 1) * amp[30] - std::complex<double>(0, 1) * amp[35] -
                   std::complex<double>(0, 1) * amp[34] - std::complex<double>(0, 1) * amp[40] -
                   std::complex<double>(0, 1) * amp[39]);
  jamp[3]  = +2. * (+std::complex<double>(0, 1) * amp[1] - std::complex<double>(0, 1) * amp[2] +
                   std::complex<double>(0, 1) * amp[4] + std::complex<double>(0, 1) * amp[5] -
                   std::complex<double>(0, 1) * amp[6] - std::complex<double>(0, 1) * amp[8] -
                   std::complex<double>(0, 1) * amp[9] + std::complex<double>(0, 1) * amp[11] -
                   std::complex<double>(0, 1) * amp[25] + std::complex<double>(0, 1) * amp[29] +
                   std::complex<double>(0, 1) * amp[28] + std::complex<double>(0, 1) * amp[32] -
                   std::complex<double>(0, 1) * amp[30] - std::complex<double>(0, 1) * amp[41] +
                   std::complex<double>(0, 1) * amp[39]);
  jamp[4]  = +2. * (-std::complex<double>(0, 1) * amp[0] - std::complex<double>(0, 1) * amp[1] -
                   std::complex<double>(0, 1) * amp[4] - std::complex<double>(0, 1) * amp[3] -
                   std::complex<double>(0, 1) * amp[13] + std::complex<double>(0, 1) * amp[14] -
                   std::complex<double>(0, 1) * amp[16] - std::complex<double>(0, 1) * amp[17] +
                   std::complex<double>(0, 1) * amp[26] - std::complex<double>(0, 1) * amp[32] -
                   std::complex<double>(0, 1) * amp[31] - std::complex<double>(0, 1) * amp[35] +
                   std::complex<double>(0, 1) * amp[33] - std::complex<double>(0, 1) * amp[37] -
                   std::complex<double>(0, 1) * amp[36]);
  jamp[5]  = +2. * (+std::complex<double>(0, 1) * amp[0] + std::complex<double>(0, 1) * amp[2] +
                   std::complex<double>(0, 1) * amp[3] - std::complex<double>(0, 1) * amp[5] -
                   std::complex<double>(0, 1) * amp[7] + std::complex<double>(0, 1) * amp[8] -
                   std::complex<double>(0, 1) * amp[10] - std::complex<double>(0, 1) * amp[11] -
                   std::complex<double>(0, 1) * amp[26] - std::complex<double>(0, 1) * amp[29] -
                   std::complex<double>(0, 1) * amp[28] + std::complex<double>(0, 1) * amp[35] -
                   std::complex<double>(0, 1) * amp[33] - std::complex<double>(0, 1) * amp[38] +
                   std::complex<double>(0, 1) * amp[36]);
  jamp[6]  = +2. * (+std::complex<double>(0, 1) * amp[6] + std::complex<double>(0, 1) * amp[8] +
                   std::complex<double>(0, 1) * amp[9] - std::complex<double>(0, 1) * amp[11] +
                   std::complex<double>(0, 1) * amp[18] + std::complex<double>(0, 1) * amp[19] +
                   std::complex<double>(0, 1) * amp[22] + std::complex<double>(0, 1) * amp[21] -
                   std::complex<double>(0, 1) * amp[24] - std::complex<double>(0, 1) * amp[28] -
                   std::complex<double>(0, 1) * amp[27] + std::complex<double>(0, 1) * amp[41] +
                   std::complex<double>(0, 1) * amp[40] + std::complex<double>(0, 1) * amp[43] +
                   std::complex<double>(0, 1) * amp[42]);
  jamp[7]  = +2. * (+std::complex<double>(0, 1) * amp[7] - std::complex<double>(0, 1) * amp[8] +
                   std::complex<double>(0, 1) * amp[10] + std::complex<double>(0, 1) * amp[11] +
                   std::complex<double>(0, 1) * amp[12] + std::complex<double>(0, 1) * amp[13] +
                   std::complex<double>(0, 1) * amp[16] + std::complex<double>(0, 1) * amp[15] +
                   std::complex<double>(0, 1) * amp[24] + std::complex<double>(0, 1) * amp[28] +
                   std::complex<double>(0, 1) * amp[27] + std::complex<double>(0, 1) * amp[38] +
                   std::complex<double>(0, 1) * amp[37] + std::complex<double>(0, 1) * amp[44] -
                   std::complex<double>(0, 1) * amp[42]);
  jamp[8]  = +2. * (-std::complex<double>(0, 1) * amp[6] - std::complex<double>(0, 1) * amp[7] -
                   std::complex<double>(0, 1) * amp[10] - std::complex<double>(0, 1) * amp[9] -
                   std::complex<double>(0, 1) * amp[19] + std::complex<double>(0, 1) * amp[20] -
                   std::complex<double>(0, 1) * amp[22] - std::complex<double>(0, 1) * amp[23] -
                   std::complex<double>(0, 1) * amp[26] - std::complex<double>(0, 1) * amp[34] -
                   std::complex<double>(0, 1) * amp[33] - std::complex<double>(0, 1) * amp[38] +
                   std::complex<double>(0, 1) * amp[36] - std::complex<double>(0, 1) * amp[41] -
                   std::complex<double>(0, 1) * amp[40]);
  jamp[9]  = +2. * (-std::complex<double>(0, 1) * amp[0] - std::complex<double>(0, 1) * amp[2] -
                   std::complex<double>(0, 1) * amp[3] + std::complex<double>(0, 1) * amp[5] +
                   std::complex<double>(0, 1) * amp[7] - std::complex<double>(0, 1) * amp[8] +
                   std::complex<double>(0, 1) * amp[10] + std::complex<double>(0, 1) * amp[11] +
                   std::complex<double>(0, 1) * amp[26] + std::complex<double>(0, 1) * amp[29] +
                   std::complex<double>(0, 1) * amp[28] - std::complex<double>(0, 1) * amp[35] +
                   std::complex<double>(0, 1) * amp[33] + std::complex<double>(0, 1) * amp[38] -
                   std::complex<double>(0, 1) * amp[36]);
  jamp[10] = +2. * (-std::complex<double>(0, 1) * amp[6] - std::complex<double>(0, 1) * amp[7] -
                    std::complex<double>(0, 1) * amp[10] - std::complex<double>(0, 1) * amp[9] -
                    std::complex<double>(0, 1) * amp[13] + std::complex<double>(0, 1) * amp[14] -
                    std::complex<double>(0, 1) * amp[16] - std::complex<double>(0, 1) * amp[17] -
                    std::complex<double>(0, 1) * amp[25] - std::complex<double>(0, 1) * amp[31] -
                    std::complex<double>(0, 1) * amp[30] - std::complex<double>(0, 1) * amp[38] -
                    std::complex<double>(0, 1) * amp[37] - std::complex<double>(0, 1) * amp[41] +
                    std::complex<double>(0, 1) * amp[39]);
  jamp[11] = +2. * (-std::complex<double>(0, 1) * amp[1] + std::complex<double>(0, 1) * amp[2] -
                    std::complex<double>(0, 1) * amp[4] - std::complex<double>(0, 1) * amp[5] +
                    std::complex<double>(0, 1) * amp[6] + std::complex<double>(0, 1) * amp[8] +
                    std::complex<double>(0, 1) * amp[9] - std::complex<double>(0, 1) * amp[11] +
                    std::complex<double>(0, 1) * amp[25] - std::complex<double>(0, 1) * amp[29] -
                    std::complex<double>(0, 1) * amp[28] - std::complex<double>(0, 1) * amp[32] +
                    std::complex<double>(0, 1) * amp[30] + std::complex<double>(0, 1) * amp[41] -
                    std::complex<double>(0, 1) * amp[39]);
  jamp[12] = +2. * (+std::complex<double>(0, 1) * amp[12] + std::complex<double>(0, 1) * amp[14] +
                    std::complex<double>(0, 1) * amp[15] - std::complex<double>(0, 1) * amp[17] +
                    std::complex<double>(0, 1) * amp[18] + std::complex<double>(0, 1) * amp[19] +
                    std::complex<double>(0, 1) * amp[22] + std::complex<double>(0, 1) * amp[21] -
                    std::complex<double>(0, 1) * amp[25] - std::complex<double>(0, 1) * amp[31] -
                    std::complex<double>(0, 1) * amp[30] + std::complex<double>(0, 1) * amp[40] +
                    std::complex<double>(0, 1) * amp[39] + std::complex<double>(0, 1) * amp[44] +
                    std::complex<double>(0, 1) * amp[43]);
  jamp[13] = +2. * (+std::complex<double>(0, 1) * amp[6] + std::complex<double>(0, 1) * amp[7] +
                    std::complex<double>(0, 1) * amp[10] + std::complex<double>(0, 1) * amp[9] +
                    std::complex<double>(0, 1) * amp[13] - std::complex<double>(0, 1) * amp[14] +
                    std::complex<double>(0, 1) * amp[16] + std::complex<double>(0, 1) * amp[17] +
                    std::complex<double>(0, 1) * amp[25] + std::complex<double>(0, 1) * amp[31] +
                    std::complex<double>(0, 1) * amp[30] + std::complex<double>(0, 1) * amp[38] +
                    std::complex<double>(0, 1) * amp[37] + std::complex<double>(0, 1) * amp[41] -
                    std::complex<double>(0, 1) * amp[39]);
  jamp[14] = +2. * (-std::complex<double>(0, 1) * amp[12] - std::complex<double>(0, 1) * amp[13] -
                    std::complex<double>(0, 1) * amp[16] - std::complex<double>(0, 1) * amp[15] -
                    std::complex<double>(0, 1) * amp[18] - std::complex<double>(0, 1) * amp[20] -
                    std::complex<double>(0, 1) * amp[21] + std::complex<double>(0, 1) * amp[23] +
                    std::complex<double>(0, 1) * amp[26] + std::complex<double>(0, 1) * amp[34] +
                    std::complex<double>(0, 1) * amp[33] - std::complex<double>(0, 1) * amp[37] -
                    std::complex<double>(0, 1) * amp[36] - std::complex<double>(0, 1) * amp[44] -
                    std::complex<double>(0, 1) * amp[43]);
  jamp[15] = +2. * (+std::complex<double>(0, 1) * amp[0] + std::complex<double>(0, 1) * amp[1] +
                    std::complex<double>(0, 1) * amp[4] + std::complex<double>(0, 1) * amp[3] +
                    std::complex<double>(0, 1) * amp[13] - std::complex<double>(0, 1) * amp[14] +
                    std::complex<double>(0, 1) * amp[16] + std::complex<double>(0, 1) * amp[17] -
                    std::complex<double>(0, 1) * amp[26] + std::complex<double>(0, 1) * amp[32] +
                    std::complex<double>(0, 1) * amp[31] + std::complex<double>(0, 1) * amp[35] -
                    std::complex<double>(0, 1) * amp[33] + std::complex<double>(0, 1) * amp[37] +
                    std::complex<double>(0, 1) * amp[36]);
  jamp[16] = +2. * (-std::complex<double>(0, 1) * amp[7] + std::complex<double>(0, 1) * amp[8] -
                    std::complex<double>(0, 1) * amp[10] - std::complex<double>(0, 1) * amp[11] -
                    std::complex<double>(0, 1) * amp[12] - std::complex<double>(0, 1) * amp[13] -
                    std::complex<double>(0, 1) * amp[16] - std::complex<double>(0, 1) * amp[15] -
                    std::complex<double>(0, 1) * amp[24] - std::complex<double>(0, 1) * amp[28] -
                    std::complex<double>(0, 1) * amp[27] - std::complex<double>(0, 1) * amp[38] -
                    std::complex<double>(0, 1) * amp[37] - std::complex<double>(0, 1) * amp[44] +
                    std::complex<double>(0, 1) * amp[42]);
  jamp[17] = +2. * (-std::complex<double>(0, 1) * amp[1] + std::complex<double>(0, 1) * amp[2] -
                    std::complex<double>(0, 1) * amp[4] - std::complex<double>(0, 1) * amp[5] +
                    std::complex<double>(0, 1) * amp[12] + std::complex<double>(0, 1) * amp[14] +
                    std::complex<double>(0, 1) * amp[15] - std::complex<double>(0, 1) * amp[17] +
                    std::complex<double>(0, 1) * amp[24] - std::complex<double>(0, 1) * amp[29] +
                    std::complex<double>(0, 1) * amp[27] - std::complex<double>(0, 1) * amp[32] -
                    std::complex<double>(0, 1) * amp[31] + std::complex<double>(0, 1) * amp[44] -
                    std::complex<double>(0, 1) * amp[42]);
  jamp[18] = +2. * (+std::complex<double>(0, 1) * amp[12] + std::complex<double>(0, 1) * amp[13] +
                    std::complex<double>(0, 1) * amp[16] + std::complex<double>(0, 1) * amp[15] +
                    std::complex<double>(0, 1) * amp[18] + std::complex<double>(0, 1) * amp[20] +
                    std::complex<double>(0, 1) * amp[21] - std::complex<double>(0, 1) * amp[23] -
                    std::complex<double>(0, 1) * amp[26] - std::complex<double>(0, 1) * amp[34] -
                    std::complex<double>(0, 1) * amp[33] + std::complex<double>(0, 1) * amp[37] +
                    std::complex<double>(0, 1) * amp[36] + std::complex<double>(0, 1) * amp[44] +
                    std::complex<double>(0, 1) * amp[43]);
  jamp[19] = +2. * (+std::complex<double>(0, 1) * amp[6] + std::complex<double>(0, 1) * amp[7] +
                    std::complex<double>(0, 1) * amp[10] + std::complex<double>(0, 1) * amp[9] +
                    std::complex<double>(0, 1) * amp[19] - std::complex<double>(0, 1) * amp[20] +
                    std::complex<double>(0, 1) * amp[22] + std::complex<double>(0, 1) * amp[23] +
                    std::complex<double>(0, 1) * amp[26] + std::complex<double>(0, 1) * amp[34] +
                    std::complex<double>(0, 1) * amp[33] + std::complex<double>(0, 1) * amp[38] -
                    std::complex<double>(0, 1) * amp[36] + std::complex<double>(0, 1) * amp[41] +
                    std::complex<double>(0, 1) * amp[40]);
  jamp[20] = +2. * (-std::complex<double>(0, 1) * amp[12] - std::complex<double>(0, 1) * amp[14] -
                    std::complex<double>(0, 1) * amp[15] + std::complex<double>(0, 1) * amp[17] -
                    std::complex<double>(0, 1) * amp[18] - std::complex<double>(0, 1) * amp[19] -
                    std::complex<double>(0, 1) * amp[22] - std::complex<double>(0, 1) * amp[21] +
                    std::complex<double>(0, 1) * amp[25] + std::complex<double>(0, 1) * amp[31] +
                    std::complex<double>(0, 1) * amp[30] - std::complex<double>(0, 1) * amp[40] -
                    std::complex<double>(0, 1) * amp[39] - std::complex<double>(0, 1) * amp[44] -
                    std::complex<double>(0, 1) * amp[43]);
  jamp[21] = +2. * (+std::complex<double>(0, 1) * amp[0] + std::complex<double>(0, 1) * amp[1] +
                    std::complex<double>(0, 1) * amp[4] + std::complex<double>(0, 1) * amp[3] +
                    std::complex<double>(0, 1) * amp[19] - std::complex<double>(0, 1) * amp[20] +
                    std::complex<double>(0, 1) * amp[22] + std::complex<double>(0, 1) * amp[23] -
                    std::complex<double>(0, 1) * amp[25] + std::complex<double>(0, 1) * amp[32] -
                    std::complex<double>(0, 1) * amp[30] + std::complex<double>(0, 1) * amp[35] +
                    std::complex<double>(0, 1) * amp[34] + std::complex<double>(0, 1) * amp[40] +
                    std::complex<double>(0, 1) * amp[39]);
  jamp[22] = +2. * (-std::complex<double>(0, 1) * amp[6] - std::complex<double>(0, 1) * amp[8] -
                    std::complex<double>(0, 1) * amp[9] + std::complex<double>(0, 1) * amp[11] -
                    std::complex<double>(0, 1) * amp[18] - std::complex<double>(0, 1) * amp[19] -
                    std::complex<double>(0, 1) * amp[22] - std::complex<double>(0, 1) * amp[21] +
                    std::complex<double>(0, 1) * amp[24] + std::complex<double>(0, 1) * amp[28] +
                    std::complex<double>(0, 1) * amp[27] - std::complex<double>(0, 1) * amp[41] -
                    std::complex<double>(0, 1) * amp[40] - std::complex<double>(0, 1) * amp[43] -
                    std::complex<double>(0, 1) * amp[42]);
  jamp[23] = +2. * (-std::complex<double>(0, 1) * amp[0] - std::complex<double>(0, 1) * amp[2] -
                    std::complex<double>(0, 1) * amp[3] + std::complex<double>(0, 1) * amp[5] +
                    std::complex<double>(0, 1) * amp[18] + std::complex<double>(0, 1) * amp[20] +
                    std::complex<double>(0, 1) * amp[21] - std::complex<double>(0, 1) * amp[23] -
                    std::complex<double>(0, 1) * amp[24] + std::complex<double>(0, 1) * amp[29] -
                    std::complex<double>(0, 1) * amp[27] - std::complex<double>(0, 1) * amp[35] -
                    std::complex<double>(0, 1) * amp[34] + std::complex<double>(0, 1) * amp[43] +
                    std::complex<double>(0, 1) * amp[42]);

  // Sum and square the color flows to get the matrix element
  double matrix = 0;
  for (i = 0; i < ncolor; i++) {
    ztemp = 0.;
    for (j = 0; j < ncolor; j++) ztemp = ztemp + cf[i][j] * jamp[j];
    matrix = matrix + real(ztemp * conj(jamp[i])) / denom[i];
  }

  // Store the leading color flows for choice of color
  for (i = 0; i < ncolor; i++) jamp2[0][i] += real(jamp[i] * conj(jamp[i]));

  return matrix;
}
