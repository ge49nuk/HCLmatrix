#include <mpi.h>
#include <omp.h>

#include <hcl/common/data_structures.h>
#include <hcl/queue/queue.h>
#include <hcl/unordered_map/unordered_map.h>

#ifndef PARALLEL_OMP
#define PARALLEL_OMP 0
#endif

#ifndef TRACE
#define TRACE 0
#endif

#if TRACE == 1

#include "VT.h"
static int _tracing_enabled = 1;

#ifndef VT_BEGIN_CONSTRAINED
#define VT_BEGIN_CONSTRAINED(event_id) \
    if (_tracing_enabled)              \
        VT_begin(event_id);
#endif

#ifndef VT_END_W_CONSTRAINED
#define VT_END_W_CONSTRAINED(event_id) \
    if (_tracing_enabled)              \
        VT_end(event_id);
#endif

#endif

#include "mxm_task_types.h"
// #include "mxm_result_type.h"
#include "mxm_kernel.h"
#include "util.h"

using namespace std;
#define HLINE "-------------------------------------------------------------"

static inline double curtime(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

void multiply(double *matrix, double *matrix2, double *result);
bool steal(uint16_t my_server_key);

// ================================================================================
// Global Variables
// ================================================================================
int ranks_per_node = 2;
int num_nodes, ranks_per_server, server_comm_size, comm_size, my_rank, my_server;
double startTime, throughputTime;
int tasksStolen = 0;
bool throttled = false;
hcl::queue<MatTask_Type> *task_queue;
hcl::unordered_map<int, MatResult_Type> *result_map;

double execute(int argc, char *argv[], bool runLocal)
{
    tasksStolen = 0;
    throughputTime = 0;
    bool debug = false;

    // get hostname of each rank
    int name_len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &name_len);
    if (debug)
    {
        printf("[DBG] R%d: on %s | pid=%d\n", my_rank, processor_name, getpid());
    }

    // check num of ready ranks
    if (debug && my_rank == comm_size - 1)
    {
        printf("[Usage] %d ranks ready for HCL_CONF attaching\n", comm_size);
        printf("[Usage] ./main <ranks_per_server> <server_on_node> <num_tasks> <num_threads> <server_list/hostfile>\n");
        fflush(stdout);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    /* /////////////////////////////////////////////////////////////////////////////
     * Configuring HCL-setup
     * ////////////////////////////////////////////////////////////////////////// */

    // get config-values from keyboard
    int num_tasks = 1000;
    bool server_on_node = false;
    int num_omp_threads = 1;
    ranks_per_server = comm_size;
    num_nodes = comm_size / ranks_per_node;
    std::string server_lists = "./server_list";

    if (argc > 1)
        num_tasks = atoi(argv[1]);

    if (my_rank > 1)
        num_tasks = 0;

    if (my_rank < comm_size / 2 || my_rank == comm_size - 1)
        server_on_node = true;

    // if (!server_on_node)
    //     throttled = true;

    bool is_server = (my_rank) % ranks_per_server == 0;

    my_server = my_rank / ranks_per_server;

    // ser num of servers for each rank
    int num_servers = comm_size / ranks_per_server;

    MPI_Barrier(MPI_COMM_WORLD);

    // configure hcl components before running, this configuration for each rank
    auto mem_size = sizeof(MatTask_Type) * num_tasks + sizeof(MatResult_Type) * num_tasks;
    HCL_CONF->IS_SERVER = is_server;
    HCL_CONF->MY_SERVER = my_server;
    HCL_CONF->NUM_SERVERS = num_servers;
    HCL_CONF->SERVER_ON_NODE = server_on_node || is_server;
    HCL_CONF->SERVER_LIST_PATH = "./server_list";
    HCL_CONF->MEMORY_ALLOCATED = mem_size;

    if (debug)
    {
        if (my_rank == comm_size - 1)
        {
            printf("[HCL_CONF] Rank | is_server | my_server | num_servers | server_on_node | mem_alloc | CPU\n");
        }
        printf("R%d %d %d %d %d %lu %d\n", my_rank, HCL_CONF->IS_SERVER, HCL_CONF->MY_SERVER, HCL_CONF->NUM_SERVERS,
               HCL_CONF->SERVER_ON_NODE, HCL_CONF->MEMORY_ALLOCATED, sched_getcpu());
        std::cout << HLINE << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* /////////////////////////////////////////////////////////////////////////////
     * Creating HCL global queues over mpi ranks
     * ////////////////////////////////////////////////////////////////////////// */

    // allocate the hcl queue at server-side
    if (is_server)
    {
        task_queue = new hcl::queue<MatTask_Type>();
        result_map = new hcl::unordered_map<int, MatResult_Type>();
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // sounds like the server queue need to be created before the client ones
    if (!is_server)
    {
        task_queue = new hcl::queue<MatTask_Type>();
        result_map = new hcl::unordered_map<int, MatResult_Type>();
    }

    /* /////////////////////////////////////////////////////////////////////////////
     * Split the mpi communicator from the server, just for client communicator
     * ////////////////////////////////////////////////////////////////////////// */

    MPI_Comm client_comm;
    MPI_Comm_split(MPI_COMM_WORLD, !is_server, my_rank, &client_comm);
    int client_comm_size;
    MPI_Comm_size(client_comm, &client_comm_size);
    MPI_Barrier(MPI_COMM_WORLD);

    /* /////////////////////////////////////////////////////////////////////////////
     * Main loop for creating tasks on each compute rank/ client rank
     * ////////////////////////////////////////////////////////////////////////// */

    if (!is_server)
    { /* IF NOT THE SERVER */

        // Set num of omp threads here, default is 1
        const int NTHREADS = num_omp_threads;

        uint16_t my_server_key = (my_server) % num_servers;
        uint16_t my_server_remote_key = (my_server + 1) % num_servers;

        // use the local key to push tasks on each server side
        if (debug)
            std::cout << "[PUSH] R" << my_rank << ", NUM_OMP_THREADS=" << NTHREADS
                      << ", server_key=" << my_server_key << ", remote_server_key=" << my_server_remote_key
                      << ": is creating " << num_tasks << " mxm-tasks..." << std::endl;

#if PARALLEL_OMP == 1
#pragma omp parallel num_threads(NTHREADS)
        {
#pragma omp for
#endif
            for (int i = 0; i < num_tasks; i++)
            {

                size_t val = my_rank;
                auto key = MatTask_Type(i, val);

                task_queue->Push(key, my_server_key);

                // auto res = MatResult_Type(i, val);
                // result_queue->Push(res, my_server_key);
                //             cout << my_rank << " Pushed!\n";
            }
#if PARALLEL_OMP == 1
        }
#endif

        MPI_Barrier(client_comm);

        // pop tasks from the queue and then execute them
        if (debug)
            std::cout << "[POP] R" << my_rank << ", NUM_OMP_THREADS=" << NTHREADS
                      << ", server_key=" << my_server_remote_key
                      << ": is getting " << num_tasks << " mxm-tasks out for executing..." << std::endl;

        startTime = curtime();
        double mulTime = 0;

        if (!runLocal || my_rank < (comm_size - 1) / 2 || my_rank == comm_size - 1)
            while (true)
            {
                for (int i = 0; i < task_queue->Size(my_server_key); i++)
                {

                    MatTask_Type tmp_pop_T;
                    // cout << "Size of remote queue: " << task_queue -> Size(my_server_remote_key) << endl;

                    double startThroughput = curtime();
                    auto pop_result = task_queue->Pop(my_server_key);
                    throughputTime += curtime() - startThroughput;
                    if (pop_result.first)
                    {
                        if (!server_on_node)
                            tasksStolen++;
                        tmp_pop_T = pop_result.second;
                        // printf("Rank %d got task %d\n", my_rank, tmp_pop_T.tid);
                        auto tmp_result_T = MatResult_Type(tmp_pop_T.tid, my_rank);
                        // vector<double> r (SIZE*SIZE);

                        // double mulStart = curtime();
                        multiply(tmp_pop_T.A, tmp_pop_T.B, tmp_result_T.A);
                        // mulTime += curtime()-mulStart;

                        // cout << my_rank << " Trying to push\n";
                        result_map->Put(tmp_pop_T.tid, tmp_result_T);
                        // result_queue->Push(tmp_result_T, my_server_key);
                        // cout << my_rank << " Pushed!\n";
                    }
                }

                if (task_queue->Size(my_server_key) == 0)
                {
                    break;
                }
            }
        // if(my_rank == comm_size-1)
        //     printf("%lf\n", mulTime);

    } /* ENDIF NOT THE SERVER */

    // wait for making sure finalizing MPI safe
    MPI_Barrier(MPI_COMM_WORLD);
    double ExecutionTime = curtime() - startTime;

    uint16_t my_server_key = (my_server) % num_servers;
    auto res = MatResult_Type(tasksStolen, my_rank);
    result_map->Put(my_rank, res);
    MPI_Barrier(MPI_COMM_WORLD);

    if (my_rank == comm_size - 1)
    {
        for (int i = comm_size / 2; i < comm_size - 1; i++)
        {
            auto pop_result = result_map->Get(i);
            if (pop_result.first)
            {
                auto res = pop_result.second;
                tasksStolen += res.tid;
            }
        }
    }

    return ExecutionTime;
}

int main(int argc, char **argv)
{
    // init mpi with mpi_init_thread
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE)
    {
        printf("[ERR] Didn't receive appropriate MPI threading specification\n");
        exit(EXIT_FAILURE);
    }

    // variables for tracking mpi-processes
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    int interNode = 1, iterations = 1;
    if (argc > 2)
        interNode = atoi(argv[2]);
    if (argc > 3)
        iterations = atoi(argv[3]);

    if (my_rank == comm_size - 1)
        printf("SETUP: %d ranks, %d tasks, %d matrixSize, %d iterations\n", comm_size, atoi(argv[1]), SIZE, iterations);

    double interTime = 0, intraTime = 0, migrationTime = 0;
    int migratedTasks = 0, stolenTasks = 0;

    for (int i = 0; i < iterations; i++)
    {
        if (interNode > 0)
        {
            interTime += execute(argc, argv, false);
            migratedTasks += tasksStolen;
            migrationTime += throughputTime;
            stolenTasks += tasksStolen;
        }

        if (interNode == 0 || interNode == 2)
        {
            intraTime += execute(argc, argv, true);
        }
    }
    interTime /= iterations;
    intraTime /= iterations;
    migratedTasks /= iterations;
    migrationTime /= iterations;
    stolenTasks /= iterations;

    if (my_rank == comm_size - 1)
    {
        if (interNode == 0 || interNode == 2)
            printf("Local Execution Time: %lf\n", intraTime);
        if (interNode > 0)
            printf("Inter-Node Execution Time: %lf and %d tasks stolen\n", interTime, migratedTasks);
        if (interNode == 2)
            printf("Speedup: %lf\n", intraTime / interTime);
    }
    if (my_rank == 1)
    {
        double Mbytes = (sizeof(MatTask_Type) * stolenTasks) / 1000000;
        if (stolenTasks > 0)
            printf("Bandwidth in MB/s: %lf\n", Mbytes / migrationTime);
    }
    MPI_Finalize();
}

void multiply(double *matrix, double *matrix2, double *result)
{
    // for (int i = 0; i < SIZE * SIZE; i += 1)
    // {
    //     double value = 0;
    //     int k = i % SIZE;
    //     for (int j = (i / SIZE) * SIZE; j < (i / SIZE) * SIZE + SIZE; j++)
    //     {
    //         value = value + matrix[j] * matrix2[k];
    //         k += SIZE;
    //     }
    //     result[i] = value;
    // }
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            result[i * SIZE + j] = 0;
            for (int k = 0; k < SIZE; k++)
            {
                result[i * SIZE + j] += matrix[i * SIZE + k] * matrix2[k * SIZE + j];
            }
        }
    }

    if (throttled)
        for (int i = 0; i < SIZE / 2; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                result[i * SIZE + j] = 0;
                for (int k = 0; k < SIZE; k++)
                {
                    result[i * SIZE + j] += matrix[i * SIZE + k] * matrix2[k * SIZE + j];
                }
            }
        }
}