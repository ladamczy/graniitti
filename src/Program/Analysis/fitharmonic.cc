// GRANIITTI - Monte Carlo event generator for high energy diffraction
// https://github.com/mieskolainen/graniitti
//
// <Spherical Harmonic Moment t_LM Based (M,costheta,phi) Decomposition
//  based efficiency inversion and expansion for 2-body final states>
//
// (c) 2017-2020 Mikael Mieskolainen
// Licensed under the MIT License <http://opensource.org/licenses/MIT>.


// C++
#include <math.h>

#include <algorithm>
#include <chrono>
#include <complex>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>

// Own
#include "Graniitti/Analysis/MHarmonic.h"
#include "Graniitti/Analysis/MROOT.h"
#include "Graniitti/MAux.h"
#include "Graniitti/MKinematics.h"
#include "Graniitti/MPDG.h"

// Libraries
#include "cxxopts.hpp"
#include "json.hpp"
#include "rang.hpp"

// HepMC 3
#include "HepMC3/FourVector.h"
#include "HepMC3/GenEvent.h"
#include "HepMC3/GenParticle.h"
#include "HepMC3/GenVertex.h"
#include "HepMC3/Print.h"
#include "HepMC3/ReaderAscii.h"
#include "HepMC3/Relatives.h"
#include "HepMC3/Selector.h"
#include "HepMC3/WriterAscii.h"

using gra::aux::indices;
using namespace gra;

// Create Harmonic moment expansion object (needs to be global for TMinuit
// reasons)
gra::MHarmonic ha;

// Random numbers for fast detector simulation
gra::MRandom rrand;

// Final state fiducial cuts
struct FIDCUTS {
  std::vector<double> ETA = {0.0, 0.0};
  std::vector<double> PT  = {0.0, 0.0};
};

FIDCUTS fidcuts;

// TMinuit fit wrapper calling the global object
void fitwrapper(int &npar, double *gin, double &f, double *par, int iflag) {
  ha.logLfunc(npar, gin, f, par, iflag);
}

void ReadIn(const std::string inputfile, std::vector<gra::spherical::Omega> &events,
            const std::string &FRAME, int MAXEVENTS, bool SIMULATE);

// Fast toy simulation pt-efficiency parameter (put infinite for perfect
// pt-efficiency)
const double pt_scale = 4.5;

