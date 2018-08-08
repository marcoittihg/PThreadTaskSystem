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
    inline void splitEqually(unsigned long totWork, unsigned int numWorkers, std::vector<std::pair<unsigned long, unsigned long>>* out){
        unsigned long minWork = totWork / numWorkers;
        unsigned int residWork = static_cast<unsigned int>(totWork - numWorkers * minWork);

        unsigned long minWorkP1 = minWork + 1;

        if(out->size() != numWorkers)
            out->resize(numWorkers);

        unsigned long sum = 0;
        for (unsigned int i = 0; i < residWork; ++i) {
            unsigned long oldSum = sum;
            sum += minWorkP1;

            out->at(i) = std::pair<unsigned long, unsigned long>(oldSum, sum - 1);
        }


        for (unsigned int j = residWork; j < numWorkers; ++j) {
            unsigned long oldSum = sum;

            sum += minWork;

            out->at(j) = std::pair<unsigned long, unsigned long>(oldSum, sum - 1);
        }
    }
}

#endif //CODE_TASKSYSTEUTILITY_H
