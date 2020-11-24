#include "stencil.h"

/* default parameter values */
#ifdef STARPU_QUICK_CHECK
static unsigned niter = 4;
#define SIZE 16
#define NBZ 8
#else
static unsigned niter = 100;
#define SIZE 128	//8
#define NBZ 32	//4
#endif

int who_runs_what_len;
int *who_runs_what;
int *who_runs_what_index;
double *last_tick;
static unsigned bind_tasks = 0;
static unsigned ticks = 1000;

/* Problem size */
static unsigned sizex = SIZE;
static unsigned sizey = SIZE;
static unsigned sizez = NBZ*SIZE;

/* Number of blocks (scattered over the different MPI processes) */
unsigned nbz = NBZ;

/* Get global information */
unsigned get_bind_tasks(void){
	return bind_tasks;
}

unsigned get_nbz(void){
	return nbz;
}

unsigned get_niter(void){
	return niter;
}

unsigned get_ticks(void){
	return ticks;
}

/* Get info workder_id global */
unsigned global_workerid(unsigned local_workerid)
{
#if STARPU_USE_MPI
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	unsigned workers_per_node = starpu_worker_get_count();
	return (local_workerid + rank*workers_per_node);
#else
	return local_workerid;
#endif
}

/* Parsing the arguments */
static void parse_args(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-b") == 0){
			bind_tasks = 1;
		}

		if (strcmp(argv[i], "-nbz") == 0){
			nbz = atoi(argv[++i]);
		}

		if (strcmp(argv[i], "-sizex") == 0){
			sizex = atoi(argv[++i]);
		}

		if (strcmp(argv[i], "-sizey") == 0){
			sizey = atoi(argv[++i]);
		}

		if (strcmp(argv[i], "-sizez") == 0){
			sizez = atoi(argv[++i]);
		}

		if (strcmp(argv[i], "-niter") == 0){
			niter = atoi(argv[++i]);
		}

		if (strcmp(argv[i], "-ticks") == 0){
			ticks = atoi(argv[++i]);
		}

		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
			 fprintf(stderr, "Usage : %s [options...]\n", argv[0]);
			 fprintf(stderr, "\n");
			 fprintf(stderr, "Options:\n");
			 fprintf(stderr, "-b    bind tasks on CPUs/GPUs\n");
			 fprintf(stderr, "-nbz <n>  Number of blocks on Z axis (%u by default)\n", nbz);
			 fprintf(stderr, "-size[xyz] <size>	Domain size on x/y/z axis (%ux%ux%u by default)\n", sizex, sizey, sizez);
			 fprintf(stderr, "-niter <n>    Number of iterations (%u by default)\n", niter);
			 fprintf(stderr, "-ticks <t>	How often to put ticks in the output (ms, %u by default)\n", ticks);
			 exit(0);
		}
	}
}

/* Initialization */
static void init_problem(int argc, char **argv, int rank, int world_size)
{
	// parse the arguments if yes
    printf("\t[init_problem] parsing arguments\n");
    parse_args(argc, argv);

	// create block_arrays
    printf("\t[init_problem] creating block_arrays\n");
	printf("\t size_x=%d, size_y=%d, size_z=%d, num_blocks=%d\n", sizex, sizey, sizez, nbz);
    create_blocks_array(sizex, sizey, sizez, nbz);

	// assign blocks to MPI nodes
	assign_blocks_to_mpi_nodes(world_size);

	// assign blocks to workers
	assign_blocks_to_workers(rank);

	// allocate the different mem blocks, if used by the MPI process
	allocate_memory_on_node(rank);

	// display mem usage
	display_memory_consumption(rank);

	// check sth here???
	who_runs_what_len = 2 * niter;
	who_runs_what = (int *) calloc(nbz * who_runs_what_len, sizeof(*who_runs_what));
	who_runs_what_index = (int *) calloc(nbz, sizeof(*who_runs_what_index));
	last_tick = (double *) calloc(nbz, sizeof(* last_tick));
}


/* free the problem */
static void free_problem(int rank)
{
    free_memory_on_node(rank);
	free_blocks_array();
	free(who_runs_what);
	free(who_runs_what_index);
	free(last_tick);
}

/* f function ??? */
void f(unsigned task_per_worker[STARPU_NMAXWORKERS])
{
	unsigned total = 0;
	int worker;

	for (worker = 0; worker < STARPU_NMAXWORKERS; worker++)
		total += task_per_worker[worker];
	for (worker = 0; worker < STARPU_NMAXWORKERS; worker++)
	{
		if (task_per_worker[worker])
		{
			char name[64];
			starpu_worker_get_name(worker, name, sizeof(name));
			FPRINTF(stderr,"\t%s -> %u (%2.2f%%)\n", name, task_per_worker[worker], (100.0*task_per_worker[worker])/total);
		}
	}
}

/* Main body */
double start;
double begin, end;
double timing; 

