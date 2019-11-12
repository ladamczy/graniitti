// (Sub)-Processes and Amplitudes
//
// (c) 2017-2019 Mikael Mieskolainen
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.

#ifndef MSUBPROC_H
#define MSUBPROC_H

// C++
#include <complex>
#include <random>
#include <vector>

// Own
#include "Graniitti/M4Vec.h"
#include "Graniitti/MAux.h"
#include "Graniitti/MDurham.h"
#include "Graniitti/MForm.h"
#include "Graniitti/MGamma.h"
#include "Graniitti/MPDG.h"
#include "Graniitti/MRegge.h"
#include "Graniitti/MTensorPomeron.h"

namespace gra {
// "Functionoid class"

class MSubProc : public MDurham, public MRegge, public MGamma {
 public:
  MSubProc(const std::string &_ISTATE, const std::string &_CHANNEL, const MPDG &_PDG);
  MSubProc(const std::vector<std::string> &first);
  void ConstructDescriptions(const std::string &first);
  void SetTechnicalBoundaries(gra::GENCUT &gcuts, unsigned int EXCITATION);

  MSubProc() {}
  ~MSubProc() {}

  double GetBareAmplitude2(gra::LORENTZSCALAR &lts);

  std::string  ISTATE;   // "PP","yy","gg" etc.
  std::string  CHANNEL;  // "CON", "RES" etc.
  unsigned int LIPSDIM = 0;
  bool         UW      = false;

  // -------------------------------------------------------------------

  // Available channels and their descriptions
  std::map<std::string, std::map<std::string, std::string>> descriptions;

  double GetBareAmplitude2_X(gra::LORENTZSCALAR &lts);
  double GetBareAmplitude2_PP(gra::LORENTZSCALAR &lts);
  double GetBareAmplitude2_yP(gra::LORENTZSCALAR &lts);
  double GetBareAmplitude2_OP(gra::LORENTZSCALAR &lts);

  double GetBareAmplitude2_yy(gra::LORENTZSCALAR &lts);
  double GetBareAmplitude2_gg(gra::LORENTZSCALAR &lts);
  double GetBareAmplitude2_yy_DZ(gra::LORENTZSCALAR &lts);
  double GetBareAmplitude2_yy_LUX(gra::LORENTZSCALAR &lts);

  // Particle database
  MPDG PDG;

 private:
  void ReggeReset(gra::LORENTZSCALAR &lts);
};

}  // namespace gra

#endif
