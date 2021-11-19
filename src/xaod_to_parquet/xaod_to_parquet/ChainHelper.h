#ifndef X2P_CHAINHELPER_H
#define X2P_CHAINHELPER_H

#include <fstream>

#include "TFile.h"
#include "TChain.h"

/**
   Static helper methods to build a TChain from input root files

   The recommended method is ChainHelper::addInput().
   The input can be one of the following
   - single input root file
   - input directory of root files
   - a file with list of root files
   - comma-separated list of any of the above
*/

class ChainHelper
{

  public:

    //ChainHelper(std::string treeName = "");
    //~ChainHelper(){};

    enum Status {
      GOOD = 0,
      BAD = 1
    };

    /// Check if the file has the expected TTree object
    static bool hasChainName(TChain* chain, std::string filename);

    /// Add a file - not very useful (obsolete, use addInput() instead)
    static Status addFile(TChain* chain, std::string file);

    /// Add a fileList (obsolete, use addInput() instead)
    static Status addFileList(TChain* chain, std::string fileListName);

    // Add all files in a directory (obsolete, use addInput() instead)
    static Status addFileDir(TChain* chain, std::string fileDir);

    /// add generic input
    /**
       Determine internally whether it's a file, filelist, or
       directory. Also accepts comma-separated list of inputs.
     */
    static Status addInput(TChain* chain, const std::string &input, bool verbose=false);
    // get the first file from a generic input
    static std::string firstFile(const std::string &input, bool verbose=false);
    /// input files are expected to have the '.root' extension
    static bool inputIsFile(const std::string &input);
    /// input filelists are expected to have the '.txt' extension
    static bool inputIsList(const std::string &input);
    /// input directories are expected to end with '/'
    static bool inputIsDir(const std::string &input);
    /// name of the sample being processed
    /**
       read from metadata from the first root file
    */
    static std::string sampleName(const std::string &input, bool verbose);
    /// name of the sample from which the ntuple was generated
    /**
       read from metadata from the first root file
    */
    static std::string parentSampleName(const std::string &input, bool verbose);
 private:
    static std::string readMetadataTitle(const std::string &input, const std::string tobjectName, bool verbose);
};



#endif

