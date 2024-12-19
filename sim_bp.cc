#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_bp.h"
#include <iostream>
#include <iomanip> 
#include <cmath>
#include <tuple> // for tie function
#include <bitset>
#include <sstream>
#include <stdint.h>
#include <string>
using namespace std;

/* ====== Variable in the branch predictors ====== */
// K: PC bits for chooser table
// M1: PC bits for gshare
// N: Global history bits
// M2: PC bits for bimodal
// tracefile: Input trace file1

//output
int number_of_predictions = 0;
int number_of_mispredictions = 0;
double misprediction_rate = 0;


class bimodal_branch_predictor{
    public: 
    unsigned long int number_of_bimodal_PC_bits_for_index; //M2
    int* bimodal_prediction_table_for_smith_counter;
    unsigned int mask;
    unsigned long int index = 0;
    
    void create_cache__aka_prediction_table_for_smith_counter(unsigned long int M2){
        number_of_bimodal_PC_bits_for_index = M2;
        bimodal_prediction_table_for_smith_counter = new int[(1 << number_of_bimodal_PC_bits_for_index)]; //int(pow(2,M2)) <- use thie might cause a core dump as it return double instead of int, (1<<M2) = 2^(M2), just think it in binary can easily understand
        
        //check for segementation fault
        if (!bimodal_prediction_table_for_smith_counter) {
        std::cerr << "Bimodal memory allocation failed!" << std::endl;
        exit(EXIT_FAILURE);
        }

        for(int i=0; i<int (1 << number_of_bimodal_PC_bits_for_index); i++){ 
            bimodal_prediction_table_for_smith_counter[i] = 0; // 
        }
        return;
    };

    void initialize_prediction_table(){
        for(int i=0; i<int (1 << number_of_bimodal_PC_bits_for_index); i++){ //same as above, (1<<M2) = 2^M2
            bimodal_prediction_table_for_smith_counter[i] = 2; // 2-bit counter, SPEC requires to initial as 2(weakly taken)
        }
        return;
    };

    void build_a_mask_for_index(){
        mask = (1U << number_of_bimodal_PC_bits_for_index) - 1; //shift 1 n bit, then minus 1 can land number of number of PC bits(all bits are 1)
        mask = mask << 2; // shift left for two bit to land final mask (since we are going to discard last two bits in the PC)
        return;
    }

    char predict(unsigned long int addr, char actual_outcome){
        char bimodal_prediction = ' ';
        index = ((addr & mask ) >> 2); // use mask to retrieve the index bit only(upper tag bit will be zero), then right shift 2 bit to discard the lower two bit since they are also zero
        
        number_of_predictions = number_of_predictions + 1;

        //update the misprediction count
        if((bimodal_prediction_table_for_smith_counter[index] >= 2) && actual_outcome == 'n'){
            number_of_mispredictions = number_of_mispredictions + 1;
        }else if((bimodal_prediction_table_for_smith_counter[index] <= 1) && actual_outcome == 't'){
            number_of_mispredictions = number_of_mispredictions + 1;
        }

        if(bimodal_prediction_table_for_smith_counter[index] >= 2)
            bimodal_prediction = 't';
        else 
            bimodal_prediction = 'n';

        return bimodal_prediction;
    }

    void update_counter_in_prediction_table(char actual_outcome){
        //update the counter
        if((bimodal_prediction_table_for_smith_counter[index] >= 2) && actual_outcome == 't'){
            if(bimodal_prediction_table_for_smith_counter[index] < 3)
                bimodal_prediction_table_for_smith_counter[index] = bimodal_prediction_table_for_smith_counter[index] + 1;
        }else if((bimodal_prediction_table_for_smith_counter[index] <= 1) && actual_outcome == 'n'){
            if(bimodal_prediction_table_for_smith_counter[index] > 0)
                bimodal_prediction_table_for_smith_counter[index] = bimodal_prediction_table_for_smith_counter[index] - 1;
        }else if((bimodal_prediction_table_for_smith_counter[index] >= 2) && actual_outcome == 'n'){
            if(bimodal_prediction_table_for_smith_counter[index] > 0)
                bimodal_prediction_table_for_smith_counter[index] = bimodal_prediction_table_for_smith_counter[index] - 1;
        }else if((bimodal_prediction_table_for_smith_counter[index] <= 1) && actual_outcome == 't'){
            if(bimodal_prediction_table_for_smith_counter[index] < 3)
                bimodal_prediction_table_for_smith_counter[index] = bimodal_prediction_table_for_smith_counter[index] + 1;
        }
    }

