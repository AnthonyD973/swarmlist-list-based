#include <argos3/core/simulator/loop_functions.h>
#include <argos3/core/utility/datatypes/datatypes.h>
#include <fstream>
#include <string>
#include <vector>

#include "KilobotProcess.h"

#ifndef EXP_LOOP_FUNC_H
#define EXP_LOOP_FUNC_H

namespace swlexp {

    class ExpLoopFunc : public argos::CLoopFunctions {

    // ==============================
    // =          METHODS           =
    // ==============================

    public:

        ExpLoopFunc();
        virtual ~ExpLoopFunc();

        virtual void Init(argos::TConfigurationNode& t_tree);
        virtual void Destroy();
        virtual void Reset();

        virtual void PostStep();
        virtual bool IsExperimentFinished();

    private:

        /**
         * Places a certain number of robots in a line.
         * @param[in] numRobots The number of robots to place.
         */
        void _placeLine(argos::UInt32 numRobots);

        /**
         * Updates the number of messages sent by all kilobots using the shared memory.
         */
        void _checkNumMessages();

        /**
         * Finishes the experiment, in effect writing experiment data to the log
         * file.
         */
        void _finishExperiment();

    // ==============================
    // =         ATTRIBUTES         =
    // ==============================

    private:

        /**
         * Experiment param. Specifies the probablility that a
         * message that should have been received is dropped.
         */
        argos::Real m_msgDropProb;

        /**
         * Shared memory maps between the ARGoS and the kilobot processes.
         * This allows ARGoS to gather data about the kilobots' state, for example
         * to know when the experiment is finished.
         */
        std::vector<swlexp::KilobotProcess> m_kilobotProcesses;

        /**
         * Log file that will contain data about the experiment.
         */
        std::ofstream m_expLog;

        /**
         * Number of packets sent by all kilobots since the beginning
         * of the experiment.
         */
        argos::UInt64 m_numMsgsSent = 0;

    };

}

#endif // !EXP_LOOP_FUNC_H