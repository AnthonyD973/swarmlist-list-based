/**
 * @file swarmlist.h
 * @brief Definition of the Swarmlist class.
 */

#ifndef SWARMLIST_H
#define SWARMLIST_H

#include <stdexcept> // std::domain_error
#include <unordered_map>
#include <string>

#include "include.h"
#include "Messenger.h"

namespace swlexp {

    /**
     * The data that we know about other robots.
     */
    class Swarmlist {

    protected:

        friend class SwarmMsgCallback;

    public:
        class Entry;
        friend void writeInPacket(argos::CByteArray& packet,
                                  const Swarmlist::Entry& entry,
                                  argos::UInt16 idx);

    // ==============================
    // =       NESTED SYMBOLS       =
    // ==============================

    public:

        /**
         * Entry of the Swarmlist.
         */
        class Entry {

        public:

            Entry(RobotId robot, argos::UInt8 swarmMask, Lamport32 lamport);

            inline
            RobotId getRobotId() const { return m_robot; }
            inline
            argos::UInt8 getSwarmMask() const { return m_swarmMask; }
            inline
            Lamport32 getLamport() const { return m_lamport; }
            inline
            argos::UInt32 getTimeToInactive() const { return m_timeToInactive; }

            /**
             * Determines whether the entry is active.
             * The entry of the current robot is always active.
             * @param[in] id ID of the current robot.
             * @return Whether the entry is active.
             */
            inline
            bool isActive(RobotId id) const { return m_timeToInactive != 0 || m_robot == id; }

            /**
             * Removes 1 from the timer.
             */
            void tick() { --m_timeToInactive; }

            /**
             * Resets the entry's timer.
             */
            void resetTimer() { m_timeToInactive = c_ticksToInactive; }

            /**
             * Sets the entry's swarm mask.
             */
            inline
            void setSwarmMask(argos::UInt8 swarmMask) { m_swarmMask = swarmMask; }

            /**
             * Increments the entry's lamport clock.
             */
            void incrementLamport() { ++m_lamport; }

        public:
            /**
             * Sets after how many ticks without any update we consider the entry
             * to be inactive.
             */
            inline static
            void setTicksToInactive(argos::UInt32 ticksToInactive) { c_ticksToInactive = ticksToInactive; }

        private:
            RobotId m_robot;                ///< Robot ID this entry is for.
            argos::UInt8 m_swarmMask;       ///< Data that we wish to share.
            Lamport32 m_lamport;            ///< Time at which the entry was last updated.
            argos::UInt32 m_timeToInactive; ///< Number of swarmlist ticks until we consider this robot to be inactive.
        
        private:
            static argos::UInt32 c_ticksToInactive; ///< The number of ticks until we consider and entry to be inactive.
        };

        /**
         * Callback class to handle swarm messages.
         */
        class SwarmMsgCallback : public Messenger::Callback {
        public:
            SwarmMsgCallback(Swarmlist* swarmlist) : m_swarmlist(swarmlist) { }
            
            /**
             * Deals with an incoming swarm message.
             */
            virtual
            void operator()(const argos::CCI_RangeAndBearingSensor::SPacket& packet);

        private:
            Swarmlist* m_swarmlist;
        };

    // ==============================
    // =          METHODS           =
    // ==============================

    public:

        Swarmlist(Messenger* msn);

        ~Swarmlist();

        // Init, control step, reset

        /**
         * Initializes the swarmlist.
         * @param[in] id The ID of the swarmlist's owner.
         */
        void init(RobotId id);

        /**
         * Function that should be called exactly once every timestep.
         */
        void controlStep();

        /**
         * Resets the swarmlist.
         */
        void reset();

        /**
         * @brief Places the swarmlist in a consensus state.
         * The timer of each entry is reset. This also takes a random entry as
         * the next entry to send.
         * @details This is used when we want to see how long it
         * would take for a new robot's data to be propagated
         * through an existing swarm.
         * @param[in] existingRobots A vector of all existing robots.
         */
        void forceConsensus(const std::vector<RobotId>& existingRobots);

        void setSwarmMask(argos::UInt8 swarmMask);

        /**
         * Determines the total number of entries, be they active or inactive.
         * @return The total number of entries.
         */
        inline
        argos::UInt32 getSize() const { return m_data.size(); }

        /**
         * Determines the total number of active entries.
         * @return The number of active entries.
         */
        inline
        argos::UInt32 getNumActive() const { return m_numActive; }

        /**
         * Gets the number of swarm messages sent by the swarmlist
         * since the beginning of the experiment.
         */
        inline
        argos::UInt64 getNumMsgsTx() const { return m_numMsgsTx; }

        /**
         * Gets the number of swarm messages received by the swarmlist
         * since the beginning of the experiment.
         * @return The number of messages received by the footbots of this
         * controller since the beginning of the experiment.
         */
        inline
        argos::UInt64 getNumMsgsRx() const { return m_numMsgsRx; }

