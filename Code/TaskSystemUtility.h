//
// Created by Marco on 08/08/18.
//

#ifndef CODE_TASKSYSTEUTILITY_H
#define CODE_TASKSYSTEUTILITY_H

#include <utility>
#include <vector>

namespace TaskSystem{

    /**
     * Equally split the total ammount of work for the number of workers passed as argument
     * @param totWork Total ammount of work
     * @param numWorkers Number of available workers
     * @param out std::vector of pairs of indexes that contain the bounds of the i-th worker
     */
    inline void splitEqually(long totWork, int numWorkers, std::vector<std::pair<long, long>>* out){
        long minWork = totWork / numWorkers;
        int residWork = static_cast<int>(totWork - numWorkers * minWork);

        long minWorkP1 = minWork + 1;

        if(out->size() != numWorkers)
            out->reserve(numWorkers);

        long sum = 0;
        for (int i = 0; i < residWork; ++i) {
            unsigned long oldSum = sum;
            sum += minWorkP1;

            (*out)[i] = std::pair<long, long>(oldSum, sum - 1);
        }


        for (int j = residWork; j < numWorkers; ++j) {
            long oldSum = sum;

            sum += minWork;

            (*out)[j] = std::pair<long, long>(oldSum, sum - 1);
        }
    }
}

#endif //CODE_TASKSYSTEUTILITY_H
