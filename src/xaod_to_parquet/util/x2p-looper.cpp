#include "xaod_to_parquet/AnalysisLooper.h"
#include "xaod_to_parquet/ChainHelper.h"

// std/stl
#include <iostream>
#include <fstream>

// CLI
#include "xaod_to_parquet/CLI11.hpp"

// xAOD
#include "xAODRootAccess/Init.h"

int main(int argc, char* argv[]) {

    std::string algo_name{argv[0]};
    std::string input_filepath{""};
    int n_to_process{-1};

    // parse the user input
    CLI::App app;
    app.add_option("-i,--input", input_filepath, "Input to process")->required();
    app.add_option("-n,--n-events", n_to_process, "Number of events to process");
    CLI11_PARSE(app, argc, argv);

    // initialize the xAOD environment
    xAOD::Init("X2P");
    //std::unique_ptr<TChain> chain = std::make_unique<TChain>("CollectionTree");
    TChain* chain = new TChain("CollectionTree");
    int err = ChainHelper::addInput(chain, input_filepath, true);
    if(err) return 1;
    chain->ls();
    Long64_t n_in_chain = chain->GetEntries();
    if(n_to_process < 0 || n_to_process > n_in_chain) n_to_process = n_in_chain;

    // build the analysis looper
    std::unique_ptr<x2p::AnalysisLooper> ana = std::make_unique<x2p::AnalysisLooper>();
    ana->set_chain(chain);

    std::cout << "===================================================================" << std::endl;
    std::cout << " Total number of entries in input chain : " << n_in_chain << std::endl;
    std::cout << " Total number of entires to process     : " << n_to_process << std::endl;
    std::cout << "===================================================================" << std::endl;
    chain->Process(ana.get(), input_filepath.c_str(), n_to_process);

    delete chain;
    std::cout << "Done." << std::endl;
    return 0;
}
