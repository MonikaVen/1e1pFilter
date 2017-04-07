////////////////////////////////////////////////////////////////////////
// Class:       MyFilter
// Module Type: filter
// File:        MyFilter_module.cc
//
// Generated at Mon Oct  3 13:17:19 2016 by Lorena Escudero Sanchez using artmod
// from cetpkgsupport v1_10_02.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"

#include <memory>

#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "lardataobj/RecoBase/OpFlash.h"
#include "larcore/Geometry/Geometry.h"
#include "lardataobj/RawData/TriggerData.h"

#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Vertex.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/MCBase/MCShower.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "canvas/Persistency/Common/FindOneP.h"
#include "canvas/Utilities/InputTag.h"

#include "larpandora/LArPandoraInterface/LArPandoraHelper.h"

#include "TTree.h"
#include "TFile.h"
#include "TEfficiency.h"

double x_start = 0;
double x_end = 256.35;
double y_start = -116.5;
double y_end = 116.5;
double z_start = 0;
double z_end = 1036.8;
double fidvol = 10;

bool is_fiducial(double x[3], double d) {
  bool is_x = x[0] > (x_start+d) && x[0] < (x_end-d);
  bool is_y = x[1] > (y_start+d) && x[1] < (y_end-d);
  bool is_z = x[2] > (z_start+d) && x[2] < (z_end-d);
  return is_x && is_y && is_z;
}

double distance(double a[3], double b[3]) {
  double d = 0;

  for (int i = 0; i < 3; i++) {
    d += pow((a[i]-b[i]),2);
  }

  return sqrt(d);
}

using namespace lar_pandora;


class MyPi0Filter;

