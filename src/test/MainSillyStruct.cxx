
#include <stdio.h>

#include "TBranch.h"
#include "TFile.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"

#include "SillyStruct.h"

int main(int argc, char *argv[]) {

    TFile *hfile;
    TTree *tree;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s [read|write]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "read") == 0) {
        hfile = new TFile("SillyStruct.root");
        TTreeReader myReader("T", hfile);
        TTreeReaderValue<float> a(myReader, "a");
        TTreeReaderValue<int> b(myReader, "b");
        TTreeReaderValue<double> c(myReader, "c");
        TTreeReaderValue<SillyStruct> ss(myReader, "myEvent");
        while (myReader.Next()) {
            std::cout << "A=" << *a << ", B=" << *b << ", C=" << *c << "\n";
        }
    } else if (strcmp(argv[1], "write") == 0) {
        hfile = new TFile("SillyStruct.root","RECREATE","TTree benchmark ROOT file");
        hfile->SetCompressionLevel(1);
        tree = new TTree("T", "An example ROOT tree of SillyStructs.");
        SillyStruct ss;
        TBranch *branch = tree->Branch("myEvent", &ss, 32000, 1);
        branch->SetAutoDelete(kFALSE);
        ss.b = 2;
        ss.c = 3;
        for (int ev = 0; ev < 10; ev++) {
          ss.a = ev+1;
          tree->Fill();
        }
        hfile = tree->GetCurrentFile();
        hfile->Write();
        tree->Print();
    } else {
        fprintf(stderr, "Unknown command: %s.\n", argv[1]);
        return 1;
    }
    hfile->Close();

    return 0;
}
