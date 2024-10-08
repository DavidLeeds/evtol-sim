/*
 * Copyright (c) 2024 David Leeds <davidesleeds@gmail.com>
 *
 * evtol-sim is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <random>
#include <string>

#include "charging_network.hpp"
#include "sim.hpp"
#include "utility.hpp"

namespace dl {

/*
 * Simplistic eVTOL simulator.
 */
class aircraft : public sim {
    using double_hours = std::chrono::duration<double, std::ratio<3600>>;
public:
    struct attributes {
        std::string manufacturer                {};
        unsigned passenger_count                {};
        unsigned flight_speed_mph               {};
        float flight_consumption_kwh_per_mi     {};
        float battery_capacity_kwh              {};
        float faults_per_hr                     {};
        duration charge_time                    {};
    };

    struct statistics {
        duration flight_time                    {};
        duration charge_time                    {};
        double flight_distance_mi               {};
        unsigned flight_count                   {};
        unsigned charge_count                   {};
        unsigned fault_count                    {};
    };

    enum class state {
        LANDED,
        FLYING,
        QUEUED_TO_CHARGE,
        CHARGING
    };

    aircraft(attributes attribs, charging_network &chargers) :
        attributes_(std::move(attribs)),
        chargers_(chargers)
    {
    }

    const attributes &get_attributes() const
    {
        return attributes_;
    }

    const statistics &get_statistics() const
    {
        return statistics_;
    }

    state get_state() const
    {
        return state_;
    }

    void set_state(state state)
    {
        if (state_ == state) {
            return;
        }

        /* Update stats on state entry */
        switch (state) {
        case state::FLYING:
            ++statistics_.flight_count;
            break;

        case state::CHARGING:
            ++statistics_.charge_count;
            break;

        default:
            break;
        }

        /* Update charger state */
        if (state == state::CHARGING) {
            chargers_.plug_in();
        } else if (state_ == state::CHARGING) {
            chargers_.plug_out();
        }

        state_ = state;

        /* Record simulator event on state change */
        record_event("StateChanged", dump());
    }

    void step(duration time_slice) override
    {
        /*
         * Update stats in current state.
         */
        switch (state_) {
        case state::LANDED:
            break;

        case state::FLYING:
        {
            double step_distance_mi = double_hours{time_slice}.count() * attributes_.flight_speed_mph;
            double step_energy_kwh = step_distance_mi * attributes_.flight_consumption_kwh_per_mi;

            statistics_.flight_time += time_slice;
            statistics_.flight_distance_mi += step_distance_mi;

            /* Model linear energy consumption during flight */
            battery_energy_kwh_ = std::max<float>(battery_energy_kwh_- step_energy_kwh, 0);
            break;
        }

        case state::QUEUED_TO_CHARGE:
            break;

        case state::CHARGING:
        {
            double step_energy_kwh = (double_hours{time_slice} / attributes_.charge_time) * attributes_.battery_capacity_kwh;

            statistics_.charge_time += time_slice;

            /* Model linear battery charge rate */
            battery_energy_kwh_ = std::min<float>(battery_energy_kwh_ + step_energy_kwh, attributes_.battery_capacity_kwh);
            break;
        }
        }

        /*
         * Apply fault probability equally in all operational states.
         */
        std::bernoulli_distribution fault_distribution{attributes_.faults_per_hr * double_hours{time_slice}.count()};
        statistics_.fault_count += fault_distribution(random_engine());

        /*
         * Evaluate automatic state transitions.
         */
        switch (state_) {
        case state::LANDED:
            break;

        case state::FLYING:
            /* Charge when battery is dead */
            if (battery_energy_kwh_ <= 0) {
                if (chargers_.available_chargers() > 0) {
                    set_state(state::CHARGING);
                } else {
                    set_state(state::QUEUED_TO_CHARGE);
                }
            }
            break;

        case state::QUEUED_TO_CHARGE:
            /* Start charging when a charger is available */
            if (chargers_.available_chargers() > 0) {
                set_state(state::CHARGING);
            }
            break;

        case state::CHARGING:
            /* Take off when battery is charged */
            if (battery_energy_kwh_ >= attributes_.battery_capacity_kwh) {
                set_state(state::FLYING);
            }
            break;
        }
    }

    json dump() const
    {
        json j;

        j["state"] = to_string(state_);
        j["batteryEnergyKwh"] = battery_energy_kwh_;
        j["batteryPercent"] = std::round(100 * battery_energy_kwh_ / attributes_.battery_capacity_kwh);
        j["flightTime"] = statistics_.flight_time;
        j["chargeTime"] = statistics_.charge_time;
        j["flightMiles"] = statistics_.flight_distance_mi;
        j["flightCount"] = statistics_.flight_count;
        j["chargeCount"] = statistics_.charge_count;
        j["faultCount"] = statistics_.fault_count;

        return j;
    }

private:
    static constexpr std::string_view to_string(state s)
    {
        switch (s) {
        case state::LANDED:             return "Landed";
        case state::FLYING:             return "Flying";
        case state::QUEUED_TO_CHARGE:   return "QueuedToCharge";
        case state::CHARGING:           return "Charging";
        }
        return {};
    }

    attributes attributes_;
    statistics statistics_;
    state state_{state::LANDED};
    float battery_energy_kwh_{attributes_.battery_capacity_kwh};
    charging_network &chargers_;
};

} /* namespace dl */