    void display_bimodal_prediction_table_for_smith_counter_result(){

        std::cout << "FINAL BIMODAL CONTENTS" << endl;

        for(int i=0; i<int(1 << number_of_bimodal_PC_bits_for_index); i++){
            std::cout << " " << std::dec << i << "\t" << bimodal_prediction_table_for_smith_counter[i] << std::endl;
        }
    return;
    };
};



class gshare_branch_predictor{
    public:
    unsigned long int global_history_register = 0;
    unsigned long int number_of_global_history_bits = 0; // N
    unsigned long int number_of_gshare_PC_bits_for_index = 0; //M1
    int* gshare_prediction_table_for_smith_counter;
    unsigned int mask_to_clean_higher_bit_in_PC = 0;
    unsigned long int index = 0;
    char gshare_prediction = ' ';
    unsigned long int lower_PC_bits = 0;

    void create_cache__aka_prediction_table_for_smith_counter(unsigned long int M1, unsigned long int N){
        number_of_gshare_PC_bits_for_index = M1;
        number_of_global_history_bits = N; // just sent this value to this object, that's all
        gshare_prediction_table_for_smith_counter = new int[(1 << number_of_gshare_PC_bits_for_index)]; // like pow(2, number_of_gshare_PC_bits_for_index)
        
        if (!gshare_prediction_table_for_smith_counter) {
        std::cerr << "Gshare memory allocation failed!" << std::endl;
        exit(EXIT_FAILURE);
        }

        //initialize_prediction_table
        for(int i=0; i<int (1 << number_of_gshare_PC_bits_for_index); i++){ 
            gshare_prediction_table_for_smith_counter[i] = 0; 
        }
        return;
    };

    //SPEC didn't specifiy, but I guess this table initialize to weakly taken(2) as well
    void initialize_prediction_table(){
        for(int i = 0; i < (1 << number_of_gshare_PC_bits_for_index); i++){
            gshare_prediction_table_for_smith_counter[i] = 2;
        }
        return;
    };

    void build_a_mask_for_index(){
        mask_to_clean_higher_bit_in_PC = (1U << number_of_gshare_PC_bits_for_index) - 1;
        return;
    };

    char predict_and_update_global_history_register(unsigned long int addr, char actual_outcome){
        
        number_of_predictions = number_of_predictions + 1;

        addr = addr >> 2; //get rid of lower two bits since they always zero

        lower_PC_bits = addr & mask_to_clean_higher_bit_in_PC;

        index = (global_history_register << (number_of_gshare_PC_bits_for_index - number_of_global_history_bits)) ^ lower_PC_bits;

        //assign gshare prediction, just to make the code easy to read
        if(gshare_prediction_table_for_smith_counter[index] >= 2)
            gshare_prediction = 't'; //taken
        else
            gshare_prediction = 'n'; //not-taken
        
        //update number_of_mispredictions
        if(gshare_prediction != actual_outcome)
            number_of_mispredictions = number_of_mispredictions + 1;

        //update history register
        if(actual_outcome == 't'){
            // example: 0100 + 10000 = 10100, then shift one bit -> 1010 (for human, just shift one bit right then add 1 at the front)
            global_history_register = (1 << (number_of_global_history_bits)) | global_history_register; //global_history_register + (1 << (number_of_global_history_bits));
            global_history_register = global_history_register >> 1;
        }else{
            global_history_register = global_history_register >> 1; //just shift rigth one bit, first element should be "0" which means not-taken
        }

        return gshare_prediction;
    };

    void update_counter_in_prediction_table(char actual_outcome){ 
         //update smith counter
        if(actual_outcome == 't' && gshare_prediction_table_for_smith_counter[index] < 3)
            gshare_prediction_table_for_smith_counter[index] = gshare_prediction_table_for_smith_counter[index] + 1;
        else if(actual_outcome == 'n' && gshare_prediction_table_for_smith_counter[index] > 0)
            gshare_prediction_table_for_smith_counter[index] = gshare_prediction_table_for_smith_counter[index] - 1;
        return;
    }


