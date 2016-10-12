/*********************************
 Ant Colony Optimization algorithms (AS, ACS, EAS, RAS, MMAS) for CVRP
 
 Created by 孙晓奇 on 2016/10/8.
 Copyright © 2016年 xiaoqi.sxq. All rights reserved.
 
 Program's name: acovrp
 Purpose: vrp problem model, problem init/exit, check
 
 email: sunxq1991@gmail.com
 
 *********************************/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include "problem.h"
#include "io.h"
#include "utilities.h"
#include "vrpHelper.h"
#include "timer.h"


long int seed;

double   g_max_runtime;             /* maximal allowed run time */
double   g_best_so_far_time;        /* 当前最优解出现的时间 */
long int g_best_solution_iter;      /* iteration in which best solution is found */

/**** 用于parallel aco参数 ***/
long int g_master_problem_iteration_num;   /* 每次外循环，主问题蚁群的迭代的次数 */
long int g_sub_problem_iteration_num;      /* 每次外循环，子问题蚁群的迭代的次数 */
long int g_num_sub_problems;               /* 拆分子问题个数 */

double rho;           /* parameter for evaporation */
double alpha;         /* importance of trail */
double beta;          /* importance of heuristic evaluate */
long int ras_ranks;   /* additional parameter for rank-based version of ant system */

/* ------------------------------------------------------------------------ */

long int report_flag;                    /* 结果是否打印输出 */

void set_default_parameters (Problem *instance);
void allocate_ants (Problem *instance);


/*
 * 初始化主问题
 */
Problem * init_master_problem(const char *filename)
{
    Problem *instance = (Problem *)malloc(sizeof(Problem));
    instance->nodeptr = read_instance_file(instance, filename);
    
    set_default_parameters(instance);
    
    // 为 problem 实例的成员分配内存
    instance->distance = compute_distances(instance);
    instance->nn_list = compute_nn_lists(instance);
    instance->pheromone = generate_double_matrix(instance->num_node, instance->num_node);
    instance->total_info = generate_double_matrix(instance->num_node, instance->num_node );
    instance->demand_meet_node_map = (long int *)calloc(instance->num_node, sizeof(long int));
    allocate_ants(instance);
    
    init_report(instance, report_flag);
    
    return instance;
}


/*
 * 主问题结束
 */
void exit_master_problem(Problem *instance)
{
    exit_report(instance, report_flag);
    
    // 释放内存
    free( instance->distance );
    free(instance->nodeptr);
    free( instance->nn_list );
    free( instance->pheromone );
    free( instance->total_info );
    for (int i = 0 ; i < instance->n_ants ; i++ ) {
        free( instance->ants[i].tour );
        free( instance->ants[i].visited );
    }
    free( instance->ants );
    free( instance->best_so_far_ant->tour );
    free( instance->best_so_far_ant->visited );
    free( instance->prob_of_selection );
    free(instance->demand_meet_node_map);
    free(instance);
}


/*
 * 初始化子问题
 */
void init_sub_problem(Problem *instance)

{
}


/*
 * 结束子问题
 */
void exit_sub_problem(Problem *instance)
{
    
}


/*
 FUNCTION:       allocate the memory for the ant colony, the best-so-far and
 the iteration best ant
 INPUT:          none
 OUTPUT:         none
 (SIDE)EFFECTS:  allocation of memory for the ant colony and two ants that
 store intermediate tours
 
 */
void allocate_ants (Problem *instance)
{
    long int i;
    AntStruct *ants, *best_so_far_ant;
    double   *prob_of_selection;
    
    if((ants = (AntStruct *)malloc(sizeof( AntStruct ) * instance->n_ants +
                                  sizeof(AntStruct *) * instance->n_ants)) == NULL){
        printf("Out of memory, exit.");
        exit(1);
    }
    for (i = 0 ; i < instance->n_ants ; i++) {
        ants[i].tour        = (long int *)calloc(2*instance->num_node-1, sizeof(long int));   // tour最长为2 * num_node - 1
        ants[i].visited     = (char *)calloc(instance->num_node, sizeof(char));
    }
    
    if((best_so_far_ant = (AntStruct *)malloc(sizeof(AntStruct))) == NULL){
        printf("Out of memory, exit.");
        exit(1);
    }
    best_so_far_ant->tour        = (long int *)calloc(2*instance->num_node-1, sizeof(long int));
    best_so_far_ant->visited     = (char *)calloc(instance->num_node, sizeof(char));
    
    if ((prob_of_selection = (double *)malloc(sizeof(double) * (instance->nn_ants + 1))) == NULL) {
        printf("Out of memory, exit.");
        exit(1);
    }
    /* Ensures that we do not run over the last element in the random wheel.  */
    prob_of_selection[instance->nn_ants] = HUGE_VAL;
    
    instance->ants = ants;
    instance->best_so_far_ant = best_so_far_ant;
    instance->prob_of_selection = prob_of_selection;
}

