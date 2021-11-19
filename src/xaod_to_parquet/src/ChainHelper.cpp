#include "xaod_to_parquet/ChainHelper.h"
#include "xaod_to_parquet/string_helpers.h"
#include "TObjArray.h"
#include "TChainElement.h"
#include "TKey.h"

#include <iostream>

using namespace std;

/*--------------------------------------------------------------------------------*/
// Build TChain from input fileList
/*--------------------------------------------------------------------------------*/
ChainHelper::Status ChainHelper::addFile(TChain* chain, string file)
{
  if(!file.empty()) chain->Add(file.c_str());
  return GOOD;
}
/*--------------------------------------------------------------------------------*/
// Test if file has expected TTree object
/*--------------------------------------------------------------------------------*/
bool ChainHelper::hasChainName(TChain* chain, string filename)
{
    bool has_tree = true;
    TFile* tmp_file = TFile::Open(filename.c_str());
    has_tree = tmp_file->GetListOfKeys()->Contains(chain->GetName());
    if(!has_tree) {
        cout << "ChainHelper WARNING Input file (" << filename << ") does not"
             << " contain requested TTree object (" << chain->GetName() << ")"
             << endl;
        cout << "ChainHelper WARNING This file will not be included in "
             << "the final TChain" << endl;
    }
    delete tmp_file;
    return has_tree;
}

/*--------------------------------------------------------------------------------*/
// Build TChain from input fileList
/*--------------------------------------------------------------------------------*/
ChainHelper::Status ChainHelper::addFileList(TChain* chain, string fileListName)
{
  if(!fileListName.empty()){
    ifstream fileList(fileListName.c_str());
    if(!fileList.is_open()){
      cout << "ERROR opening fileList " << fileListName << endl;
      return BAD;
    }
    string fileName;
    while(fileList >> fileName){
      // Only consider files that have the TTree we request
      if(hasChainName(chain, fileName)) {
        // Add protection against file read errors
        if(chain->Add(fileName.c_str(), -1)==0){
            cerr << "ChainHelper ERROR adding file " << fileName << endl;
            return BAD;
        }
      }
    }
    fileList.close();
  }
  return GOOD;
}

/*--------------------------------------------------------------------------------*/
// Build TChain from input directory
/*--------------------------------------------------------------------------------*/
ChainHelper::Status ChainHelper::addFileDir(TChain* chain, string fileDir)
{
  if(!fileDir.empty()){
    string files = fileDir + "/*";
    chain->Add(files.c_str());
  }
  return GOOD;
}
//----------------------------------------------------------
ChainHelper::Status ChainHelper::addInput(TChain* chain, const std::string &input, bool verbose)
{
    Status status=GOOD;
    using namespace Susy::utils;
    if(contains(input, ",")){
        size_t num_added=0;
        std::vector< std::string > tokens = tokenizeString(input, ',');
        for(size_t i=0; i<tokens.size(); ++i)
            if(GOOD==addInput(chain, tokens[i], verbose)) num_added++;
        status = (num_added==tokens.size() ? GOOD : BAD);
    } else if(inputIsFile(input)){
        if(verbose) cout<<"ChainHelper::addInput adding file "<<input<<endl;
        status = ChainHelper::addFile(chain, input);
    }
    else if(inputIsList(input)){
        if(verbose) cout<<"ChainHelper::addInput adding list "<<input<<endl;
        status = ChainHelper::addFileList(chain, input);
    }
    else if(inputIsDir(input)){
        if(verbose) cout<<"ChainHelper::addInput adding dir "<<input<<endl;
        status = ChainHelper::addFileDir(chain, input);
    }
    else {
        cout<<"ChainHelper::addInput: cannot determine whether the input (=" << input << ") is a file/filelist/dir"<<endl;
        cout<<"ChainHelper::addInput: Provide a valid input or use addFile/addFileList/addFileDir"<<endl;
        status = BAD;
    }
    return status;
}
//----------------------------------------------------------
std::string ChainHelper::firstFile(const std::string &input, bool verbose)
{
    string filename;
    string defaultTreename = "susyNt";
    TChain dummyChain(defaultTreename.c_str());
    if(addInput(&dummyChain, input, verbose)==GOOD) {
        TIter next(dummyChain.GetListOfFiles());
        if(TChainElement *chEl = static_cast<TChainElement*>(next())) {
            filename = chEl->GetTitle();
        }
    } else {
        if(verbose)
            cout<<"ChainHelper::firstFile(): invalid input, cannot provide first file"<<endl;
    }
    return filename;
}
//----------------------------------------------------------
bool ChainHelper::inputIsFile(const std::string &input)
{
    return Susy::utils::contains(Susy::utils::rmLeadingTrailingWhitespaces(input), ".root");
}
//----------------------------------------------------------
bool ChainHelper::inputIsList(const std::string &input)
{
    return Susy::utils::endswith(Susy::utils::rmLeadingTrailingWhitespaces(input), ".txt");
}
//----------------------------------------------------------
bool ChainHelper::inputIsDir(const std::string &input)
{
    return Susy::utils::endswith(Susy::utils::rmLeadingTrailingWhitespaces(input), "/");
}
//----------------------------------------------------------
std::string ChainHelper::sampleName(const std::string &input, bool verbose)
{
    string containerName = "outputContainerName"; // see SusyNtMaker::writeMetaData()
    return ChainHelper::readMetadataTitle(input, containerName, verbose);
}
//----------------------------------------------------------
std::string ChainHelper::parentSampleName(const std::string &input, bool verbose)
{
    string containerName = "inputContainerName";
    return ChainHelper::readMetadataTitle(input, containerName, verbose);
}
//----------------------------------------------------------
std::string ChainHelper::readMetadataTitle(const std::string &input, const std::string tobjectName, bool verbose)
{
    string sampleName;
    bool fileFound(false), containerFound(false);
    string fileName = ChainHelper::firstFile(input, verbose);
    if(TFile *inputFile = TFile::Open(fileName.c_str())) {
        fileFound = true;
        if(TKey *container = inputFile->FindKey(tobjectName.c_str())) {
            containerFound = true;
            sampleName = container->GetTitle();
        }
        inputFile->Close();
        inputFile->Delete();
    }
    if(verbose && !containerFound) {
        cout<<"ChainHelper::sampleName: failed."<<endl
            <<" fileFound: "<<fileFound<<" (firstFile '"<<fileName<<"')"<<endl
            <<" containerFound "<<containerFound<<" (TObject '"<<tobjectName<<"')"<<endl;
    }
    return sampleName;
}
//----------------------------------------------------------