        /**
         * Gets the highest value of the Ticks To Inactive of entries of
         * this robot before its update.
         */
        inline
        argos::UInt32 getHighestTti() const { return m_highestTti; }

        /**
         * Determines the Ticks To Inactive that would be required on
         * an average update.
         */
        inline
        argos::Real getAverageTti() const { return (argos::Real)m_ttiSum / m_numUpdates; }

        /**
         * Composes a string consisting of a set of
         * "(ID,lamport since update, time to inactive)"
         * entries.
         * @param[in] elemDelim Delimiter bewteen each entry's element.
         * @param[in] entryDelim Delimiter bewteen each entry.
         * @return The serialized data.
         */
        std::string serializeData(char elemDelim, char entryDelim) const;

    private:

        // Other functions

        /**
         * Gets an entry of the swarmlist given its robot ID.
         * @throw std::out_of_range The ID is not found.
         * @param[in] robot The robot ID whose data to fetch.
         * @return The entry of the swarmlist corresponding to the passed ID.
         */
        const Entry& _get(RobotId robot) const;

        /**
         * Adds/Modifies an entry of the swarmlist.
         */
        void _set(const Entry& entry);


        /**
         * Sets the entry inside the swarmlist, and resets the entry's timer.
         * @param[in] robot The robot ID this entry is for.
         * @param[in] swarmMask The payload data.
         * @param[in] lamport The time at which this entry was created by
         * 'robot'.
         */
        void _update(RobotId robot, argos::UInt8 swarmMask, Lamport32 lamport);

        /**
         * Removes 1 from all timers, and deals with old entries.
         */
        void _tick();

        /**
         * Changes the next entry to send.
         */
        void _next();

        /**
         * Returns a copy of the next entry we will send.
         * @return A copy of the next entry we will send.
         */
        Entry _getNext();

        /**
         * Creates a swarm message.
         * @return The created swarm message.
         */
        argos::CByteArray _makeNextMessage();

        /**
         * Sends a set of swarm messages.
         */
        void _sendSwarmChunk();

    // ==============================
    // =       STATIC METHODS       =
    // ==============================

    public:

        /**
         * Determines the total number of swarmlist entries in the entire swarm.
         */
        inline static
        argos::UInt64 getTotalNumActive() { return c_totalNumActive; }


        /**
         * Determines whether existing entries should become inactive after a while.
         */
        inline static
        bool getEntriesShouldBecomeInactive() { return c_entriesShouldBecomeInactive; }

        /**
         * Sets whether existing entries should become inactive after a while.
         */
        inline static
        void setEntriesShouldBecomeInactive(bool shouldBecomeInactive) { c_entriesShouldBecomeInactive = shouldBecomeInactive; }

    // ==============================
    // =         ATTRIBUTES         =
    // ==============================

    private:

        RobotId m_id;                     ///< ID of the robot whose swarmlist this is.
        std::vector<Entry> m_data;        ///< Index => Entry in O(1)
        std::unordered_map<RobotId, argos::UInt32> m_idToIndex; ///< Robot ID => Index of m_data in O(1)

        argos::UInt32 m_numActive;        ///< Number of active entries.
        argos::UInt32 m_next;             ///< The index of the next entry to send via a swarm chunk.

        argos::UInt64 m_numMsgsTx;        ///< Number of swarm messages transmitted since the beginning of the experiment.
        argos::UInt64 m_numMsgsRx;        ///< Number of swarm messages received since the beginning of the experiment.

        Messenger* m_msn;                 ///< Messenger object.
        SwarmMsgCallback m_swMsgCb;       ///< Callback object.

        argos::UInt32 m_highestTti;       ///< Highest Ticks To Inactive reached by an entry before its update during the experiment.
        argos::UInt64 m_ttiSum;
        argos::UInt32 m_numUpdates;

    // ==============================
    // =       STATIC MEMBERS       =
    // ==============================

    private:

        static bool c_entriesShouldBecomeInactive; ///< Whether existing entrie should become inactive after a while.
        static argos::UInt64 c_totalNumActive;     ///< The sum, over all robots, of the number of active entries.

        static argos::UInt16 c_numEntriesPerSwarmMsg;    ///< The number of data entries we transmit about other robots per packet.
        static const argos::UInt16 c_SWARM_ENTRY_SIZE;   ///< Size of a single swarmlist entry in a message.
        static const argos::UInt8 c_ROBOT_ID_POS;        ///< Offset, inside a swarmlist entry, of the robot's ID.
        static const argos::UInt8 c_SWARM_MASK_POS;      ///< Offset, inside a swarmlist entry, of the swarm mask.
        static const argos::UInt8 c_LAMPORT_POS;         ///< Offset, inside a swarmlist entry, of the lamport clock.

    };

    void writeInPacket(argos::CByteArray& packet,
                       const Swarmlist::Entry& entry,
                       argos::UInt16 idx);

}

#endif // !SWARMLIST_H