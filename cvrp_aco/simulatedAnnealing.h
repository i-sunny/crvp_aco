//
//  simulatedAnnealing.h
//  cvrp_aco
//
//  Created by 孙晓奇 on 2016/10/19.
//  Copyright © 2016年 xiaoqi.sxq. All rights reserved.
//

#ifndef simulatedAnnealing_h
#define simulatedAnnealing_h

#include <stdio.h>
#include "problem.h"
#include "neighbourSearch.h"

class SimulatedAnnealing {
public:
    SimulatedAnnealing(Problem *instance, double t0, double alpha, long int epoch_length, long int terminal_ratio);
    ~SimulatedAnnealing();
    void run(void);
    bool step(void);
    
private:
    Problem *instance;
    AntStruct *ant;
    NeighbourSearch *neighbour_search;
    LocalSearch *local_search;
    double alpha;
    double t0;
    double t;
    long int ticks;
    long int epoch_length;
    long int epoch_counter;
    long int terminal_ratio;
    long int best_length;      /* 当前最优解长度 */
    // 用于统计
    long int test_cnt;
    long int improvement_cnt;
    long int accept_cnt;
    
    bool acceptable(Move *move);
    void accept(Move *move);
    void reject(Move *move);
};

#endif /* simulatedAnnealing_h */