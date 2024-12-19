# Branch-predictor-simulator
This project implements a branch predictor simulator that evaluates different configurations of branch predictors, including:
1.Bimodal predictor
2.Gshare predictor
3.Hybrid predictor 


Type "make" to build

To run the trace file: 
  bimodal predictor:
    type "./sim bimodal <M2> <tracefile>"

   gshare predictor:
   type "./sim gshare <M1> <N> <tracefile>"

   hybrid predictor:
   type "./sim hybrid <K> <M1> <N> <M2> <tracefile>"

K: Number of PC bits used to index the chooser table

M1: Number of PC bits for gshare table

N: Number of global branch history register bits

M2: Number of PC bits for bimodal table

For example:

./sim bimodal 6 harrys_gcc_trace_test.txt

./sim gshare 9 3 harrys_gcc_trace_test.txt

./sim hybrid 8 14 10 5 harrys_gcc_trace_test.txt



---------------------------------
Trace file Input Format
The simulator reads trace files in the following format:

<hex: branch PC> t|n

<hex: branch PC>: Branch instruction address in memory

t: Branch was taken

n: Branch was not taken

To learn more: see the harrys_gcc_trace_test.txt 

---------------------------------
Output
The simulator outputs:

  1.Simulator command and configuration
  
  2.Performance measurements:
  
    Number of predictions
    
    Number of mispredictions
    
    Misprediction rate
    
  3.Final contents of the branch predictor

   


