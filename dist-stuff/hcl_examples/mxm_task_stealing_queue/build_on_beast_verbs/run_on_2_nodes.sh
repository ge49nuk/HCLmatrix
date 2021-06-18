# Soft-config with server_on_node=true
#    Usage: ./main <ranks_per_server> <server_on_node> <num_threads> <num_tasks>
#    For example: ./main 2 1 1000 8
#    (Note: ranks_per_server = num ranks per node, this case is 2, 4 ranks in total, running on 2 nodes
#           server_list/hostfile, you can create a file, named server_list, the content is
#            10.12.1.1
#            10.12.1.2
#     They are IB-IP-addresses of Rome1 and Rome2. HCL will get them to create RPC-servers and clients for communication)

# Run command
mpirun -n 4 -ppn 2 --host 10.12.1.1,10.12.1.2 ./main 400 2

# (This means that we run 4 ranks in total, 2 ranks/node,
#   1 server/node, and 2 ranks/server,
#   server_on_node=true, num_tasks=1000, num_threads for execution=16)
