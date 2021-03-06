// Tim Lou
// 10/04/2015

// C++ tools
#include <cstring>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>

// ROOT
#include "TROOT.h"
#include "TRandom.h"
#include "TApplication.h"
#include "TObjArray.h"
#include "TDatabasePDG.h"
#include "TParticlePDG.h"
#include "TLorentzVector.h"

// Delphes library
#include "modules/Delphes.h"
#include "classes/DelphesClasses.h"
#include "classes/DelphesFactory.h"

// FastJet?
// Duplicates in Delphes
#include "fastjet/ClusterSequence.hh"
#include "fastjet/Selector.hh"
#include "fastjet/contribs/Nsubjettiness/Nsubjettiness.hh"

// Pythia
#include "Pythia8/Pythia.h"
#include "Pythia8Plugins/CombineMatchingInput.h" //  Matching not implemented yet
#include "Pythia8Plugins/HepMC2.h"


// Pythia8 library to perform t-channel production
#include "tchannel_hidden.hh"

// Pythia8 library to perform higher dimensional ops
// dim-7 GG-chichi production
//#include "gluonportal.hh"

#include "CmdLine/CmdLine.hh"

// To simplify code moving all unnecessary functions to this file
#include "pythia_functions.h"

//using namespace Pythia8;
using namespace fastjet;
using namespace fastjet::contrib;
using namespace std;  

