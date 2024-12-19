#ifndef SIM_BP_H
#define SIM_BP_H

typedef struct bp_params{
    unsigned long int K;  // K: PC bits for chooser table
    unsigned long int M1; // M1: PC bits for gshare index
    unsigned long int M2; // M2: PC bits for bimodal index
    unsigned long int N;  // N: Global history bits
    char*             bp_name; // tracefile: Input trace file
}bp_params;

// Put additional data structures here as per your requirement

#endif


