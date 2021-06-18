To run the code do: 
mpirun -n <NUMNODES> -ppn <NUMNODES / 2> --host 10.12.1.1,10.12.1.2 ./main <TASKS> <RUNMODE> <ITERATIONS>

RUNMODE: 0 = only run on local rank
         1 = run on remote and local ranks
         2 = compare remote+local to only local runtime