int main(int argc, char** argv) {

  cout<<"Usage: -m (mode) -n (nevent = 100) -o (output) -pt_min (100) -mphi (10000) -metmin (0) -phimass (default=20) -lambda (dark confinement scale) -frag (fragmentation) -inv (invisible ratio) -v (verbose) -seed (0) -rehad (off) -njet (2)"<<endl;

  //parse input strings
  CmdLine cmdline(argc, argv);
  
  string mode = cmdline.value<string>("-m", "tchannel"); // Run mode
  double pt_min = cmdline.value<double>("-ptmin", 0); // Min pT of leading jets
  double met_min = cmdline.value<double>("-metmin", 0); // Min MET
  double met_max = cmdline.value<double>("-metmax", 99999); // Max MET
  double dphi_min = cmdline.value<double>("-dphimin", 0); // Max dphi between jet and the selected jets

  string output = cmdline.value<string>("-o", "output"); // Name of output file

  bool lepton_veto = cmdline.value<bool>("-lveto", true); // Do lepton veto
  bool rehad = cmdline.present("-rehad"); // Rehadronize

  // for Zprime mode
  bool Zprime = cmdline.present("-Zprime");
  if(Zprime)
    cout<<"INFO: Zprime mode enabled, jets will be reclustered."<<endl;

  // Instantiate event-wide, object and info files
  // file_evt stores event wide variables
  // also last two line stores cxn, efficiency etc
  ofstream file_evt;
  ofstream file_meta;

  string input;

  // If using an external lhe file
  if (mode == "lhe")
    input = cmdline.value<string>("-i");  
    cout << "using LHE mode" << endl;
  try
  {
    file_evt.open((output + ".evt").c_str());
    file_meta.open((output + ".meta").c_str());
  }
  catch(...)
  {
    cerr<<"ERROR: cannot open "<<output<<", exiting..."<<endl;
    return 1;
  }

  int nEvent = cmdline.value<int>("-n", 1000);
  //int nAbort = round(0.2*nEvent);//5;
  int nAbort = 10; // Abort after how many errors

  // Pythia generator
  Pythia pythia;

  // Interface for conversion from Pythia8::Event to HepMC one.
  HepMC::Pythia8ToHepMC ToHepMC;
  HepMC::IO_GenEvent *ascii_io;

  // Initialization for LHC
  pythia.readString("Beams:eCM = " + 
     to_st(cmdline.value<int>("-ECM", 13000)));

  // Custom Higgs pT cut
  /*
  ParticlePTCut* HiggsPTCut = new ParticlePTCut(pt_min,25);
  pythia.setUserHooksPtr(HiggsPTCut);  
  */

  bool m_lhe = false;


  // weighted events
  bool weighted = cmdline.present("-w");
  
  // Check for verbose mode
  if(!cmdline.present("-v"))
    pythia.readString("Print:quiet = on");


  string hepmc_file;
  bool hepmc=false;
  if(cmdline.present("-hepmc")){
    cout<<"HepMC output specified"<<endl;
    hepmc_file=cmdline.value<string>("-hepmc", "out.hepmc");
    hepmc=true;
    ascii_io=new HepMC::IO_GenEvent(hepmc_file.c_str(), std::ios::out);
  }
  
  


  // Hidden scalar production
  if (mode == "tchannel"){ 

    double mphi=
      cmdline.value<double>("-mphi", 1000.0); // Bifundamental mass

      cout << "INFO: bufundamental mass is " + to_st(mphi) << endl;

      double pt_cut = cmdline.value<double>("-ptcut", 600.0); // phase-space cut on pthatmin to speed up MC generation

    init_tchannel(pythia, mphi, pt_cut);

    init_hidden(pythia,
    cmdline.value<double>("-phimass", 20.0),
    cmdline.value<double>("-lambda", 10),
    cmdline.value<double>("-inv", 0.3),
    cmdline.value<bool>("-run", true),
    cmdline.value<int>("-Nc", 2),   
    cmdline.value<int>("-NFf", 2),    
    cmdline.value<int>("-NBf", 0)
    );  
  }  
  
  else if(mode == "lhe"){
    //read lhe file
    m_lhe=true;
    
    CombineMatchingInput combined;
    UserHooks* matching = combined.getHook(pythia);
    if (!matching) {
      cout<<"ERROR: cannot obtain matching pointer"<<endl;
      return 1;
    }
    
    pythia.setUserHooksPtr(matching);

    init_hidden(pythia,
    cmdline.value<double>("-phimass", 20.0),
    cmdline.value<double>("-lambda", 10),
    cmdline.value<double>("-inv", 0.3),
    cmdline.value<bool>("-run", true),
    cmdline.value<int>("-Nc", 2),   
    cmdline.value<int>("-NFf", 2),    
    cmdline.value<int>("-NBf", 0)
    );  

    cout<<"reading file: "<<input<<endl;

    pythia.readString("Init:showChangedParticleData = off");
    pythia.readString("Beams:LHEF = "+ input);
    pythia.readString("Beams:frameType = 4");

    pythia.readString("JetMatching:merge = on");
    pythia.readString("JetMatching:setMad = on");
    pythia.readString("JetMatching:scheme = 1");
    
    pythia.readString("JetMatching:jetAlgorithm = 2");
    pythia.readString("JetMatching:exclusive = 2");
    pythia.readString("JetMatching:nJetMax = " + to_st(cmdline.value<int>("-nmatch", 1)));
    
    cout<<"input: "<<"Beams:LHEF = "+input<<endl;
  } 
  else
  {
    cerr<<"ERROR: mode: " << mode << " not supported, exiting..." << endl;
    return 1;
  }
  
  // Set seed
  pythia.readString("Random:setSeed = on");
  // Fix random seed
  pythia.readString("Random:seed = " + 
        to_st(cmdline.value<int>("-seed", 0)));
  // Rehadronization turned on
  if(rehad)
    pythia.readString("HadronLevel:all = off");
  
  // Initialize Pythia

  pythia.init();
  
  cout<<"INFO: pT cut: "<< pt_min <<endl;
  cout<<"INFO: MEt cut: "<< met_min <<endl;

  // Begin event loop.
  int iAbort = 0;
  
  // Event level variable
  file_evt << "evt,MEt,mjj,Mt,pt1,eta1,y1,pt2,eta2,y2,pt3,eta3,y3,pt4,eta4,y4,dphi,"
     << "nj,n_meson,n_glu" ;

  if(weighted)
    file_evt << ", weight";

  file_evt << endl;

  // Access to pythia event
  Pythia8::Event& event = pythia.event;
  Pythia8::Event saved_event;

  // create HepMC files
  HepMC::GenEvent* hepmcevt;

  int iEvent = 0;
  int iTotal = 0;

  // Start timer
  Timer mytime(nEvent);

  // Declare Delphes variables

  ExRootConfReader *config = new ExRootConfReader();
  config->ReadFile("delphes_card_CMS.tcl");
  // config->ReadFile("delphes_card_ATLAS.tcl");

  Delphes *delphes = new Delphes("Delphes"); 
  delphes -> SetConfReader(config);
  DelphesFactory *factory = delphes->GetFactory();

  TObjArray* stable = 
    delphes->ExportArray("stableParticles");

  // Start root application mode

  gROOT->SetBatch();
  int appargc = 1;
  char appName[] = "Delphes";
  char *appargv[] = {appName};
  TApplication app(appName, &appargc, appargv);

  // Initialize delphes

  delphes->InitTask();

  // Events to print
  int evt_print = 20;

  // Running simulation
  bool end = false;
  
  int pythia_status=-1;

  while ((!m_lhe && (iEvent < nEvent)) || 
   (m_lhe && (iTotal < nEvent) && !end)) 
  {
    // Clear delphes
    delphes->Clear();
    bool sim_failed=false;
    // If rehadronization is turned on
    if(rehad) {
      
      // Renew an event
      if (iTotal % 5 == 0) {
        while (!(pythia_status=pythia.next())) {

          if(pythia.info.atEndOfFile()){
            cout <<"Pythia reached end of file"<<endl;
            end=true;
            sim_failed=true;
            break;     
          }

          if (++iAbort < nAbort) continue;
      
          cerr << "ERROR: Event generation aborted prematurely, owing to error!" << endl;
          sim_failed=true;
          break; 
          
        }
  
  saved_event = pythia.event;
      }

      else pythia.event = saved_event;
  
      // Run hadronization
      pythia.forceHadronLevel();
    }
    else
    {
      // Tell pythia to run pythia.next()
      while (!(pythia_status=pythia.next())) {

  cout<<"Pythia failed, status "<<pythia_status<<endl;

    if(pythia.info.atEndOfFile()){
      cout <<"Pythia reached end of file"<<endl;
      end=true;
      sim_failed=true;
      break; 
    }
    
    if (++iAbort < nAbort) continue;
    cerr<<"ERROR: Event generation aborted due to error!" 
         <<endl;
    sim_failed=true;
    return 1; 
      }
    }

    if(sim_failed==true) continue;
    // Increment tried events
    ++iTotal;

    
    //fill hepmc pointers, and write files
    if(hepmc){
      hepmcevt = new HepMC::GenEvent();
      ToHepMC.fill_next_event( pythia, hepmcevt );
      (*ascii_io) << hepmcevt;
      delete hepmcevt;
    }

    


    // Now process through Delphes
    Pythia_to_Delphes(factory, stable, event);
    
    // Run delphes code
    delphes->ProcessTask();

    Candidate *can;

    const TObjArray* vMEt = delphes->ImportArray
      ("MissingET/momentum");
    
    can = (Candidate*) TIter(vMEt).Next() ;

    // Missing ET pointer must exist
    if(can == NULL){
      cout<<"ERROR: MET pointer not found!"<<endl;
      continue;
    }
    // If met is too small or large, continue
    if ((can->Momentum.Pt() < met_min) || (can->Momentum.Pt() > met_max)){
      continue;
    }
    PseudoJet MEt(-can->Momentum.Px(),-can->Momentum.Py(), 0,can->Momentum.Pt());    
    
    // Now grab the jets
    const TObjArray* jets = delphes->ImportArray
      ("UniqueObjectFinder/jets");

    // Now grab muons
    const TObjArray* muons = delphes->ImportArray
      ("UniqueObjectFinder/muons");      

    // Now grab the electrons
    const TObjArray* electrons = delphes->ImportArray
      ("UniqueObjectFinder/electrons");      

    // Demand at least two jets above pt cut
    int njet = cmdline.value<int>("-njet", 1);

    int njet_max = cmdline.value<int>("-njetmax", 100);

    if(njet < 0){
      cout<<"ERROR: cannot require negative jets"<<endl;
      njet=0;
    }

    //grab objects
    vector<PseudoJet> selected_jets;
    vector<PseudoJet> selected_leptons;
    

    // Loop over muons and get information
    for(int i=0; i<muons->GetEntriesFast(); i++){
      
      Candidate* cmuon = (Candidate*) muons->At(i);
      if(fabs(cmuon->Momentum.Eta())>2.5)
        continue;
      
      if(fabs(cmuon->Momentum.Pt())<10)
        continue;

      PseudoJet cmuon_v(cmuon->Momentum.Px(), 
      cmuon->Momentum.Py(),
      cmuon->Momentum.Pz(),
      cmuon->Momentum.E());

      selected_leptons.push_back(cmuon_v);
    }    

    // Loop over electrons
    for(int i=0; i<electrons->GetEntriesFast(); i++){
      
      Candidate* celectron = (Candidate*) electrons->At(i);
      if(fabs(celectron->Momentum.Eta())>2.5)
        continue;
      
      if(fabs(celectron->Momentum.Pt())<20)
        continue;

      PseudoJet celectron_v(celectron->Momentum.Px(), 
           celectron->Momentum.Py(),
           celectron->Momentum.Pz(),
           celectron->Momentum.E());

      selected_leptons.push_back(celectron_v);
    }    

    // Do lepton veto
    if ((selected_leptons.size()>0) && (lepton_veto))
      continue;

    // Loop over jets and get information
    for(int i=0; i<jets->GetEntriesFast(); i++){
      
      Candidate* cjet = (Candidate*) jets->At(i);

      if(cjet->Momentum.Pt()<30.0)
  continue;
  
      if(fabs(cjet->Momentum.Eta())>2.8)
  continue;

      PseudoJet cjet_v(cjet->Momentum.Px(), 
           cjet->Momentum.Py(),
           cjet->Momentum.Pz(),
           cjet->Momentum.E());

      //store the jets
      selected_jets.push_back(cjet_v);
    }



    double mjj=0;
    if(selected_jets.size()>=2){
      mjj = (selected_jets[0]+selected_jets[1]).m();
    }

    //if Zprime mode, recluster into R=1.0 jets
    if(Zprime){
      JetDefinition jet_def(cambridge_algorithm, 1.1);
      ClusterSequence cs(selected_jets, jet_def);
      selected_jets = sorted_by_pt(cs.inclusive_jets());
    }
    


    //demand njets > pt_min
    if(selected_jets.size() < njet || selected_jets[0].pt() < pt_min || 
       selected_jets.size() > njet_max) 
      continue;

    if (get_dphijj(MEt, selected_jets) < dphi_min)
      continue;

    //print the jets

    /*
    
      for(int i=0; i<selected_jets.size(); i++){
      file_obj << iEvent << ",j,"
         << i+1 << ","
         << selected_jets[i] 
         << jets_ntrk[i] <<endl;  
    }    

    //print the leptons
    for(int i=0; i<selected_leptons.size(); i++){
      file_obj << iEvent << ",l,"
         << i+1 << ","
         << selected_leptons[i] <<"1"
         << endl;  
    }
    //print met
    file_obj << iEvent <<",met,"
       << 1 << "," 
       << MEt<<"0"<<endl;

    */

    /*
    int n_diag=0;
    int n_dark=0;
    vector<PseudoJet> dark_mesons;
    PseudoJet inv_jet;

    // gather MC information
    for (int i=0; i<event.size(); ++i){
      Particle& p = event[i];

      if(p.id()==4900111)
        n_diag++;

      // Only get invisible pions
      if(p.id()!=4900211) continue;
      
      // We expect the next particle to be an anti-pion
      // if not, there is an error!
      if(i+1 >= event.size() || event[i+1].id() != -4900211)
  {
    cout 
      << "ERROR: invisible pions not coming in pairs!" 
      << endl;
    continue;
  }

      n_dark++;
      
      Particle& p_next = event[i+1];

      PseudoJet
  meson( p.px() + p_next.px(),
         p.py() + p_next.py(),
         p.pz() + p_next.pz(),
         p.e() + p_next.e() );
      
      dark_mesons.push_back( meson );
      inv_jet += meson;
    }
    */

    double dphijj = get_dphijj(MEt, selected_jets);

    double pt_list[4] = {0};
    double eta_list[4] = {0};
    double y_list[4] = {0};

    for(int i=0; i<4; i++){
      if(selected_jets.size() > i){
  pt_list[i]=selected_jets[i].pt();
  eta_list[i]=selected_jets[i].eta(); 
  y_list[i]=selected_jets[i].rap();
      }
      else{
  pt_list[i]=-1;
  eta_list[i]=999;
      }
    }

    
    double Mt_ = 0;
    if(selected_jets.size()>= 2)
      Mt_ = (MEt+selected_jets[0]+selected_jets[1]).mperp();

    file_evt<<iEvent<<","
      << MEt.pt()<<","    
      << mjj <<","
      << Mt_ << ","
      << pt_list[0]<<","
      << eta_list[0]<<","      
      << y_list[0]<<","      
      << pt_list[1]<<","
      << eta_list[1]<<","      
      << y_list[1]<<","      
      << pt_list[2]<<","
      << eta_list[2]<<","      
      << y_list[2]<<","      
      << pt_list[3]<<","
      << eta_list[3]<<","      
      << y_list[3]<<","      
      << dphijj<<","
      << selected_jets.size()<<","
      << get_nmeson(event) << ","
      << get_glu(event);

    if(weighted){
      file_evt << ", " << pythia.info.weight();
    }


    file_evt<<endl;

    ++iEvent;
    
    if(!m_lhe && (iEvent %10 ==0))
    {
      mytime.update(iEvent);
      cout<<mytime;
    }

    else if(m_lhe && (iTotal %10 ==0))
    {
      mytime.update(iTotal);
      cout<<mytime;
    }
  }

  if(!m_lhe)
    mytime.update(iEvent);

  else
    mytime.update(iTotal);

  cout<<mytime<<endl;
  cout<<iEvent<<" total events"<<endl;

  file_meta<<"nevt, npass, eff, total, pass, "
     <<"ptcut, metcut, cxn, cxn_err";

  if(weighted)
    file_meta << ", sum_weight";

  file_meta<<endl;

  file_meta<<iTotal<<","<<iEvent<<","
     <<iEvent/double(iTotal)<<","
     <<iTotal<<","
     <<iEvent<<","
     <<pt_min<<","
     <<met_min<<","
     <<pythia.info.sigmaGen()*1e9<<","
     <<pythia.info.sigmaErr()*1e9;

  if(weighted)
    file_meta << ", "<< pythia.info.weightSum();
  file_meta << endl;
  

  //clean up
  delphes->FinishTask();
  delete delphes;
  delete config;
  
  if(cmdline.present("-v"))
    pythia.stat();
  // Done.
  file_evt.close();
  //file_obj.close();

  delete ascii_io;
  return 0;

}