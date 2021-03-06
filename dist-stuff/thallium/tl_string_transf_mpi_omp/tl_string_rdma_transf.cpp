#include <cstdio>
#include <cstdlib>
#include <random>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <sys/syscall.h>
#include <atomic>
#include <chrono>

// for mpi, omp
#include <omp.h>
#include <mpi.h>

// for thallium
#include <thallium.hpp>
#include <thallium/serialization/stl/string.hpp>
#include <bits/stdc++.h>

// for boost-serialize data
#include <boost/serialization/string.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

// declare some namespaces
namespace tl = thallium;

// ================================================================================
// Global variables
// ================================================================================
const int MAX = 26;
const int NUM_DEFAULT_CHAR = 1024*1024;
MPI_Request gath_request;
double iallgather_time_buffer[2];

// ================================================================================
// Util-functions
// ================================================================================
std::string generate_random_string(int n) {
    char alphabet[MAX] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                          'h', 'i', 'j', 'k', 'l', 'm', 'n', 
                          'o', 'p', 'q', 'r', 's', 't', 'u',
                          'v', 'w', 'x', 'y', 'z' };
  
    std::string res = "";
    for (int i = 0; i < n; i++) 
        res = res + alphabet[rand() % MAX];
      
    return res;
}


// ================================================================================
// Main function
// ================================================================================
int main(int argc, char **argv){

    // variables for tracking mpi-processes
    int my_rank;
    int num_ranks;
    int provided;   // level of provided thread support
    int requested = MPI_THREAD_MULTIPLE;    // level of desired thread support

    // init MPI at runtime
    MPI_Init_thread(&argc, &argv, requested, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    /*
     * **************************************************************************** 
     * Init thallium rpc-rdma data transfer
     * ****************************************************************************
     */
    
    // if rank = 0, init thallium server
    if (my_rank == 0){
        // check the server
        int name_len;
        char hostname[MPI_MAX_PROCESSOR_NAME];
        MPI_Get_processor_name(hostname, &name_len);
        std::string server_addr_str(hostname);
        std::cout << "[R0] is initializing the tl-server at " << server_addr_str << std::endl;

        // init the tl-server mode
        tl::engine ser_engine("verbs", THALLIUM_SERVER_MODE);
        std::cout << "[R0] inits tl-server at " << ser_engine.self() << std::endl;
        std::string str_serveraddr = ser_engine.self();
        std::cout << "[R0] casts the addr to string-type: " << str_serveraddr << std::endl;

        // define rpc-function at the server side
        std::function<void(const tl::request&, tl::bulk&)> f =
        [&ser_engine](const tl::request& req, tl::bulk& b) {
            // get the client???s endpoint (client-addr)
            tl::endpoint ep = req.get_endpoint();

            // create a buffer of size 6. We initialize segments
            // and expose the buffer to get a bulk object from it.
            int n_char = NUM_DEFAULT_CHAR;
            std::vector<char> v(n_char);
            std::vector<std::pair<void*, std::size_t>> segments(1);
            segments[0].first  = (void*)(&v[0]);
            segments[0].second = v.size();
            tl::bulk local = ser_engine.expose(segments, tl::bulk_mode::write_only);

            // The call to the >> operator pulls data from the remote
            // bulk object b and the local bulk object. 
            b.on(ep) >> local;

            // Measure the recv-time here
            double recv_time = omp_get_wtime();
            double mpi_recv_time = MPI_Wtime();

            std::cout << "[R0] SERVER received bulk (10 first-characters): ";
            for (int i = 0; i < 10; i++)
                std::cout << v[i];
            std::cout << "..." << std::endl;

            // Since the local bulk is smaller (6 bytes) than the remote
            // one (9 bytes), only 6 bytes are pulled. Hence the loop will
            // print Matthi. It is worth noting that an endpoint is needed
            // for Thallium to know in which process to find the memory
            // we are pulling. That???s what bulk::on(endpoint) does.

            // Gather the measured time at server-side
            MPI_Iallgather(&mpi_recv_time, 1, MPI_DOUBLE, iallgather_time_buffer, 1, MPI_DOUBLE, MPI_COMM_WORLD, &gath_request);

            // Wait for gathering to complete before printing the values received
            MPI_Wait(&gath_request, MPI_STATUS_IGNORE);

            // For summarizing the benchmark
            double bw_server = v.size() / (iallgather_time_buffer[0]-iallgather_time_buffer[1]);
            std::cout << "[R0] recv_time: " << recv_time << " | " << mpi_recv_time << std::endl;
            std::cout << "[R1] Elapsed-time: " << iallgather_time_buffer[0] << " - " << iallgather_time_buffer[1] << " = "
                      << (iallgather_time_buffer[0]-iallgather_time_buffer[1]) << " | " << bw_server << "(bytes/s)" << std::endl;

            // finalize the server after the first transfer
            ser_engine.finalize();
        };

        // define the procedure
        ser_engine.define("do_rdma", f).disable_response();

        // use mpi_send to let the client know the server address
        int reciever = 1; // rank 1, send_tag = 0
        MPI_Send(str_serveraddr.c_str(), str_serveraddr.length(), MPI_CHAR, reciever, 0, MPI_COMM_WORLD);


    } else if (my_rank == 1) {
        // check the client
        std::cout << "[R1] is initializing the tl-client..." << std::endl;

        // use mpi probe to check the message-size from rank 0
        MPI_Status status;
        int sender = 0; // rank 0, send_tag = 0
        MPI_Probe(sender, 0, MPI_COMM_WORLD, &status);
        int mess_size;
        MPI_Get_count(&status, MPI_CHAR, &mess_size);

        // get the ser-addr over mpi-transfer
        char *rec_buf = new char[mess_size];
        MPI_Recv(rec_buf, mess_size, MPI_CHAR, sender, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        std::string ser_addr(rec_buf, mess_size);
        std::cout << "[R1] got the serv-addr: " << ser_addr << std::endl;

        // init the tl-client mode
        tl::engine cli_engine("verbs", MARGO_CLIENT_MODE);
        tl::remote_procedure remote_do_rdma = cli_engine.define("do_rdma").disable_response();
        tl::endpoint ser_endpoint = cli_engine.lookup(ser_addr);

        // we define a buffer with the content ???Matthieu??? (because it???s a string,
        // there is actually a null-terminating character). We then define
        // segments as a vector of pairs of void* and std::size_t
        // std::string buffer = "Matthieu";
        int n_characters = NUM_DEFAULT_CHAR;
        std::string buffer = generate_random_string(n_characters);
        std::vector<std::pair<void*, std::size_t>> segments(1);

        // check mem-allocation of the string
        void *pc;
        for (int i = 0;  i < 10; i++){
            pc = (void*)(buffer.at(i));
            std::cout << "[DBG-alloc-string] CLIENT string[" << i << "]: "
                      << pc << std::endl;
        }

        // Each segment (here only one) is characterized by its starting
        // address in local memory and its size. 
        segments[0].first  = (void*)(&buffer[0]);
        segments[0].second = buffer.size()+1;
        std::cout << "[R1] CLIENT num_characters = " << buffer.size()+1
                << ", sizeof(bufer) = " << sizeof(buffer)
                << std::endl;

        // We call engine::expose to expose the buffer and get a bulk instance from it.
        // We specify tl::bulk_mode::read_only to indicate that the memory will only be
        // read by other processes (alternatives are tl::bulk_mode::read_write
        // and tl::bulk_mode::write_only). 
        tl::bulk myBulk = cli_engine.expose(segments, tl::bulk_mode::read_only);

        // Finally we send an RPC to the server, passing the bulk object as an argument.
        // Measure the send-time at the sender (client)
        double send_time = omp_get_wtime();
        double mpi_send_time = MPI_Wtime();

        // Do the calling action
        remote_do_rdma.on(ser_endpoint)(myBulk);

        // Gather the measured time at client-side
        MPI_Iallgather(&mpi_send_time, 1, MPI_DOUBLE, iallgather_time_buffer, 1, MPI_DOUBLE, MPI_COMM_WORLD, &gath_request);

        // Wait for gathering to complete before printing the values received
        MPI_Wait(&gath_request, MPI_STATUS_IGNORE);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // sleep for 1s
        double bw_client = (buffer.size()+1) / (iallgather_time_buffer[0]-iallgather_time_buffer[1]);
        std::cout << "[R1] send_time: " << send_time << " | " << mpi_send_time << std::endl;
        std::cout << "[R1] Elapsed-time: " << iallgather_time_buffer[0] << " - " << iallgather_time_buffer[1] << " = "
                      << (iallgather_time_buffer[0]-iallgather_time_buffer[1]) << " | " << bw_client << "(bytes/s)" << std::endl;

        // free the memory allocated by new
        delete[] rec_buf; // because having [size] after new

    } else {

        // do nothing, just wait
        std::cout << "R" << my_rank << ": do nothing, just wait..." << std::endl;
    }
    /*
     * ****************************************************************************
     */

    // put mpi barrier here to wait, just make sure every ranks finished
    MPI_Barrier(MPI_COMM_WORLD);

    // finalize mpi
    MPI_Finalize();

    return 0;
}