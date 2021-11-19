#include "xaod_to_parquet/AnalysisLooper.h"
#include "xaod_to_parquet/helpers.h"

// std/stl
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <fstream>

// xAOD
#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/TStore.h"
#include "xAODEventInfo/EventInfo.h"
#include "xAODCutFlow/CutBookkeeper.h"
#include "xAODCutFlow/CutBookkeeperContainer.h"
#include "PathResolver/PathResolver.h"
#include "xAODMetaData/FileMetaData.h"
#include "xAODBase/IParticleHelpers.h"

// ROOT
#include "TChainElement.h"
#include "TFile.h"

namespace x2p {

AnalysisLooper::AnalysisLooper() :
    _chain(nullptr),
    _n_events_processed(0),
    _sumw(0),
    _sumw2(0)
{
    _tevent = std::make_unique<xAOD::TEvent>(xAOD::TEvent::kClassAccess);
    _tstore = std::make_unique<xAOD::TStore>();
}

void AnalysisLooper::Init(TTree* tree) {
    std::cout << __x2pfunc__ << std::endl;
    get_sumw();

    _tevent->readFrom(tree);
    _tevent->getEntry(0);

    const xAOD::EventInfo* ei{nullptr};
    if(!_tevent->retrieve(ei, "EventInfo").isSuccess()) {
        throw std::runtime_error("Failed to retrieve EventInfo");
    }

    if(!ei->eventType(xAOD::EventInfo::IS_SIMULATION)) {
        throw std::runtime_error("Cannot run on non-simulation inputs");
    }
    std::string simflavor{""};
    std::string amitag{""};
    if(!get_metadata_info(tree, simflavor, amitag)) {
        throw std::runtime_error("Failed to obtain MetaData tree");
    }

    _is_af2 = simflavor.find("ATLFASTII") != std::string::npos;
    if(amitag.find("r9364") != std::string::npos) _mctype = "mc16a";
    else if(amitag.find("r9781") != std::string::npos) _mctype = "mc16c";
    else if(amitag.find("r10201") != std::string::npos) _mctype = "mc16d";
    else if(amitag.find("r10724") != std::string::npos) _mctype = "mc16e";
    else {
        throw std::runtime_error("Failed to determine MC campaign");
    }
    if(amitag.find("r9364") != std::string::npos) _mctype = "mc16a";
    std::cout << __x2pfunc__ << "Treating sample as " << (_is_af2 ? "AFII" : "FullSim") << std::endl;
    std::cout << __x2pfunc__ << "Treating MC sample as MC campaign " << _mctype << std::endl;


    // SUSYTools
    if(!initialize_susytools()) {
        throw std::runtime_error("Failed to initialize SUSYTools");
    }

}

void AnalysisLooper::get_sumw() {

    // get the sumw of all the input files to be processed
    TObjArray* chain_files = _chain->GetListOfFiles();
    TIter next(chain_files);
    TChainElement* ch_file{0};
    int file_counter{0};
    while ((ch_file = (TChainElement*)next())) {
        std::cout << __x2pfunc__ << "CutBookkeepr info for: " << ch_file->GetTitle() << std::endl;
        TFile* f = TFile::Open(ch_file->GetTitle());
        _tevent->readFrom(f);
        _tevent->getEntry(0);
        if(!process_cutbookkeeper(*_tevent, file_counter)) {
            throw std::runtime_error("Failed to process CutBookkeeper info for file: " + std::string(ch_file->GetTitle() ));
        }
        f->Close();
        f->Delete();
        file_counter++;
    }
    std::cout << __x2pfunc__ << "----------------------------------------------------" << std::endl;
    std::cout << __x2pfunc__ << "CutBookkeeper summary:" << std::endl;
    std::cout << __x2pfunc__ << "  # events processed    : " << _n_events_processed << std::endl;
    std::cout << __x2pfunc__ << "  sum of weights        : " << _sumw << std::endl;
    std::cout << __x2pfunc__ << "  (sum of weights)^2    : " << _sumw2 << std::endl;
    std::cout << __x2pfunc__ << "----------------------------------------------------" << std::endl;

}

bool AnalysisLooper::process_cutbookkeeper(xAOD::TEvent& event, int file_counter) {
    bool ok{true};
    const xAOD::CutBookkeeperContainer* completeCBC = nullptr;
    ok = event.retrieveMetaInput(completeCBC, "CutBookkeepers");
    if(!ok) {
        throw std::runtime_error("Failed to get CutBookkeeper metadata for file #" + file_counter);
    }

    const xAOD::CutBookkeeper* allEventsCBK = nullptr;
    int maxcycle{-1};
    for(auto& cbk : *completeCBC) {
        if(cbk->name() == "AllExecutedEvents" && cbk->inputStream() == "StreamAOD" && cbk->cycle() > maxcycle) {
            maxcycle = cbk->cycle();
            allEventsCBK = cbk;
        }
    } // cbk

    uint64_t nevents{0};
    double sumw{0};
    double sumw2{0};
    if(allEventsCBK) {
        nevents = allEventsCBK->nAcceptedEvents();
        sumw = allEventsCBK->sumOfEventWeights();
        sumw2 = allEventsCBK->sumOfEventWeightsSquared();
    } else {
        std::cout << __x2pfunc__ << "\"AllExecutedEvents\" branch not found in CutBookkeeper metadata for file #" + file_counter << std::endl;
        ok = false;
    }

    _n_events_processed += nevents;
    _sumw += sumw;
    _sumw2 += sumw2;

    std::cout << __x2pfunc__ << " > file #" << std::setw(10) << file_counter << " # accepted events: " << _n_events_processed << ", sumw: " << _sumw << ", sumw2: " << _sumw2 << std::endl;
    return ok;

}

bool AnalysisLooper::get_metadata_info(TTree* tree, std::string& simflavor, std::string& amitag) {

    TTree* metadata{nullptr};
    if(TDirectory* treedir = get_directory_from_chain(tree)) {
        if(auto md = treedir->Get("MetaData"); md != nullptr) {
            metadata = dynamic_cast<TTree*>(md);
        }
    }

    const xAOD::FileMetaData* md{nullptr};
    RETURN_CHECK("X2P", _tevent->retrieveMetaInput(md, "FileMetaData"));
    if(md) {
        md->value(xAOD::FileMetaData::amiTag, amitag);
        md->value(xAOD::FileMetaData::simFlavour, simflavor);
        std::transform(simflavor.begin(), simflavor.end(), simflavor.begin(), ::toupper);
    } else {
        std::cout << __x2pfunc__ << "WARNING Cannot get FileMetaData" << std::endl;
        amitag = "";
        simflavor = "";
    }

    return true;

}

TDirectory* AnalysisLooper::get_directory_from_chain(TTree* tree) {
    TDirectory* dir{nullptr};
    if(tree) {
        dir = tree->GetDirectory();
        if(!dir) {
            std::cout << __x2pfunc__ << "Trying to get directory from TChain" << std::endl;
            if(TChain* c = dynamic_cast<TChain*>(tree)) {
                if(TChainElement* ce = static_cast<TChainElement*>(c->GetListOfFiles()->First())) {
                    TFile* first_file = TFile::Open(ce->GetTitle());
                    dir = static_cast<TDirectory*>(first_file);
                }
            }
        }
    }
    std::cout << __x2pfunc__ << "Got directory from input: " << (dir ? dir->GetName() : "NULL") << std::endl;
    return dir;

}

bool AnalysisLooper::initialize_susytools() {

    std::cout << __x2pfunc__ << "Initializing SUSYTools" << std::endl;
    std::string name{"SUSYTools_X2P"};
    _susytools = std::make_unique<ST::SUSYObjDef_xAOD>(name);

    std::stringstream config;
    config << "xaod_to_parquet/SUSYTools_X2P.conf";
    RETURN_CHECK("X2P", _susytools->setProperty("ConfigFile", config.str()));
    RETURN_CHECK("X2P", _susytools->setProperty("mcCampaign", _mctype));
    _susytools->msg().setLevel(MSG::DEBUG);
    ST::ISUSYObjDef_xAODTool::DataSource datasource = _is_af2 ? ST::ISUSYObjDef_xAODTool::AtlfastII : ST::ISUSYObjDef_xAODTool::FullSim;
    RETURN_CHECK("X2P", _susytools->setProperty("DataSource", datasource));

    // add prw configs and lumicalc files
    std::vector<std::string> lumicalc_files = get_lumicalc_files();
    std::vector<std::string> prw_files = get_prw_files();

	std::cout << "=============================================================" << std::endl;
	std::cout << __x2pfunc__ << "Loading lumicalc files into SUSYTools:" << std::endl;
	for(const auto& f : lumicalc_files) {
		std::cout << __x2pfunc__ << " > " << f << std::endl;
	}
	std::cout << __x2pfunc__ << "Loading PRW config files into SUSYTools:" << std::endl;
	for(const auto& f : prw_files) {
		std::cout << __x2pfunc__ << " > " << f << std::endl;
	}
	std::cout << "=============================================================" << std::endl;
	RETURN_CHECK("X2P", _susytools->setProperty("PRWLumiCalcFiles", lumicalc_files));
	RETURN_CHECK("X2P", _susytools->setProperty("PRWConfigFiles", prw_files));

	if(_susytools->initialize() != StatusCode::SUCCESS) {
		std::cout << __x2pfunc__ << "Cannot initialize SUSYTools" << std::endl;
		return false;
	}
    return true;
}

std::vector<std::string> AnalysisLooper::get_lumicalc_files() {
    std::vector<std::string> lumicalc_files;
    bool lumicalc_ok{true};

    // 2015
    std::string lumicalc_2015_path{"GoodRunsLists/data15_13TeV/20170619/PHYS_StandardGRL_All_Good_25ns_276262-284484_OflLumi-13TeV-008.root"};
    std::string lumicalc_2015 = PathResolverFindCalibFile(lumicalc_2015_path);
    if(lumicalc_2015.empty()) {
        std::cout << __x2pfunc__ << "ERROR PathResolver unable to find 2015 lumicalc file (=" << lumicalc_2015_path << ")" << std::endl;
        lumicalc_ok = false;
    }
    lumicalc_files.push_back(lumicalc_2015);

    // 2016
    std::string lumicalc_2016_path{"GoodRunsLists/data16_13TeV/20180129/PHYS_StandardGRL_All_Good_25ns_297730-311481_OflLumi-13TeV-009.root"};
    std::string lumicalc_2016 = PathResolverFindCalibFile(lumicalc_2016_path);
    if(lumicalc_2016.empty()) {
        std::cout << __x2pfunc__ << "ERROR PathResolver unable to find 2016 lumicalc file (=" << lumicalc_2016_path << ")" << std::endl;
        lumicalc_ok = false;
    }
    lumicalc_files.push_back(lumicalc_2016);

    // 2017
    std::string lumicalc_2017_path{"GoodRunsLists/data17_13TeV/20180619/physics_25ns_Triggerno17e33prim.lumicalc.OflLumi-13TeV-010.root"};
    std::string lumicalc_2017 = PathResolverFindCalibFile(lumicalc_2017_path);
    if(lumicalc_2017.empty()) {
        std::cout << __x2pfunc__ << "ERROR PathResolver unable to find 2017 lumicalc file (=" << lumicalc_2017_path << ")" << std::endl;
        lumicalc_ok = false;
    }
    lumicalc_files.push_back(lumicalc_2017);

    // 2018
    std::string lumicalc_2018_path{"GoodRunsLists/data18_13TeV/20181111/ilumicalc_histograms_None_348885-364292_OflLumi-13TeV-001.root"};
    std::string lumicalc_2018 = PathResolverFindCalibFile(lumicalc_2018_path);
    if(lumicalc_2018.empty()) {
        std::cout << __x2pfunc__ << "ERROR PathResolver unable to find 2018 lumicalc file (=" << lumicalc_2018_path << ")" << std::endl;
        lumicalc_ok = false;
    }
    lumicalc_files.push_back(lumicalc_2018);

    if(!lumicalc_ok) {
        throw std::runtime_error("Failed to retrieve ilumicalc files");
    }

    std::cout << __x2pfunc__ << "Loading " << lumicalc_files.size() << " lumicalc files:" << std::endl;
    for(const auto& f : lumicalc_files) {
        std::cout << __x2pfunc__ << " > " << f << std::endl;
    }
    return lumicalc_files;
}

std::vector<std::string> AnalysisLooper::get_prw_files() {
    const xAOD::EventInfo* ei{nullptr};
    if(!_tevent->retrieve(ei, "EventInfo").isSuccess()) {
		throw std::runtime_error("Failed to retrieve EventInfo");
    }

    bool file_ok{true};
    std::vector<std::string> prw_out;
    int dsid = ei->mcChannelNumber();
    std::string sim_type = (_is_af2 ? "AFII" : "FS");
    
    std::stringstream dsid_s;
    dsid_s << dsid;
    std::string first3_of_dsid = dsid_s.str();
    first3_of_dsid = first3_of_dsid.substr(0,3);
    std::stringstream prw_dir;
    prw_dir << "dev/PileupReweighting/share/";
    prw_dir << "DSID" << first3_of_dsid << "xxx/";
    
    std::string prw_config = prw_dir.str();
    
    //std::string prw_config = PathResolverFindCalibDirectory(prw_config_loc);
    //if(prw_config == "")
    //{
    //    std::cout << __x2pfunc__ << "    WARNING Could not locate "
    //        "group area (=" << prw_config_loc << "), will try the older one: ";
    //    prw_config_loc = "dev/PileupReweighting/mc16_13TeV/";
    //    std::cout << prw_config_loc << std::endl;
    //
    //    prw_config = PathResolverFindCalibDirectory(prw_config_loc);
    //    if(prw_config == "")
    //    {
    //        std::cout << __x2pfunc__ << "    ERROR Could not locate "
    //            "alternative group area (=" << prw_config_loc << "), failed to find"
    //            " PRW config files for sample!" << std::endl;
    //        exit(1);
    //    }
    //}
    
    std::string prw_config_a = prw_config;
    std::string prw_config_d = prw_config;
    std::string prw_config_e = prw_config;
    
    prw_config_a += "pileup_mc16a_dsid" + std::to_string(dsid) + "_" + sim_type + ".root";
    prw_config_d += "pileup_mc16d_dsid" + std::to_string(dsid) + "_" + sim_type + ".root";
    prw_config_e += "pileup_mc16e_dsid" + std::to_string(dsid) + "_" + sim_type + ".root";
    
    std::cout << "================================================" << std::endl;
    std::cout << __x2pfunc__ << "PRW CONFIG directory : " << prw_config << std::endl;
    
    //TFile test(prw_config_a.data(), "read");
    //if(test.IsZombie()) {
    //    std::cout << __x2pfunc__ << "    ERROR Unable to open the mc16a PRW config file (=" << prw_config_a << ")" << std::endl;
    //    file_ok = false;
    //}
    //else {
    //    std::cout << __x2pfunc__ <<  "  >> File opened OK (" << prw_config_a << ")" << std::endl;
    //}
    //
    //TFile test2(prw_config_d.data(), "read");
    //if(test2.IsZombie()) {
    //    std::cout << __x2pfunc__ << "    ERROR Unable to open the mc16d PRW config file (=" << prw_config_d << ")" << std::endl;
    //    file_ok = false;
    //}
    //else {
    //    std::cout << __x2pfunc__ << "  >> File opened OK (" << prw_config_d << ")" << std::endl;
    //}
    //
    //TFile test3(prw_config_e.data(), "read");
    //if(test3.IsZombie()) {
    //    std::cout << __x2pfunc__ << "    ERROR Unable to open the mc16e PRW config file (=" << prw_config_e << ")" << std::endl;
    //    file_ok = false;
    //}
    //else {
    //    std::cout << __x2pfunc__ << "  >> File opened OK (" << prw_config_e << ")" << std::endl;
    //}
    //std::cout << "================================================" << std::endl;
    //
    //if(!file_ok) exit(1);
    
    prw_out.push_back(prw_config_a);
    prw_out.push_back(prw_config_d);
    prw_out.push_back(prw_config_e);

    return prw_out;

}

void AnalysisLooper::Begin(TTree* tree) {
    return;
}

Bool_t AnalysisLooper::Process(Long64_t entry) {
    return true;
}

void AnalysisLooper::Terminate() {
}


} // namespace x2p