    void display_gshare_prediction_table_for_smith_counter_result(){

        std::cout << "FINAL GSHARE CONTENTS" << endl;

        for(int i=0; i < (1 << number_of_gshare_PC_bits_for_index); i++){
            cout << std::dec << i << "\t" << gshare_prediction_table_for_smith_counter[i] << endl;
        }

        return;
    };

};

class hybrid_branch_predictor{
    public:
    unsigned long int number_of_PC_bits_for_chooser_table = 0; //K
    unsigned long int number_of_PC_bits_for_gshare_index = 0; //M1
    unsigned long int number_of_PC_bits_for_bimodal_index = 0; //N2
    unsigned long int number_of_global_history_bits = 0; //N
    int* chooser_table;
    unsigned long int mask_for_chooser_table = 0;
    unsigned long int index_for_chooser_table = 0;
    char overall_prediction = ' ';

    bimodal_branch_predictor* bimodal_predictor_in_hybrid;
    gshare_branch_predictor* gshare_predictor_in_hybrid;

    int number_of_predictions_local = 0;
    int number_of_mispredictions_local = 0;
    double misprediction_rate_local = 0;

    // Constructor
    hybrid_branch_predictor() {
        bimodal_predictor_in_hybrid = new bimodal_branch_predictor();
        gshare_predictor_in_hybrid = new gshare_branch_predictor();
    }

    void create_bimodal_and_gshare_prediction_table(unsigned long int K, unsigned long int M1, unsigned long int N, unsigned long int M2){
        number_of_PC_bits_for_chooser_table = K;
        number_of_PC_bits_for_gshare_index = M1;
        number_of_PC_bits_for_bimodal_index = M2;
        number_of_global_history_bits = N;

        bimodal_predictor_in_hybrid -> create_cache__aka_prediction_table_for_smith_counter(M2);
        gshare_predictor_in_hybrid -> create_cache__aka_prediction_table_for_smith_counter(M1, N);
    };

    void initialize_prediction_table(){ 
        bimodal_predictor_in_hybrid -> initialize_prediction_table();
        gshare_predictor_in_hybrid -> initialize_prediction_table();
    };

    void build_bimodal_and_gshare_mask(){
        bimodal_predictor_in_hybrid -> build_a_mask_for_index();
        gshare_predictor_in_hybrid -> build_a_mask_for_index();
    }

    void build_a_mask_for_chooser_table(){
        mask_for_chooser_table = (1 << number_of_PC_bits_for_chooser_table) - 1;
        mask_for_chooser_table = mask_for_chooser_table << 2;
        //std::cout << "\nmask: " << mask_for_chooser_table << endl;
    }

    void create_a_chooser_table(){
        chooser_table = new int[(1 << number_of_PC_bits_for_chooser_table)];
        for(int i = 0; i < (1 << number_of_PC_bits_for_chooser_table); i++){
            chooser_table[i] = 1; //initialize to 1 as SPEC required
        }
    }

