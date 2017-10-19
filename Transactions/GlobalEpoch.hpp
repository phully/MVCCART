//
// Created by Players Inc on 11/10/2017.
//

#ifndef MVCCART_GLOBALEPOCH_HPP
#define MVCCART_GLOBALEPOCH_HPP


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
#include "Epochs.hpp"
#define EPOCH_TIME_ELPSE_SLEEP_MS 50


void EpochThread();
///< typedef for a callback function which is invoked when the table was updated
typedef boost::signals2::signal<void ()> ObserverCallback;
typedef ObserverCallback::slot_type const& CallBackObserver;
ObserverCallback mImmediateGC;


class GlobalEpoch
{
      public:
      std::vector<Epoch> EpochsPast;
      Epoch ActiveEpoch;
      boost::thread* mEpochThread;

      GlobalEpoch()
      {
            ActiveEpoch = Epoch();
      }

      void registerGCTrigger(CallBackObserver cb)
      {
            mImmediateGC.connect(cb);
      }

      void StartEpochThread()
      {
            mEpochThread = new boost::thread(&EpochThread);
      }

      Epoch getActiveEpoch()
      {
            return ActiveEpoch;
      }
};

GlobalEpoch myEpochGlobal = GlobalEpoch();

void EpochThread()
{

    while(true)
    {
            ///If no new transaction has been registered
            /// in the recent active_epoch, chance for GC
            if(myEpochGlobal.ActiveEpoch.counter==0)
            {

                ///Trigger GC but check if all Epochs counters are 0
                bool isAllEpochsFlushed = true;
                for(int i=0; i < myEpochGlobal.EpochsPast.size(); i++)
                {
                    if(myEpochGlobal.EpochsPast[i].counter!=0)
                    {
                        isAllEpochsFlushed = false;
                    }
                }
                /// if all previous epoch's counter is 0
                if (isAllEpochsFlushed)
                {
                    mImmediateGC();
                    isAllEpochsFlushed = false;
                    myEpochGlobal.EpochsPast.clear();
                }
            }
                    myEpochGlobal.EpochsPast.push_back(myEpochGlobal.ActiveEpoch);
                    myEpochGlobal.ActiveEpoch = Epoch();


        std::this_thread::sleep_for(std::chrono::milliseconds(EPOCH_TIME_ELPSE_SLEEP_MS));
    }
}

#endif //MVCCART_GLOBALEPOCH_HPP
