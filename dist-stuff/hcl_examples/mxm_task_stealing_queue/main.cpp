#include <mpi.h>
#include <omp.h>

#include <hcl/common/data_structures.h>
#include <hcl/queue/queue.h>

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

void multiply(vector<double> matrix, vector<double> matrix2, vector<double> result);
bool steal(uint16_t my_server_key);

// ================================================================================
// Global Variables
// ================================================================================
int ranks_per_node = 2;
int num_nodes;
int ranks_per_server;
int server_comm_size;
int comm_size, my_rank;
int my_server;
double startTime;
hcl::queue<MatTask_Type> *global_queue;
static bool taskStealing = false;

double execute(int argc, char *argv[], bool runLocal)
{

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
    if (debug && my_rank == 0)
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

    bool is_server = (my_rank) % ranks_per_server == 1;

    my_server = my_rank / ranks_per_server;

    // ser num of servers for each rank
    int num_servers = comm_size / ranks_per_server;

    // get IB IP addresses
    // MPI_Comm server_comm;
    // MPI_Comm_split(MPI_COMM_WORLD, is_server, my_rank, &server_comm);
    // MPI_Comm_size(server_comm, &server_comm_size);
    // MPI_Barrier(MPI_COMM_WORLD);
    // if (is_server){
    //     char *IPbuffer;
    //     IPbuffer = getHostIB_IPAddr();
    //     std::string send_addr(IPbuffer);
    //     std::cout << "[DBG] R" << my_rank << ": send_addr=" << send_addr << std::endl;
    //     size_t message_length = send_addr.size();
    //     char recv_buff[message_length*server_comm_size];
    //     MPI_Allgather(send_addr.c_str(), message_length, MPI_CHAR, recv_buff, message_length, MPI_CHAR, server_comm);
    //     if (my_rank == 1){
    //         // write ib-addresses to file
    //         ofstream ser_addr_file;
    //         ser_addr_file.open("./server_list");
    //         for (int i = 0;  i < num_servers; i++){
    //             std::string ib_addr = "";
    //             for (int j = 0; j < message_length; j++)
    //                 ib_addr = ib_addr + recv_buff[i*message_length + j];
    //             std::cout << "[DBG] Server " << i << ": IB-IP=" << ib_addr << std::endl;
    //             ser_addr_file << ib_addr << std::endl;
    //         }
    //         ser_addr_file.close();
    //     }
    // MPI_Barrier(server_comm);
    // }
    MPI_Barrier(MPI_COMM_WORLD);

    // configure hcl components before running, this configuration for each rank
    auto mem_size = SIZE * SIZE * num_tasks;
    HCL_CONF->IS_SERVER = is_server;
    HCL_CONF->MY_SERVER = my_server;
    HCL_CONF->NUM_SERVERS = num_servers;
    HCL_CONF->SERVER_ON_NODE = server_on_node || is_server;
    HCL_CONF->SERVER_LIST_PATH = "./server_list";
    HCL_CONF->MEMORY_ALLOCATED = mem_size;

    if (debug)
    {
        if (my_rank == 0)
        {
            printf("[HCL_CONF] Rank | is_server | my_server | num_servers | server_on_node | mem_alloc\n");
        }
        printf("R%d %d %d %d %d %lu\n", my_rank, HCL_CONF->IS_SERVER, HCL_CONF->MY_SERVER, HCL_CONF->NUM_SERVERS,
               HCL_CONF->SERVER_ON_NODE, HCL_CONF->MEMORY_ALLOCATED);
        std::cout << HLINE << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* /////////////////////////////////////////////////////////////////////////////
     * Creating HCL global queues over mpi ranks
     * ////////////////////////////////////////////////////////////////////////// */

    // allocate the hcl queue at server-side
    if (is_server)
    {
        global_queue = new hcl::queue<MatTask_Type>();
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // sounds like the server queue need to be created before the client ones
    if (!is_server)
    {
        global_queue = new hcl::queue<MatTask_Type>();
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

                global_queue->Push(key, my_server_key);
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

        if (!runLocal || my_rank < comm_size / 2 || my_rank == comm_size - 1)
            while (true)
            {
                bool queueEmpty = false;
#if PARALLEL_OMP == 1
#pragma omp parallel num_threads(NTHREADS)
                {
#pragma omp for
#endif
                    for (int i = 0; i < global_queue->Size(my_server_remote_key); i++)
                    {
                        if (queueEmpty)
                            continue;

                        MatTask_Type tmp_pop_T;
                        // cout << "Size of remote queue: " << global_queue -> Size(my_server_remote_key) << endl;
                        auto pop_result = global_queue->Pop(my_server_remote_key);

                        if (pop_result.first)
                        {
                            // printf("Rank %d got one\n", my_rank);
                            tmp_pop_T = pop_result.second;
                            multiply(tmp_pop_T.A, tmp_pop_T.B, tmp_pop_T.C);
                        }
                        else
                        {
                            queueEmpty = true;
                        }
                    }
#if PARALLEL_OMP == 1
                }
#endif

                if (global_queue->Size(my_server_key) == 0 && (!taskStealing || !steal(my_rank)))
                {
                    break;
                }
            }

    } /* ENDIF NOT THE SERVER */

    // wait for making sure finalizing MPI safe
    MPI_Barrier(MPI_COMM_WORLD);

    double ExecutionTime = curtime() - startTime;
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

    if (my_rank == 0)
        printf("SETUP: %d ranks, %d tasks, %d iterations\n", comm_size, atoi(argv[1]), iterations);

    double interTime = -1, intraTime = -1;

    if (interNode > 0)
    {
        interTime = 0;
        for (int i = 0; i < iterations; i++)
            interTime += execute(argc, argv, false);
        interTime /= iterations;

        if (my_rank == 0)
            printf("Inter Execution Time: %lf\n", interTime);
    }
    if (interNode == 0 || interNode == 2)
    {
        intraTime = 0;
        for (int i = 0; i < iterations; i++)
            intraTime += execute(argc, argv, true);
        intraTime /= iterations;

        if (my_rank == 0)
            printf("Intra Execution Time: %lf\n", intraTime);
    }

    if (my_rank == 0 && interNode == 2)
        printf("Speedup: %lf\n", intraTime / interTime);
    MPI_Finalize();
}

void multiply(vector<double> matrix, vector<double> matrix2, vector<double> result)
{
    for (int i = 0; i < SIZE * SIZE; i += 1)
    {
        double value = 0;
        int k = i % SIZE;
        for (int j = (i / SIZE) * SIZE; j < (i / SIZE) * SIZE + SIZE; j++)
        {
            value = value + matrix[j] * matrix2[k];
            k += SIZE;
        }
        result[i] = value;
    }
}

bool steal(uint16_t my_server_key)
{
    srand(unsigned(std::time(0)));
    vector<uint16_t> ranks;

    for (uint16_t i = 0; i < comm_size; i += comm_size / ranks_per_server)
    {
        if (i != my_rank)
            ranks.push_back(i);
    }

    std::random_shuffle(ranks.begin(), ranks.end());
    //iterates through ranks and tests if they have tasks left
    for (std::vector<uint16_t>::iterator it = ranks.begin(); it != ranks.end(); ++it)
    {

        //measuring the time until the first response from foreign rank
        long size = global_queue->Size(*it);
        cout << "Remote size: " << size << endl;

        if (size > 0)
        {
            //steals half the tasks
            int j = 0;
            while (j < global_queue->Size(*it) * 0.5)
            {
                MatTask_Type tmp_pop_T;
                printf("Trying to steal %d %d\n", my_rank, *it);
                auto popped = global_queue->Pop(*it);
                printf("steal %d %d\n", my_rank, *it);
                if (popped.first)
                {
                    tmp_pop_T = popped.second;
                    global_queue->Push(tmp_pop_T, my_server_key);
                }
                else
                    break;

                j++;
            }

            long ownSize = global_queue->Size(my_server_key);
            if (ownSize > 0)
            {
                printf("[%ld]Successfully stolen %ld/%ld tasks!\n", my_rank, ownSize, size);
                return true;
            }
        }
    }

    return false;
}