/*
 * 参数设置
 */
void set_default_parameters (Problem *instance)
{
    /* number of ants */
    instance->n_ants         = instance->num_node;
    /* number of nearest neighbours in tour construction(neighbor不应该包括depot和自身) */
    instance->nn_ants        = MIN(MAX(instance->n_ants>>2, 25), instance->num_node - 2);
    /* use fixed radius search in the 20 nearest neighbours */
    instance->nn_ls          = MIN(instance->nn_ants, 20);
    
    /* maximum number of iterations */
    instance->max_iteration  = 5000;
    /* optimal tour length if known, otherwise a bound */
    instance->optimum        = 1;
    /* counter of number iterations */
    instance->iteration      = 0;
    
    /* apply local search */
    instance->ls_flag        = 1;
    /* apply don't look bits in local search */
    instance->dlb_flag       = TRUE;
    
    alpha          = 1.0;
    beta           = 2.0;
    rho            = 0.1;
    ras_ranks      = 6;          /* number of ranked ants, top-{ras_ranks} ants */
    
    seed           = (long int) time(NULL);
    g_max_runtime    = 20.0;
    
    report_flag    = 1;
    
    // parallel aco
    //    parallel_flag                     = TRUE;
    g_master_problem_iteration_num    = 1;
    g_sub_problem_iteration_num       = 75;
    g_num_sub_problems                = instance->num_node/50;
}


/*
 * 检查 ant vrp solution 的有效性
 * i.e. tour = [0,1,4,2,0,5,3,0] toute1 = [0,1,4,2,0] route2 = [0,5,3,0]
 */
int check_solution(Problem *instance, const long int *tour, long int tour_size)
{
    int i;
    int * used;
    long int num_node = instance->num_node;
    long int route_beg;    /* 单条回路起点 */
    
    used = (int *)calloc (num_node, sizeof(int));
    
    if (tour == NULL) {
        fprintf (stderr,"\n%s:error: permutation is not initialized!\n", __FUNCTION__);
        exit(1);
    }
    
    route_beg = 0;
    used[0] = TRUE;
    for (i = 1; i < tour_size; i++) {
        
        if (tour[i] != 0) {
            // 非depot点只能路过一次
            if (used[tour[i]]) {
                fprintf(stderr,"\n%s:error: solution vector has two times the value %ld (last position: %d)\n", __FUNCTION__, tour[i], i);
                goto error;
            } else {
                used[tour[i]] = TRUE;
            }
        }
        
        if (tour[i] == 0) {
            // 形成单条回路
            if(!check_route(instance, tour, route_beg, i)) {
                goto error;
            }
            route_beg = i;
        }
    }
    
    for (i = 0; i < num_node; i++) {
        if (!used[i]) {
            fprintf(stderr,"\n%s:error: vector position %d not occupied\n", __FUNCTION__, i);
            goto error;
        }
    }
    
    free (used);
    return TRUE;
    
error:
    fprintf(stderr,"\n%s:error: solution_vector:", __FUNCTION__);
    for (i = 0; i < tour_size; i++)
        fprintf(stderr, " %ld", tour[i]);
    fprintf(stderr,"\n");
    free(used);
    return FALSE;
}

/*
 * 检查单条回路有效性
 * 注意depot点出现两次（分别出现在首尾）
 * i.e. toute = [0,1,4,2,0]
 */
int check_route(Problem *instance, const long int *tour, long int rbeg, long int rend)
{
    long int load = 0;
    
    if (tour[rbeg] != 0 || tour[rend] != 0) {
        fprintf(stderr,"\n%s:error: 车辆路径没有形成一条回路\n", __FUNCTION__);
        return FALSE;
    }
    if (rend - rbeg < 2) {
        fprintf(stderr,"\n%s:error: 单条回路长度不对. rbeg=%ld, rend=%ld\n", __FUNCTION__, rbeg, rend);
        return FALSE;
    }
    for (long int i = rbeg + 1; i < rend - 1; i++) {
        load += instance->nodeptr[tour[i]].demand;
    }
    if (load > instance->vehicle_capacity) {
        fprintf(stderr,"\n%s:error: 单条回路超过车辆最大承载量 load = %ld, capacity = %ld rbeg = %ld rend = %ld\n",
                __FUNCTION__, load, instance->vehicle_capacity, rbeg, rend);
        return FALSE;
    }
    return TRUE;
}
