#ifndef ANALYSIS_LOOPER_H
#define ANALYSIS_LOOPER_H

// std/stl
#include <vector>
#include <memory>
#include <string>

// xAOD
#include "xAODRootAccess/tools/ReturnCheck.h"
#include "xAODEventInfo/EventInfo.h"
namespace xAOD {
    class TEvent;
    class TStore;
}
#include "SUSYTools/SUSYObjDef_xAOD.h"

// xAOD object containers
#include "xAODMuon/MuonContainer.h"
#include "xAODEgamma/ElectronContainer.h"
#include "xAODJet/JetContainer.h"
#include "xAODMissingET/MissingETContainer.h"
#include "xAODMissingET/MissingETAuxContainer.h"

#include "JetMomentTools/JetVertexTaggerTool.h"


// ASG
#include "AsgTools/ToolHandle.h"

// ROOT
#include "TSelector.h"
#include "TChain.h"
#include "TStopwatch.h"

namespace x2p {

    template <typename T>
    struct PtGreater {
        bool operator() (const T* a, const T* b) { return a->pt() > b->pt(); };
    };

class AnalysisLooper : public TSelector
{
    public :
        AnalysisLooper();
        virtual ~AnalysisLooper() = default;

        void set_chain(TChain* chain) { _chain = chain; }


        // TSelector implementation
        void Init(TTree* tree);
        void Begin(TTree* tree);
        Bool_t Notify() { return kTRUE; }
        void Terminate();
        Int_t Version() const { return 2; }
        Bool_t Process(Long64_t entry);

    protected :

        void get_sumw();
        bool process_cutbookkeeper(xAOD::TEvent& event, int file_counter);
        bool get_metadata_info(TTree* tree, std::string& simflavor, std::string& amitag);
        TDirectory* get_directory_from_chain(TTree* tree);

        bool initialize_susytools();
        std::vector<std::string> get_lumicalc_files();
        std::vector<std::string> get_prw_files();
        bool load_objects();


        //TStopwatch* timer() { return &_timer; }
        //std::string timer_summary();


    private :

        TChain* _chain;

        uint64_t _n_events_processed;
        double _sumw;
        double _sumw2;

        // xAOD
        std::unique_ptr<xAOD::TEvent> _tevent;
        std::unique_ptr<xAOD::TStore> _tstore;

        // flags
        bool _is_af2;
        std::string _mctype;

        // Tools
        std::unique_ptr<ST::SUSYObjDef_xAOD> _susytools;

        // Object containers
        std::unique_ptr<xAOD::ElectronContainer*> _electrons;
        std::unique_ptr<xAOD::ShallowAuxContainer*> _electrons_aux;
        std::unique_ptr<xAOD::MuonContainer*> _muons;
        std::unique_ptr<xAOD::ShallowAuxContainer*> _muons_aux;
        std::unique_ptr<xAOD::JetContainer*> _jets;
        std::unique_ptr<xAOD::ShallowAuxContainer*> _jets_aux;
        std::unique_ptr<xAOD::MissingETContainer*> _met;
        std::unique_ptr<xAOD::ShallowAuxContainer*> _met_aux;

        std::vector<xAOD::IParticle*> _base_electrons;
        std::vector<xAOD::IParticle*> _sig_eletrons;
        std::vector<xAOD::IParticle*> _base_muons;
        std::vector<xAOD::IParticle*> _sig_muons;
        std::vector<xAOD::IParticle*> _base_leptons;
        std::vector<xAOD::IParticle*> _sig_leptons;
        std::vector<xAOD::Jet*> _base_jets;
        std::vector<xAOD::Jet*> _sig_jets;

}; // class AnalysisLooper

} // namespace x2p



#endif
