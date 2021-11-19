// std/stl
#include <iostream>
#include <fstream>

// CLI
#include "xaod_to_parquet/CLI11.hpp"

// ATLAS
#include "xAODRootAccess/Init.h"
#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/tools/ReturnCheck.h"

// ROOT
#include "TFile.h"

// parquet-writer
#include "parquet_writer.h"

int main(int argc, char* argv[]) {

    namespace pw = parquetwriter;

    std::string layout_filepath{""};
    uint32_t n_events{10};

    // parse the user input
    CLI::App app;
    app.add_option("--layout", layout_filepath, "Input parquet-writer layout JSON file")->required();
    app.add_option("-n,--n-events", n_events, "Number of events to process");
    CLI11_PARSE(app, argc, argv);

    pw::Writer writer;
    std::ifstream layout_file(layout_filepath);
    writer.set_layout(layout_file);
    writer.set_dataset_name("my_dataset");
    writer.initialize();

    float foo_data{42.0};
    uint32_t bar_data{42};
    std::vector<float> baz_data{42.0, 42.1, 42.2};

    for(uint32_t ievent = 0; ievent < n_events; ievent++) {
        writer.fill("foo", foo_data);
        writer.fill("bar", bar_data);
        writer.fill("baz", baz_data);
        writer.end_row();
    } // ievent

    writer.finish();
    return 0;
}