// Main function
int main(int argc, char *argv[]) {
  gra::aux::PrintArgv(argc, argv);
  gra::rootstyle::SetROOTStyle();
  gra::aux::PrintFlashScreen(rang::fg::blue);

  std::cout << rang::style::bold << "GRANIITTI - Spherical Harmonics Inverse Expansion"
            << rang::style::reset << std::endl
            << std::endl;
  gra::aux::PrintVersion();

  // Save the number of input arguments
  const int NARGC = argc - 1;
  try {
    cxxopts::Options options(argv[0], "");
    options.add_options()("r,ref",
                          "Reference MC (angular flat MC sample)     <filename> without .hepmc3",
                          cxxopts::value<std::string>())(
        "i,input",
        "Input sample                              <filename1,filename2,...> without .hepmc3",
        cxxopts::value<std::string>())(
        "t,titles", "Phase space titles (3 of them)            <detector,fiducial,flat>",
        cxxopts::value<std::string>())(
        "l,legend", "Legend text                               <title1,title2,...>",
        cxxopts::value<std::string>())("w,yaxis",
                                       "Y-axis text                               <label>",
                                       cxxopts::value<std::string>())(
        "d,mode", "Input mode                                <MC|DATA,...>      ",
        cxxopts::value<std::string>())(
        "c,cuts", "Fiducial cuts                             <ETAMIN,ETAMAX,PTMIN,PTMAX>",
        cxxopts::value<std::string>())(
        "S,scale", "Scale plots (set -1 for unit normalized)  <scale1,scale2,...>",
        cxxopts::value<std::string>())(
        "f,frame", "Lorentz rest frame                        <CM|HX|CS|AH|PG|GJ>",
        cxxopts::value<std::string>())("g,lmax",
                                       "Maximum angular order                     <1|2|3|4|...>",
                                       cxxopts::value<unsigned int>())(
        "o,removeodd", "Remove negative M                         <true|false>",
        cxxopts::value<std::string>())("v,removenegative",
                                       "Remove odd M                              <true|false>",
                                       cxxopts::value<std::string>())(
        "e,eml", "Extended Maximum Likelihood               <true|false>",
        cxxopts::value<std::string>())(
        "a,svdreg", "SVD regularization weight                 <value>", cxxopts::value<double>())(
        "b,l1reg", "L1 regularization weight                  <value>", cxxopts::value<double>())(
        "M,mass", "System mass binning                       <bins,min,max>",
        cxxopts::value<std::string>())("P,momentum",
                                       "System momentum (pt) binning              <bins,min,max>",
                                       cxxopts::value<std::string>())(
        "Y,rapidity", "System rapidity binning                   <bins,min,max>",
        cxxopts::value<std::string>())("z,fastsim",
                                       "Fast simulation of efficiency response    <true|false,...>",
                                       cxxopts::value<std::string>())(
        "X,maximum", "Maximum number of events                  <value>",
        cxxopts::value<unsigned int>())("H,help", "Help");

    auto r = options.parse(argc, argv);

    if (r.count("help") || NARGC == 0) {
      std::cout << options.help({""}) << std::endl;
      std::cout << rang::style::bold << "Example:" << rang::style::reset << std::endl;
      std::cout << "  " << argv[0] << " -r SH_2pi_REF -i SH_2pi ..." << std::endl << std::endl;

      gra::aux::CheckUpdate();
      return EXIT_FAILURE;
    }

    // Fiducial cuts
    if (r.count("cuts")) {
      const std::string   str  = r["cuts"].as<std::string>();
      std::vector<double> cuts = gra::aux::SplitStr(str, double(0), ',');
      if (cuts.size() != 4) {
        throw std::invalid_argument("fitharmonic:: cuts not size 4 <ETAMIN,ETAMAX,PTMIN,PTMAX>");
      }
      fidcuts.ETA = {cuts[0], cuts[1]};
      fidcuts.PT  = {cuts[2], cuts[3]};
    } else {
      throw std::invalid_argument("fitharmonic:: cuts parameter not given");
    }

    MHarmonic::HPARAM hparam;

    // Discretization
    auto tripletfunc = [&](const std::string &str) {
      const std::string         vecstr = r[str].as<std::string>();
      const std::vector<double> vec    = gra::aux::SplitStr(vecstr, double(0), ',');
      if (vec.size() != 3) {
        throw std::invalid_argument("analyze:: " + str +
                                    " discretization not size 3 <bins,min,max>");
      }
      return vec;
    };

    hparam.M  = tripletfunc("M");
    hparam.PT = tripletfunc("P");
    hparam.Y  = tripletfunc("Y");

    // ------------------------------------------------------------------
    // INITIALIZE HARMONIC EXPANSION PARAMETERS

    hparam.LMAX            = r["lmax"].as<unsigned int>();
    hparam.REMOVEODD       = r["removeodd"].as<std::string>() == "true" ? true : false;
    hparam.REMOVENEGATIVEM = r["removenegative"].as<std::string>() == "true" ? true : false;
    hparam.EML             = r["eml"].as<std::string>() == "true" ? true : false;
    hparam.SVDREG          = r["svdreg"].as<double>();
    hparam.L1REG           = r["l1reg"].as<double>();

    // Check valid values
    if (hparam.LMAX < 1) {
      throw std::invalid_argument("fitharmonic: Parameter 'lmax' should be integer >= 1");
    }
    if (hparam.SVDREG < 0) {
      throw std::invalid_argument("fitharmonic: Parameter 'svdreg' should be > 0");
    }
    if (hparam.L1REG < 0) {
      throw std::invalid_argument("fitharmonic: Parameter 'l1reg' should be > 0");
    }

    ha.Init(hparam);

    // ------------------------------------------------------------------
    // INPUT
    const std::string ref = r["ref"].as<std::string>();

    // Lorentz frame
    const std::string FRAME = r["frame"].as<std::string>();

    // Maximum number of events
    int MAXEVENTS = 1E9;
    if (r.count("maximum")) { MAXEVENTS = r["maximum"].as<unsigned int>(); }

    // Read in reference MC for detector expansion (needs to be the same FRAME
    // for all)
    std::vector<gra::spherical::Omega> REFMC;
    ReadIn(ref, REFMC, FRAME, MAXEVENTS, true);  // Always simulate reference here

    // Check dimensions
    auto checkdim = [](const std::vector<std::vector<std::string>> &vec) {
      std::vector<std::size_t> dim(vec.size(), 0);
      for (const auto &i : indices(vec)) { dim[i] = vec[i].size(); }
      if ((std::equal(dim.begin() + 1, dim.end(), dim.begin()))) {
        return true;
      } else {
        return false;
      }
    };

    // Read in real DATA/MC
    const std::vector<std::string> input   = gra::aux::SplitStr2Str(r["input"].as<std::string>());
    const std::vector<std::string> legend  = gra::aux::SplitStr2Str(r["legend"].as<std::string>());
    const std::vector<std::string> mode    = gra::aux::SplitStr2Str(r["mode"].as<std::string>());
    const std::vector<std::string> fastsim = gra::aux::SplitStr2Str(r["fastsim"].as<std::string>());

    if (!checkdim({input, legend, mode, fastsim})) {
      throw std::invalid_argument("fitharmonic:: Input list with different dimensions");
    }

    // Scaling
    std::vector<double> scale(input.size(), 1.0);  // Default 1.0 for all
    if (r.count("scale")) {
      const std::vector<std::string> str_vals =
          gra::aux::SplitStr2Str(r["scale"].as<std::string>());
      if (str_vals.size() == input.size()) {
        for (auto const &i : indices(str_vals)) { scale[i] = std::stod(str_vals[i]); }
      } else {
        throw std::invalid_argument("analyzer::scale input list needs to be of length 0 or N");
      }
    }

    std::vector<gra::spherical::Data> DATA;
    DATA.resize(input.size());

    const std::vector<std::string> titles = gra::aux::SplitStr2Str(r["titles"].as<std::string>());
    if (titles.size() != 3) {
      throw std::invalid_argument(
          "fitharmonic:: 'titles' list should be of size 3 "
          "(detector,fiducial,reference)");
    }

    // yaxis
    std::string yaxis_label = "";
    if (r.count("yaxis")) { yaxis_label = r["yaxis"].as<std::string>(); }

    for (const auto &i : indices(input)) {
      // Read in data
      gra::spherical::Data data;
      data.META.NAME    = input[i];
      data.META.LEGEND  = legend[i];
      data.META.MODE    = mode[i];
      data.META.FRAME   = FRAME;
      data.META.FASTSIM = (fastsim[i] == "true") ? true : false;
      data.META.SCALE   = scale[i];
      data.META.YAXIS   = yaxis_label;

      data.META.TITLES = titles;
      ReadIn(data.META.NAME, data.EVENTS, data.META.FRAME, MAXEVENTS, data.META.FASTSIM);

      // Set data
      DATA[i] = data;
    }

    // LOOP OVER HYPERBINS
    ha.HyperLoop(fitwrapper, REFMC, DATA, hparam);

    // Print out results for external analysis
    // ha.PrintLoop(outputname);

    // Plot all
    std::string inputstr = r["input"].as<std::string>();
    gra::aux::TrimAllSpace(inputstr);
    std::string outputpath = ref + "___";
    for (std::size_t i = 0; i < input.size(); ++i) {
      const std::string marker = (i < input.size() - 1) ? "+" : "";
      outputpath += input[i] + marker;
    }

    ha.PlotAll(outputpath);

  } catch (const std::invalid_argument &e) {
    gra::aux::PrintGameOver();
    std::cerr << rang::fg::red << "Exception catched: " << rang::fg::reset << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const std::ios_base::failure &e) {
    gra::aux::PrintGameOver();
    std::cerr << rang::fg::red << "Exception catched: std::ios_base::failure: " << rang::fg::reset
              << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const cxxopts::OptionException &e) {
    gra::aux::PrintGameOver();
    std::cerr << rang::fg::red << "Exception catched: Commandline options: " << rang::fg::reset
              << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const nlohmann::json::exception &e) {
    gra::aux::PrintGameOver();
    std::cerr << rang::fg::red << "Exception catched: JSON input: " << rang::fg::reset << e.what()
              << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    gra::aux::PrintGameOver();
    std::cerr << rang::fg::red << "Exception catched: Unspecified (...) (Probably input)"
              << rang::fg::reset << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "[fitharmonic:: done]" << std::endl;

  return EXIT_SUCCESS;
}

// Read events in
void ReadIn(const std::string inputfile, std::vector<gra::spherical::Omega> &events,
            const std::string &FRAME, int MAXEVENTS, bool SIMULATE) {
  const std::string total_input = gra::aux::GetBasePath(2) + "/output/" + inputfile + ".hepmc3";
  printf("ReadIn:: Maximum event count %d \n", MAXEVENTS);
  printf("Reading %s \n", total_input.c_str());

  HepMC3::ReaderAscii input_file(total_input);
  // Reading failed
  if (input_file.failed()) {
    throw std::invalid_argument("MHarmonic::ReadIn: Failed to open <" + total_input + ">");
  }
  // Event loop
  int events_read = 0;

  // Dummy vector
  M4Vec            pvec;
  HepMC3::GenEvent evt(HepMC3::Units::GEV, HepMC3::Units::MM);

  // Allocate memory here for speed
  events.resize((int)1e7);
  printf("Memory allocated \n");

  // Event loop
  while (!input_file.failed()) {
    if (events_read >= MAXEVENTS) { break; }

    // Read event from input file
    input_file.read_event(evt);

    if (events_read == 0) {
      HepMC3::Print::listing(evt);
      HepMC3::Print::content(evt);
    }

    // Mesons
    std::vector<M4Vec> pip;
    std::vector<M4Vec> pim;

    // Find out central particles
    std::vector<HepMC3::GenParticlePtr> find_pip =
        HepMC3::applyFilter(HepMC3::StandardSelector::PDG_ID == PDG::PDG_pip, evt.particles());

    std::vector<HepMC3::GenParticlePtr> find_pim =
        HepMC3::applyFilter(HepMC3::StandardSelector::PDG_ID == PDG::PDG_pim, evt.particles());

    for (const HepMC3::GenParticlePtr &p1 : find_pip) {
      pip.push_back(gra::aux::HepMC2M4Vec(p1->momentum()));
    }
    for (const HepMC3::GenParticlePtr &p1 : find_pim) {
      pim.push_back(gra::aux::HepMC2M4Vec(p1->momentum()));
    }

    // CHECK CONDITION
    if (!(pip.size() == 1 && pim.size() == 1)) {
      // HepMC3::Print::listing(evt);
      // HepMC3::Print::content(evt);
      continue;  // skip event, something is wrong
    } else {
      // printf("Found valid \n");
    }

    // ------------------------------------------------------------------
    // Valid events

    // System
    M4Vec sys_ = pip[0] + pim[0];

    // Find out beam protons
    M4Vec p_beam_plus;
    M4Vec p_beam_minus;
    M4Vec p_final_plus;
    M4Vec p_final_minus;

    // Beam protons needed
    if (FRAME == "GJ" || FRAME == "PG" || FRAME == "CS" || FRAME == "AH") {
      for (HepMC3::ConstGenParticlePtr p1 :
           HepMC3::applyFilter(HepMC3::StandardSelector::STATUS == PDG::PDG_BEAM &&
                                   HepMC3::StandardSelector::PDG_ID == PDG::PDG_p,
                               evt.particles())) {
        // Print::line(p1);
        pvec = gra::aux::HepMC2M4Vec(p1->momentum());
        if (pvec.Pz() > 0) {
          p_beam_plus = pvec;
        } else {
          p_beam_minus = pvec;
        }
      }
    }

    // Final state protons
    //if (FRAME == "GJ") {
      for (HepMC3::ConstGenParticlePtr p1 :
           HepMC3::applyFilter(HepMC3::StandardSelector::STATUS == PDG::PDG_STABLE &&
                                   HepMC3::StandardSelector::PDG_ID == PDG::PDG_p,
                               evt.particles())) {
        // Print::line(p1);
        pvec = gra::aux::HepMC2M4Vec(p1->momentum());
        if (pvec.Pz() > 0) {
          p_final_plus = pvec;
        } else {
          p_final_minus = pvec;
        }
      }
    //}

    // ------------------------------------------------------------------
    // Do the frame transformations
    std::vector<M4Vec> rf = {pip[0], pim[0]};

    const int direction = 1;  // PG and GJ

    const M4Vec X = pip[0] + pim[0];

    // Non-rotated rest frame
    if (FRAME == "CM") {
      gra::kinematics::CMframe(rf, X);
      // Helicity frame
    } else if (FRAME == "HX") {
      gra::kinematics::HXframe(rf, X);
      // Pseudo GJ-frame
    } else if (FRAME == "PG") {
      gra::kinematics::PGframe(rf, X, direction, p_beam_plus, p_beam_minus);
      // Collins-Soper frame
    } else if (FRAME == "CS") {
      gra::kinematics::CSframe(rf, X, p_beam_plus, p_beam_minus);
      // Anti-Helicity frame
    } else if (FRAME == "AH") {
      gra::kinematics::AHframe(rf, X, p_beam_plus, p_beam_minus);
      // Gottfried-Jackson frame
    } else if (FRAME == "GJ") {
      gra::kinematics::GJframe(rf, X, direction, p_beam_plus - p_final_plus,
                               p_beam_minus - p_final_minus);
    } else {
      throw std::invalid_argument("MHarmonic::ReadIn: Unknown Lorentz frame: " + FRAME);
    }

    // ------------------------------------------------------------------
    // Construct microevent structure
    gra::spherical::Omega evt;

    // Event data
    evt.costheta = std::cos(rf[0].Theta());
    evt.phi      = rf[0].Phi();
    evt.M        = sys_.M();
    evt.Pt       = sys_.Pt();
    evt.Y        = sys_.Rap();
    evt.fiducial = false;
    evt.selected = false;

    // ------------------------------------------------------------------
    // FIDUCIAL CUTS

    if (std::abs(p_final_minus.Py()) > 0.17 && std::abs(p_final_minus.Py()) < 0.5 &&
        std::abs(p_final_plus.Py()) > 0.17 && std::abs(p_final_plus.Py()) < 0.5 &&

        pip[0].Eta() > fidcuts.ETA[0] && pip[0].Eta() < fidcuts.ETA[1] &&
        pip[0].Pt() > fidcuts.PT[0] && pip[0].Pt() < fidcuts.PT[1] &&

        pim[0].Eta() > fidcuts.ETA[0] && pim[0].Eta() < fidcuts.ETA[1] &&
        pim[0].Pt() > fidcuts.PT[0] && pim[0].Pt() < fidcuts.PT[1]) {
      evt.fiducial = true;
    }

    // ------------------------------------------------------------------
    // EFFICIENCY (FAST) SIMULATION

    // This block can be replaced with FULL GEANT SIMULATION / DELPHES style
    // fast
    // simulation
    // This is a simple parametrization to mimick pt-efficiency effects.

    evt.selected = true;

    // Fast simulate smooth detector pt-efficiency curve here by hyperbolic
    // tangent per
    // particle
    if (SIMULATE) {
      if ((std::tanh(pip[0].Pt() * pt_scale) > rrand.U(0, 1)) &&
          (std::tanh(pim[0].Pt() * pt_scale) > rrand.U(0, 1))) {
        // fine
      } else {
        evt.selected = false;
      }
    }
    // ------------------------------------------------------------------

    events[events_read] = evt;

    ++events_read;
    if (events_read % 500000 == 0) { printf("Event %d \n", events_read); }
  }

  // Remove empty memory
  events.resize(events_read);
  printf("Events read: %d \n\n", events_read);

  // Close HepMC file
  input_file.close();
}
