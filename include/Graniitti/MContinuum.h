// Continuum 2->N phase space class
//
// (c) 2017-2021 Mikael Mieskolainen
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#ifndef MCONTINUUM_H
#define MCONTINUUM_H

// C++
#include <complex>
#include <vector>

// Own
#include "Graniitti/M4Vec.h"
#include "Graniitti/MAux.h"
#include "Graniitti/MMatrix.h"
#include "Graniitti/MProcess.h"
#include "Graniitti/MSpin.h"

// HepMC33
#include "HepMC3/GenEvent.h"

namespace gra {
class MContinuum : public MProcess {
 public:
  MContinuum();
  MContinuum(std::string process, const std::vector<aux::OneCMD> &syntax);
  virtual ~MContinuum();

  void post_Constructor();

  double operator()(const std::vector<double> &randvec, AuxIntData &aux) {
    return EventWeight(randvec, aux);
  }
  double EventWeight(const std::vector<double> &randvec, AuxIntData &aux);
  bool   EventRecord(HepMC3::GenEvent &evt);
  void   PrintInit(bool silent) const;

 private:
  void Initialize();
  bool LoopKinematics(const std::vector<double> &p1p, const std::vector<double> &p2p);
  bool FiducialCuts() const;

  // 3*N-4 dimensional phase space, 2->N
  bool BNRandomKin(unsigned int Nf, const std::vector<double> &randvec);
  bool BNBuildKin(unsigned int Nf, double pt1, double pt2, double phi1, double phi2,
                  const std::vector<double> &kt, const std::vector<double> &phi,
                  const std::vector<double> &y, double m1, double m2);

  void BLinearSystem(std::vector<M4Vec> &p, const std::vector<M4Vec> &q, const M4Vec &p1f,
                     const M4Vec &p2f) const;

  double BNIntegralVolume() const;
  double BNPhaseSpaceWeight() const;

  // Auxialary (kt) vectors
  std::vector<M4Vec> pkt_;
};

}  // namespace gra

#endif