class MyPi0Filter : public art::EDFilter {
public:
  explicit MyPi0Filter(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  MyPi0Filter(MyPi0Filter const &) = delete;
  MyPi0Filter(MyPi0Filter &&) = delete;
  MyPi0Filter & operator = (MyPi0Filter const &) = delete;
  MyPi0Filter & operator = (MyPi0Filter &&) = delete;

  // Required functions.
  bool filter(art::Event & e) override;

  // Selected optional functions.
  void beginJob() override;
  void reconfigure(fhicl::ParameterSet const & p) override;

private:
  TEfficiency * e_energy;

  // Declare member data here.

};


MyPi0Filter::MyPi0Filter(fhicl::ParameterSet const & p)
// :
// Initialize member data here.
{
  art::ServiceHandle<art::TFileService> tfs;
  e_energy = tfs->make<TEfficiency>("e_energy",";#nu_{e} energy [GeV];",30,0,3);

  // Call appropriate produces<>() functions here.
}

bool MyPi0Filter::filter(art::Event & evt)
{
  bool pass = false;

  art::InputTag pandoraNu_tag { "pandoraNu" };
  art::InputTag generator_tag { "generator" };


  auto const& generator_handle = evt.getValidHandle< std::vector< simb::MCTruth > >( generator_tag );
  auto const& generator(*generator_handle);

  std::vector<simb::MCParticle> nu_mcparticles;
  for (int i = 0; i < generator[0].NParticles(); i++) {
    if (generator[0].Origin() == 1) {
      nu_mcparticles.push_back(generator[0].GetParticle(i));
    }
  }

  double proton_energy = 0;
  double electron_energy = 0;
  int protons = 0;
  bool is_pion = false;
  bool is_electron = false;
  double proton_energy_threshold = 0.06;
  double electron_energy_threshold = 0.035;

  for (auto& mcparticle: nu_mcparticles) {
    if (mcparticle.Process() == "primary" and mcparticle.T() != 0 and mcparticle.StatusCode() == 1) {

      double position[3] = {mcparticle.Position().X(),mcparticle.Position().Y(),mcparticle.Position().Z()};
      double end_position[3] = {mcparticle.EndPosition().X(),mcparticle.EndPosition().Y(),mcparticle.EndPosition().Z()};

      if (mcparticle.PdgCode() == 2212 && (mcparticle.E()-mcparticle.Mass()) > proton_energy_threshold && is_fiducial(position,fidvol) && is_fiducial(end_position,fidvol)) {
        protons++;
        proton_energy = std::max(mcparticle.E()-mcparticle.Mass(), proton_energy);
      }

      if (mcparticle.PdgCode() == 11 && (mcparticle.E()-mcparticle.Mass()) > electron_energy_threshold && is_fiducial(position,fidvol)) {
        is_electron = true;
        electron_energy = std::max(mcparticle.E()-mcparticle.Mass(), electron_energy);
      }

      if (mcparticle.PdgCode() == 211 || mcparticle.PdgCode() == 111) {
        is_pion = true;
      }

    }
  }

  //double nu_energy = generator[0].GetNeutrino().Nu().E();
  //double true_neutrino_vertex[3] = {generator[0].GetNeutrino().Nu().Vx(),generator[0].GetNeutrino().Nu().Vy(),generator[0].GetNeutrino().Nu().Vz()};
  //double closest_distance = std::numeric_limits<double>::max();

  if (!(is_electron && !is_pion && protons >= 1)) {
    std::cout << "NO CCQE EVENT" << std::endl;
    //return false;
  }

  try {
    auto const& pfparticle_handle = evt.getValidHandle< std::vector< recob::PFParticle > >( pandoraNu_tag );
    auto const& pfparticles(*pfparticle_handle);

    art::FindOneP< recob::Vertex > vertex_per_pfpart(pfparticle_handle, evt, pandoraNu_tag);


    for (size_t ipf = 0; ipf < pfparticles.size(); ipf++) {

      bool is_neutrino = abs(pfparticles[ipf].PdgCode()) == 12 && pfparticles[ipf].IsPrimary();

      // Is a nu_e or nu_mu PFParticle?
      if (!is_neutrino) continue;

      int showers = 0;
      int tracks = 0;

      double neutrino_vertex[3];

      auto const& neutrino_vertex_obj = vertex_per_pfpart.at(ipf);
      neutrino_vertex_obj->XYZ(neutrino_vertex); // PFParticle neutrino vertex coordinates

      // Is the vertex within fiducial volume?
      if (!is_fiducial(neutrino_vertex, fidvol)) continue;

      //cout << pfparticles[ipf].PdgCode() << " " << distance(neutrino_vertex,correct_neutrino_vertex) << endl;

      // Loop over the neutrino daughters and check if there is a shower and a track
      for (auto const& pfdaughter: pfparticles[ipf].Daughters()) {

        // auto const& daughter_vertex_obj = vertex_per_pfpart.at(pfdaughter);
        // double daughter_vertex[3];
        // daughter_vertex_obj->XYZ(daughter_vertex);
        // double distance_daugther = distance(neutrino_vertex,daughter_vertex);

        if (pfparticles[pfdaughter].PdgCode() == 11) {
          showers++;
        }

        if (pfparticles[pfdaughter].PdgCode() == 13) {
          tracks++;
        }

      } // end for pfparticle daughters

      if (showers >= 1 && tracks >= 1) {
        //closest_distance = std::min(distance(neutrino_vertex,true_neutrino_vertex),closest_distance);
        pass = true;
      }

    } // end for pfparticles

    //e_energy->Fill(pass, nu_energy);

  } catch (...) {
    std::cout << "NO RECO DATA PRODUCTS" << std::endl;
  }

  if (pass) {
    std::cout << "EVENT SELECTED" << std::endl;
  }
  //std::cout << closest_distance << std::endl;
  return pass;
}

void MyPi0Filter::beginJob()
{
  // Implementation of optional member function here.
}

void MyPi0Filter::reconfigure(fhicl::ParameterSet const & p)
{
  // Implementation of optional member function here.
}

DEFINE_ART_MODULE(MyPi0Filter)
