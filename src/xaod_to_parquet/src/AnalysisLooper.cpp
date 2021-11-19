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
    return true;
}

std::vector<std::string> AnalysisLooper::get_lumicalc_files() {
    std::vector<std::string> lumicalc_files;
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
