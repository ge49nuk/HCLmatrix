/*
 *
 * This example is referer from the queue_test example in part of hcl-src.
 *
 * HCL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * https://github.com/scs-lab/hcl
 *
 */

#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <queue>
#include <fstream>
#include <atomic>

// for mpi, omp
#include <mpi.h>
#include <omp.h>

// for hcl data-structures
#include <hcl/common/data_structures.h>
#include <hcl/queue/queue.h>


// ================================================================================
// Global Variables
// ================================================================================


// ================================================================================
// Util-functions
// ================================================================================

void initialize_matrix_rando(double *mat_ptr, int size){
    double low_bnd = 0.0;
    double upp_bnd = 10.0;
    std::uniform_real_distribution<double> ur_dist(low_bnd, upp_bnd);
    std::default_random_engine dre;
    for (int i = 0; i < size*size; i++){
        mat_ptr[i] = ur_dist(dre);
    }
}

void initialize_matrix_zeros(double *mat_ptr, int size){
    for (int i = 0; i < size*size; i++){
        mat_ptr[i] = 0.0;
    }
}

void compute_mxm(double *a, double *b, double *c, int size){
    
    // main loop to compute as a serial way
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            c[i*size + j] = 0;
            for (int k = 0; k < size; k++) {
                c[i*size + j] += a[i*size + k] * b[k*size + j];
            }
        }
    }
}

void mxm_kernel(double *A, double *B, double *C, int size, int i){

    // call the compute entry
    compute_mxm(A, B, C, size);
}

// ================================================================================
// Struct Definition
// ================================================================================
struct mat_task {
    int size;
    double *A;  // ptr to the allocated matrix A
    double *B;  // ptr to the allocated matrix B
    double *C;  // ptr to the allocated matrix C - result
};

struct arr_mat_task {
    const int size;
    double A[size*size];
    double B[size*size];
    double C[size*size];

    // Constructor 1
    arr_mat_task(int s){
        initialize_matrix_rando(&A, s);
        initialize_matrix_rando(&B, s);
        initialize_matrix_zeros(&C, s);
    }
};

typedef struct general_task_t {
    int id;
    int32_t idx_image = 0;
    int32_t arg_num;
    std::vector<void *> arg_hst_pointers;
    std::vector<int64_t> arg_sizes;
}general_task_t;


// ================================================================================
// Main function
// ================================================================================
int main (int argc, char *argv[])
{
    // init mpi with mpi_init_thread
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        printf("Didn't receive appropriate MPI threading specification\n");
        exit(EXIT_FAILURE);
    }

    // variables for tracking mpi-processes
    int comm_size, my_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // variables for configuring the hcl-modules
    int ranks_per_server = comm_size;   // default is total-num of ranks
    bool debug = true;
    bool server_on_node = false;

    // get hostname of each rank
    int name_len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &name_len);
    if (debug) {
        printf("[DBG] R%d: on %s | pid=%d\n", my_rank, processor_name, getpid());
    }

    // check num of ready ranks
    if(debug && my_rank == 0){
        printf("[DBG] %d ranks ready for HCL_CONF attaching\n", comm_size);
        fflush(stdout);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // choose the server, for simple, assign R0 as the server,
    // or make the last as the server by e.g., ((my_rank+1) % ranks_per_server) == 0;
    bool is_server = false;
    if (my_rank == 0)
        is_server = true;

    size_t my_server = my_rank / ranks_per_server;
    int num_servers = comm_size / ranks_per_server;

    // write the server address into file
    if (is_server){
        std::ofstream server_list_file;
        server_list_file.open("./server_list");
        server_list_file << processor_name;
        server_list_file.close();
    }
    std::cout << "[CHECK] R" << my_rank << ": is_server=" << is_server
              << ", my_server=" << my_server
              << ", num_servers=" << num_servers
              << std::endl;

    // configure hcl components before running, this configuration for each rank
    HCL_CONF->IS_SERVER = is_server;
    HCL_CONF->MY_SERVER = my_server;
    HCL_CONF->NUM_SERVERS = num_servers;
    HCL_CONF->SERVER_ON_NODE = server_on_node || is_server;
    HCL_CONF->SERVER_LIST_PATH = "./server_list";


    /**
     * Create hcl global queue over mpi ranks
     * This queue contains the elements with the type is mat_task/general_task_t
     */
    hcl::queue<arr_mat_task> *mat_tasks_queue;

    // allocate the queue at server-side
    if (is_server) {
        mat_tasks_queue = new hcl::queue<arr_mat_task>();
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // allocate the queue at client-side
    if (!is_server) {
        mat_tasks_queue = new hcl::queue<arr_mat_task>();
    }

    // declare a std-queue/rank at the local side for comparison
    std::queue<arr_mat_task> local_queue = std::queue<arr_mat_task>();

    // split the mpi communicator from the server, here is just for client communicator
    MPI_Comm client_comm;
    MPI_Comm_split(MPI_COMM_WORLD, !is_server, my_rank, &client_comm);
    int client_comm_size;
    MPI_Comm_size(client_comm, &client_comm_size);
    MPI_Barrier(MPI_COMM_WORLD);

    /* /////////////////////////////////////////////////////////////////////////////
     * Test throughput of the LOCAL QUEUES at client-side
     * /////////////////////////////////////////////////////////////////////////////  
     */
    int num_tasks = 10;
    int mat_size = 10;
    if (!is_server) {
        // for pushing local
        Timer t_push_local = Timer();
        for(int i = 0; i < num_tasks; i++){
            // allocate an arr_mat task
            struct arr_mat_task T(mat_size);
            
            // put T into the queue and record eslapsed-time
            t_push_local.resumeTime();
            local_queue.push(task);
            t_push_local.pauseTime();

            std::cout << "[CHECK] R" << my_rank << ": size of each task T = " << sizeof(T) << " bytes" << std::endl; 
        }

        // estimate the throughput of pushing local
        // double throughput_pushing_local = (num_tasks*size_of_task*1000) / (timer_loca_queue_put.getElapsedTime()*1024*1024);

        // Timer timer_loca_queue_get = Timer();
        // for(int i = 0; i < num_request; i++){
        //     // measure time of get
        //     timer_loca_queue_get.resumeTime();
        //     auto result = loca_queue.front();
        //     loca_queue.pop();
        //     timer_loca_queue_get.pauseTime();
        // }
        // double throughput_loca_queue_get = (num_request*size_of_task*1000) / (timer_loca_queue_get.getElapsedTime()*1024*1024);

        // print the throughput-results
        // if (my_rank == 0) {
        //     printf("throughput_loca_queue put: %f (MB/s)\n", throughput_loca_queue_put);
        //     printf("throughput_loca_queue get: %f (MB/s)\n",throughput_loca_queue_get);
        // }

    } else {
        std::cout << "R" << my_rank << ": is waiting..." << std::endl;
    }

    // wait for make sure finalizing MPI safe
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    
    exit(EXIT_SUCCESS);
}