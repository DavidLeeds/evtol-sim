/*
 * Copyright (c) 2024 David Leeds <davidesleeds@gmail.com>
 *
 * evtol-sim is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#pragma once

#include <fmt/format.h>

#include "sim.hpp"

namespace dl {

/*
 * A simulator for a collection of charging stations.
 *
 * The simulator is trivial and it is assumed that energy is dispensed at a
 * rate determined by the consumer between plug-in and plug-out.
 */
class charging_network : public sim {
public:
    charging_network(unsigned num_chargers) :
        num_chargers_(num_chargers)
    {
    }

    unsigned available_chargers() const
    {
        return num_chargers_ - occupied_chargers_;
    }

    unsigned occupied_chargers() const
    {
        return occupied_chargers_;
    }

    void plug_in()
    {
        if (available_chargers() == 0) {
            throw std::runtime_error("no available chargers");
        }

        ++occupied_chargers_;

        /* Record simulator event on plug-in */
        record_event("PlugIn", dump());
    }

    void plug_out()
    {
        if (occupied_chargers_ == 0) {
            throw std::runtime_error("no chargers in use");
        }

        --occupied_chargers_;

        /* Record simulator event on plug-out */
        record_event("PlugOut", dump());
    }

    void step(duration time_slice) override
    {
        /* No active simulation at the charging site right now */
    }

    json dump() const
    {
        json j;

        j["availableChargers"] = available_chargers();
        j["occupiedChargers"] = occupied_chargers();

        return j;
    }

private:
    unsigned num_chargers_;
    unsigned occupied_chargers_{0};
};

} /* namespace dl */
