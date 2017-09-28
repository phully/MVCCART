//
// Created by Players Inc on 27/09/2017.
//

#ifndef MVCCART_EPOCHS_HPP
#define MVCCART_EPOCHS_HPP

#include <iostream>
#include <inttypes.h>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/tuple/tuple.hpp>
#include <stdint.h>
#include <stdbool.h>
#include "core/Tuple.hpp"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <assert.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
#include <boost/signals2.hpp>
#include <queue>
#include <atomic>
#define EPOCH_TIME_ELPSE_SLEEP_MS 50


auto hr_now() -> decltype(std::chrono::high_resolution_clock::now())
{
    return std::chrono::high_resolution_clock::now();
}

class Epoch
{
    public:
        std::vector<size_t> TxnSet;
        int counter;
    Epoch()
    {
        TxnSet.clear();
        counter=0;
    }
};

class GlobalEpoch
{

private:
    double EpochLapse;


public:
   static std::queue<Epoch> EpochsPast;
        Epoch ActiveEpoch;

    GlobalEpoch()
    {}

    Epoch getActiveEpoch()
    {
        return ActiveEpoch;
    }

    void startClock()
    {

        while(true)
        {
            if(EpochsPast.size() ==0)
            {
                EpochsPast.emplace(ActiveEpoch);
                ActiveEpoch = Epoch();
            }
            else
            {
                if(ActiveEpoch.counter==0)
                {
                   ///Trigger GC but check if all Epochs counters are 0
                    continue;
                }
                else
                {
                    EpochsPast.emplace(ActiveEpoch);
                    ActiveEpoch = Epoch();
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(EPOCH_TIME_ELPSE_SLEEP_MS));

        }
    }
};

#endif //MVCCART_EPOCHS_HPP
