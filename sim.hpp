/*
 * Copyright (c) 2024 David Leeds <davidesleeds@gmail.com>
 *
 * evtol-sim is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#pragma once

#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "utility.hpp"

namespace dl {

using namespace std::chrono_literals;
using json = nlohmann::json;

class sim;

/*
 * Runner class that constructs, owns, and advances a collection of simulators.
 */
class runner {
public:
    using duration = std::chrono::seconds;

    /*
     * Construct a simulation runner.
     * `time_slice` defines the time resolution of the simulators.
     */
    runner(duration time_slice = 1s);

    /*
     * Factory function to construct a simulator and associate it with this runner.
     */
    template <typename SimType, typename ...SimArgs>
    SimType &add(std::string id, SimArgs &&...args)
    {
        static_assert(std::is_base_of_v<sim, SimType>, "SimType must be derived from dl::sim");

        auto sim_ptr = std::make_unique<SimType>(std::forward<SimArgs>(args)...);
        SimType &sim = *sim_ptr;

        sims_.push_back(std::move(sim_ptr));

        /* Complete sim initialization */
        sim.id_ = std::move(id);
        sim.runner_ = this;

        return sim;
    }

    /*
     * Return a std::ranges::view containing a reference to each simulator of the specified type.
     */
    template <typename SimType = sim>
    auto sims() const
    {
        auto type_filter = [](const auto &ptr) {
            return dynamic_cast<SimType *>(ptr.get()) != nullptr;
        };

        auto project_derived = [](const auto &ptr) -> SimType & {
            return dynamic_cast<SimType &>(*ptr);
        };

        return sims_ | std::views::filter(type_filter) | std::views::transform(project_derived);
    }

    /*
     * Run all simulators for the specified duration.
     */
    void run(duration duration);

    /*
     * Return total elapsed simulation time.
     */
    duration elapsed() const;

    /*
     * Capture data about the simulation. Generally, sims will record their own events,
     * but events may also be inserted externally for additional context.
     */
    void record_event(std::string id, std::string event, json data = {});

    /*
     * Return a JSON array containing simulator events.
     */
    const json &events() const;

    /*
     * Apply a user-defined seed to reproduce a pseudo-random number sequence.
     */
    void seed_random_engine(std::default_random_engine::result_type seed);

    /*
     * Return a pseudo-random number generator for use by simulators.
     */
    std::default_random_engine &random_engine();

private:
    duration time_slice_;
    duration elapsed_{};
    std::vector<std::unique_ptr<sim>> sims_;
    json events_ = json::array();
    std::default_random_engine random_engine_;
};


/*
 * Base class for a simulator.
 */
class sim {
public:
    friend runner;
    using duration = std::chrono::seconds;

    virtual ~sim() = default;

    /*
     * Return the simulator's [ideally] unique identifier.
     */
    const std::string &id() const;

    /*
     * Capture data about the simulation.
     */
    void record_event(std::string event, json data = {});

    /*
     * Return the simulation runner's random engine.
     */
    std::default_random_engine &random_engine();

protected:
    /* Only runner::add() may construct a sim */
    sim() = default;

    /*
     * Simulator implementations must implement a step function to advance their
     * state by one time slice at a time. To simplify logic, sims may assume that
     * no intermediate state changes must occur within a single time slice.
     */
    virtual void step(duration time_slice) = 0;

private:
    std::string id_;
    runner *runner_{nullptr};
};

} /* namespace dl */
