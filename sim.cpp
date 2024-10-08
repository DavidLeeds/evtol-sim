/*
 * Copyright (c) 2024 David Leeds <davidesleeds@gmail.com>
 *
 * evtol-sim is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <cassert>

#include "sim.hpp"

namespace dl {

runner::runner(duration time_slice) :
    time_slice_(time_slice)
{
    assert(time_slice > 0s);
}

void runner::run(duration duration)
{
    while (duration >= time_slice_) {
        elapsed_ += time_slice_;

        for (auto &sim : sims_) {
            sim->step(time_slice_);
        }

        duration -= time_slice_;
    }
}

runner::duration runner::elapsed() const
{
    return elapsed_;
}

void runner::record_event(std::string id, std::string event, json data)
{
    json entry;

    entry["timestamp"] = elapsed();
    entry["id"] = std::move(id);
    entry["event"] = std::move(event);

    if (!data.empty()) {
        entry["data"] = std::move(data);
    }

    events_.push_back(std::move(entry));
}

const json &runner::events() const
{
    return events_;
}

void runner::seed_random_engine(std::default_random_engine::result_type seed)
{
    random_engine_.seed(seed);
}

std::default_random_engine &runner::random_engine()
{
    return random_engine_;
}

const std::string &sim::id() const
{
    return id_;
}

void sim::record_event(std::string event, json data)
{
    assert(runner_ != nullptr);
    runner_->record_event(id_, std::move(event), std::move(data));
}

std::default_random_engine &sim::random_engine()
{
    assert(runner_ != nullptr);
    return runner_->random_engine();
}

} /* namespace dl */