    void predict__update_prediction_table_and_chooser_counter(unsigned long int addr, char actual_outcome){
        char bimodal_prediction = ' ';
        char gshare_prediction = ' ';

        number_of_predictions_local = number_of_predictions_local + 1;

        //step 1: obtain predictions from bimodal and gshare
        bimodal_prediction = bimodal_predictor_in_hybrid -> predict(addr, actual_outcome);
        gshare_prediction = gshare_predictor_in_hybrid -> predict_and_update_global_history_register(addr, actual_outcome);

        // std::cout << "\nbimodal_prediction: " << bimodal_prediction << endl;
        // std::cout << "\ngshare_prediction: " << gshare_prediction << endl;

        //step 2: determine the chooser table index
        index_for_chooser_table = (addr & mask_for_chooser_table) >> 2;

        //step 3: make overall prediction
        if(chooser_table[index_for_chooser_table] >= 2)
            overall_prediction = gshare_prediction;
        else
            overall_prediction = bimodal_prediction;
        
        //update mispredition count
        if(overall_prediction != actual_outcome)
            number_of_mispredictions_local = number_of_mispredictions_local + 1;

        //step 4: update selected branch predictor based on actual outcome
        if(chooser_table[index_for_chooser_table] >= 2)
            gshare_predictor_in_hybrid -> update_counter_in_prediction_table(actual_outcome);
        else
            bimodal_predictor_in_hybrid -> update_counter_in_prediction_table(actual_outcome);
        
        //step 5: update BHT in gshare, automatically done when landing the gshare_prediction

        //step 6:update branch's chooser counter
        if(gshare_prediction == actual_outcome && bimodal_prediction == actual_outcome){
            //do nothing;
        }else if (gshare_prediction != actual_outcome && bimodal_prediction != actual_outcome){
            //do nothing;
        }else if (gshare_prediction == actual_outcome && bimodal_prediction != actual_outcome){
            if(chooser_table[index_for_chooser_table] < 3)
                chooser_table[index_for_chooser_table] = chooser_table[index_for_chooser_table] + 1;
        }else if (gshare_prediction != actual_outcome && bimodal_prediction == actual_outcome)
            if(chooser_table[index_for_chooser_table] > 0)
                chooser_table[index_for_chooser_table] = chooser_table[index_for_chooser_table] - 1;
    }

    void display_choser_table_and_prediction_tables(){

        if (number_of_predictions_local > 0) {
            misprediction_rate_local = (static_cast<double>(number_of_mispredictions_local) / static_cast<double>(number_of_predictions_local)) * 100;
        } else {
            misprediction_rate_local = 0; // Handle case with no predictions
        }

        std::cout << "OUTPUT" << std::endl;
        std::cout << " number of predictions:    " << std::dec << number_of_predictions_local << endl;
        std::cout << " number of mispredictions: " << std::dec << number_of_mispredictions_local << endl;
        std::cout << " misprediction rate:       " << std::fixed << std::setprecision(2) << misprediction_rate_local << "%" << endl;

        std::cout << "FINAL CHOOSER CONTENTS" << endl;

        for(int i = 0; i < (1 << number_of_PC_bits_for_chooser_table); i++){
            std::cout << std::dec << i << "\t" << chooser_table[i] << endl;
        }
        gshare_predictor_in_hybrid -> display_gshare_prediction_table_for_smith_counter_result();
        bimodal_predictor_in_hybrid -> display_bimodal_prediction_table_for_smith_counter_result();

        
    }
};


/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim bimodal 6 gcc_trace.txt
    argc = 4
    argv[0] = "sim"
    argv[1] = "bimodal"
    argv[2] = "6"
    ... and so on
*/
int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    bp_params params;       // look at sim_bp.h header file for the the definition of struct bp_params
    char outcome;           // Variable holds branch outcome
    //addr is the address hold by PC
    unsigned long int addr; // Variable holds the address read from input file (I think it refers to gcc_trace.txt)
    
    //Harry initialize_prediction_table all the parameters to zero 
    params.K = 0;
    params.M1 = 0;
    params.M2 = 0;
    params.N = 0;
    //params.bp.name 

    if (!(argc == 4 || argc == 5 || argc == 7))
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.bp_name  = argv[1];
    
    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    //strcmp is a string compartor, if equal then output 0
    if(strcmp(params.bp_name, "bimodal") == 0)             // Bimodal
    {
        if(argc != 4)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M2       = strtoul(argv[2], NULL, 10);
        trace_file      = argv[3];
        printf("COMMAND\n%s %s %lu %s\n", argv[0], params.bp_name, params.M2, trace_file);
    }
    else if(strcmp(params.bp_name, "gshare") == 0)          // Gshare
    {
        if(argc != 5)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M1       = strtoul(argv[2], NULL, 10);
        params.N        = strtoul(argv[3], NULL, 10);
        trace_file      = argv[4];
        printf("COMMAND\n%s %s %lu %lu %s\n", argv[0], params.bp_name, params.M1, params.N, trace_file);

    }
    else if(strcmp(params.bp_name, "hybrid") == 0)          // Hybrid
    {
        if(argc != 7)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.K        = strtoul(argv[2], NULL, 10);
        params.M1       = strtoul(argv[3], NULL, 10);
        params.N        = strtoul(argv[4], NULL, 10);
        params.M2       = strtoul(argv[5], NULL, 10);
        trace_file      = argv[6];
        printf("COMMAND\n%s %s %lu %lu %lu %lu %s\n", argv[0], params.bp_name, params.K, params.M1, params.N, params.M2, trace_file); //required output 1
    }
    else
    {
        printf("Error: Wrong branch predictor name:%s\n", params.bp_name);
        exit(EXIT_FAILURE);
    }
    
    //Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    char str[2]; //I think we actually only need one str[1] instead of two.