int main(int argc, char **argv)
{
    int rank;
    int world_size;
    int ret;

    // STARPU USE MPI
    printf("[CHECK] STARPU_USE_MPI=%d\n", STARPU_USE_MPI);
    #if STARPU_USE_MPI
        int thread_support;
        if (MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &thread_support)){
            FPRINTF(stderr, "MPI_Init_thread failed\n");
        }

        if (thread_support == MPI_THREAD_FUNNELED)
		    FPRINTF(stderr,"Warning: MPI only has funneled thread support, not serialized, hoping this will work\n");

	    if (thread_support < MPI_THREAD_FUNNELED)
		    FPRINTF(stderr,"Warning: MPI does not have thread support!\n");

        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    #else
        rank = 0;
        world_size = 1;
    #endif

    if (rank == 0){
        FPRINTF(stderr, "Running on %d nodes\n", world_size);
		fflush(stderr);
    }

    // init starpu
    printf("1. init StarPU ...\n");
    ret = starpu_init(NULL);
	if (ret == -ENODEV) return 77;
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");

    // init starpu_mpi if we use mpi
    #if STARPU_USE_MPI
    ret = starpu_mpi_init(NULL, NULL, 0);
    STARPU_CHECK_RETURN_VALUE(ret, "starpu_mpi_init");
    #endif

    // init the problem
    printf("2. init the problem ...\n");
    init_problem(argc, argv, rank, world_size);

	// check the assignment of blocks to mpi_nodes
	// unsigned b_idx;
	// for (b_idx = 0; b_idx < nbz; b_idx++){
	// 	struct block_description *block = get_block_description(b_idx);
	// 	printf("\t[main] block %d: rank_%d, lay_hand1_%p, lay_hand2_%p\n", block->bz, block->mpi_node, &block->layers_handle[0], &block->layers_handle[1]);
	// 	printf("\t\t block %d: bound_h[B]_%p, bound_h[T]_%p\n", block->bz, &block->boundary_blocks[B], &block->boundary_blocks[T]);
	// }

	// create tasks
	printf("3. creat tasks ...\n");
	create_tasks(rank);

	// make a barrier to wait for all the tasks created
	#if STARPU_USE_MPI
	int barrier_ret = MPI_Barrier(MPI_COMM_WORLD);
	STARPU_ASSERT(barrier_ret == MPI_SUCCESS);
	#endif

	// measuring time here
	if (rank == 0)
		FPRINTF(stderr, "Go!\n");
	start = starpu_timing_now();
	begin = starpu_timing_now();

	// Explicitly unlock tag id. It may be useful in the case of applications which execute part of
	// their computation outside StarPU tasks (e.g. third-party libraries)
	starpu_tag_notify_from_apps(TAG_INIT_TASK);

	wait_end_tasks(rank);

	end = starpu_timing_now();

	// make a barrier here to wait for all the tasks finished
	#if STARPU_USE_MPI
	barrier_ret = MPI_Barrier(MPI_COMM_WORLD);
	STARPU_ASSERT(barrier_ret == MPI_SUCCESS);
	#endif

	#if 0
	check(rank);
	#endif

	#if STARPU_USE_MPI
	starpu_mpi_shutdown();
	#endif

	/* timing in us */
	timing = end - begin;
	double min_timing = timing;
	double max_timing = timing;
	double sum_timing = timing;

	#if STARPU_USE_MPI
	int reduce_ret;

	reduce_ret = MPI_Reduce(&timing, &min_timing, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
	STARPU_ASSERT(reduce_ret == MPI_SUCCESS);

	reduce_ret = MPI_Reduce(&timing, &max_timing, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	STARPU_ASSERT(reduce_ret == MPI_SUCCESS);

	reduce_ret = MPI_Reduce(&timing, &sum_timing, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	STARPU_ASSERT(reduce_ret == MPI_SUCCESS);

	int *who_runs_what_tmp = malloc(nbz * who_runs_what_len * sizeof(*who_runs_what));
	reduce_ret = MPI_Reduce(who_runs_what, who_runs_what_tmp, nbz * who_runs_what_len, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	STARPU_ASSERT(reduce_ret == MPI_SUCCESS);

	memcpy(who_runs_what, who_runs_what_tmp, nbz * who_runs_what_len * sizeof(*who_runs_what));
	free(who_runs_what_tmp);

	int *who_runs_what_index_tmp = malloc(nbz * sizeof(*who_runs_what_index));
	reduce_ret = MPI_Reduce(who_runs_what_index, who_runs_what_index_tmp, nbz, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	STARPU_ASSERT(reduce_ret == MPI_SUCCESS);

	memcpy(who_runs_what_index, who_runs_what_index_tmp, nbz * sizeof(*who_runs_what_index));
	free(who_runs_what_index_tmp);
	#endif

	if (rank == 0)
	{
		#if 1
		FPRINTF(stderr, "update: \n");
		f(update_per_worker);
		FPRINTF(stderr, "top: \n");
		f(top_per_worker);
		FPRINTF(stderr, "bottom: \n");
		f(bottom_per_worker);
		#endif
		
		#if 1
		unsigned nzblocks_per_process = (nbz + world_size - 1) / world_size;
		int iter;
		for (iter = 0; iter < who_runs_what_len; iter++)
		{
			starpu_iteration_push(iter);
			unsigned last, bz;
			last = 1;
			for (bz = 0; bz < nbz; bz++)
			{
				if ((bz % nzblocks_per_process) == 0)
					FPRINTF(stderr, "| ");

				if (who_runs_what_index[bz] <= iter)
					FPRINTF(stderr,"_ ");
				else
				{
					last = 0;
					if (who_runs_what[bz + iter * nbz] == -1)
						FPRINTF(stderr,"* ");
					else
						FPRINTF(stderr, "%d ", who_runs_what[bz + iter * nbz]);
				}
			}
			FPRINTF(stderr, "\n");
			starpu_iteration_pop();
			if (last)
				break;
		}
		#endif
		
		fflush(stderr);
		FPRINTF(stdout, "Computation took: %f ms on %d MPI processes\n", max_timing/1000, world_size);
		FPRINTF(stdout, "\tMIN : %f ms\n", min_timing/1000);
		FPRINTF(stdout, "\tMAX : %f ms\n", max_timing/1000);
		FPRINTF(stdout, "\tAVG : %f ms\n", sum_timing/(world_size*1000));
	}

	free_problem(rank);
	starpu_shutdown();

	// end MPI
	#if STARPU_USE_MPI
	MPI_Finalize();
	#endif

    return 0;
}