/* =================================Harry's code start from here ===========*=================================*/

    // Create instances of the classes
    bimodal_branch_predictor bimodal_predictor;
    gshare_branch_predictor gshare_predictor;
    hybrid_branch_predictor hybrid_predictor;

    
    //Create branch history table, initialize_prediction_table the value, and setup mask
    if(strcmp(params.bp_name, "bimodal") == 0){
        bimodal_predictor.create_cache__aka_prediction_table_for_smith_counter(params.M2);
        bimodal_predictor.initialize_prediction_table();
        bimodal_predictor.build_a_mask_for_index(); 
    } else if(strcmp(params.bp_name, "gshare") == 0){
        gshare_predictor.create_cache__aka_prediction_table_for_smith_counter(params.M1, params.N);
        gshare_predictor.initialize_prediction_table();
        gshare_predictor.build_a_mask_for_index(); 
    } else if(strcmp(params.bp_name, "hybrid") == 0){
        hybrid_predictor.create_bimodal_and_gshare_prediction_table(params.K, params.M1, params.N, params.M2);
        hybrid_predictor.create_a_chooser_table();
        hybrid_predictor.initialize_prediction_table();
        hybrid_predictor.build_bimodal_and_gshare_mask();
        hybrid_predictor.build_a_mask_for_chooser_table();
    } else{
        printf("Error: None of these three predictors\n");
    } 

    // printf("test, created\n");
    while(fscanf(FP, "%lx %s", &addr, str) != EOF)
    {
        
        outcome = str[0];
        // if (outcome == 't'){
        //     printf("%lx %s\n", addr, "t");           // Print and test if file is read correctly
        // } else if (outcome == 'n'){
        //     printf("%lx %s\n", addr, "n");          // Print and test if file is read correctly
        // }
            
        /************************************* Harry's branch predictor code start here */

        if(strcmp(params.bp_name, "bimodal") == 0){
            bimodal_predictor.predict(addr, outcome);
            bimodal_predictor.update_counter_in_prediction_table(outcome);
        } else if(strcmp(params.bp_name, "gshare") == 0){
            gshare_predictor.predict_and_update_global_history_register(addr, outcome);
            gshare_predictor.update_counter_in_prediction_table(outcome);
        } else if(strcmp(params.bp_name, "hybrid") == 0){
            hybrid_predictor.predict__update_prediction_table_and_chooser_counter(addr, outcome);
        } else{
            printf("Error: None of these three predictors\n");
        } 

        /************************************** Harry's branch predictor code end here */
    }


    //if not hybrid, then print this
    if(strcmp(params.bp_name, "hybrid") != 0){
        if (number_of_predictions > 0) {
            misprediction_rate = (static_cast<double>(number_of_mispredictions) / static_cast<double>(number_of_predictions)) * 100;
        } else {
            misprediction_rate = 0; // Handle case with no predictions
        }
        std::cout << "OUTPUT" << std::endl;
        std::cout << " number of predictions:    " << std::dec << number_of_predictions << endl;
        std::cout << " number of mispredictions: " << std::dec << number_of_mispredictions << endl;
        std::cout << " misprediction rate:       " << std::fixed << std::setprecision(2) << misprediction_rate << "%" << endl;
    }
    


    if(strcmp(params.bp_name, "bimodal") == 0){
        bimodal_predictor.display_bimodal_prediction_table_for_smith_counter_result();
    } else if(strcmp(params.bp_name, "gshare") == 0){
        gshare_predictor.display_gshare_prediction_table_for_smith_counter_result();
    } else if(strcmp(params.bp_name, "hybrid") == 0){
        hybrid_predictor.display_choser_table_and_prediction_tables();
    } else{
        printf("Error: None of these three predictors\n");
    } 

    return 0;
}


