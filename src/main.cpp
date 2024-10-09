/*
 * Copyright (c) 2024 David Leeds <davidesleeds@gmail.com>
 *
 * evtol-sim is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <cstdlib>
#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <numeric>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include "aircraft.hpp"
#include "charging_network.hpp"
#include "sim.hpp"
#include "utility.hpp"

using namespace dl;

struct statistics {
    std::chrono::seconds average_charge_time    { 0 };
    std::chrono::seconds average_flight_time    { 0 };
    double average_flight_distance_mi           { 0 };
    double passenger_miles                      { 0 };
    unsigned fault_count                        { 0 };
};

/*
 * Compute performance data across a range of aircraft.
 */
statistics calc_stats(const auto &aircraft)
{
    auto attribs = aircraft | std::views::transform(&aircraft::get_attributes);
    auto stats = aircraft | std::views::transform(&aircraft::get_statistics);

    auto sum = [](auto &&range, auto &&member) {
        auto elements = range | std::views::transform(member);
        using value_type = std::ranges::range_value_t<decltype(elements)>;

        return std::accumulate(elements.begin(), elements.end(), value_type{});
    };

    unsigned flight_count = sum(stats, &aircraft::statistics::flight_count);
    unsigned charge_count = sum(stats, &aircraft::statistics::charge_count);
    statistics result;

    if (charge_count > 0) {
        result.average_charge_time = sum(stats, &aircraft::statistics::charge_time) / charge_count;
    }

    if (flight_count > 0) {
        result.average_flight_time = sum(stats, &aircraft::statistics::flight_time) / flight_count;
        result.average_flight_distance_mi = sum(stats, &aircraft::statistics::flight_distance_mi) / flight_count;
    }

    result.fault_count = sum(stats, &aircraft::statistics::fault_count);

    for (auto &a : aircraft) {
        result.passenger_miles += a.get_attributes().passenger_count *
                                  a.get_statistics().flight_distance_mi;
    }

    return result;
}

/*
 * Dump simulation data to an output stream.
 */
static void print_report(std::ostream &stream, const runner &runner)
{
    json output;

    /* Record simulation events */
    output["events"] = runner.events();

    /* Group aircraft by manufacturer for the simulation summary */
    std::map<std::string_view, std::vector<const aircraft *>> evtols_by_manufacturer;
    for (const auto &a : runner.sims<aircraft>()) {
        evtols_by_manufacturer[a.get_attributes().manufacturer].push_back(&a);
    }

    /* Record simulation summary */
    json &summary = output["summary"];

    for (const auto &[manufacturer, evtols] : evtols_by_manufacturer) {
        auto ptr_to_ref = [](const auto &a) -> const aircraft & { return *a; };
        const statistics stats = calc_stats(evtols | std::views::transform(ptr_to_ref));

        json &mfg_summary = summary[manufacturer];
        mfg_summary["aircraftCount"] = evtols.size();
        mfg_summary["averageChargeTime"] = stats.average_charge_time;
        mfg_summary["averageFlightTime"] = stats.average_flight_time;
        mfg_summary["averageFlightMiles"] = stats.average_flight_distance_mi;
        mfg_summary["faultCount"] = stats.fault_count;
        mfg_summary["passengerMiles"] = stats.passenger_miles;
    }

    stream << output.dump(2) << "\n";
}

/*
 * Table of aircraft models.
 */
static const std::array<aircraft::attributes, 5> aircraft_types{
        aircraft::attributes{
        .manufacturer = "Alpha",
        .passenger_count = 4,
        .flight_speed_mph = 120,
        .flight_consumption_kwh_per_mi = 1.6,
        .battery_capacity_kwh = 320,
        .faults_per_hr = 0.25,
        .charge_time = 36min
    },
    aircraft::attributes{
        .manufacturer = "Bravo",
        .passenger_count = 5,
        .flight_speed_mph = 100,
        .flight_consumption_kwh_per_mi = 1.5,
        .battery_capacity_kwh = 100,
        .faults_per_hr = 0.10,
        .charge_time = 12min
    },
    aircraft::attributes{
        .manufacturer = "Charlie",
        .passenger_count = 3,
        .flight_speed_mph = 160,
        .flight_consumption_kwh_per_mi = 2.2,
        .battery_capacity_kwh = 220,
        .faults_per_hr = 0.05,
        .charge_time = 48min
    },
    aircraft::attributes{
        .manufacturer = "Delta",
        .passenger_count = 2,
        .flight_speed_mph = 90,
        .flight_consumption_kwh_per_mi = 0.8,
        .battery_capacity_kwh = 120,
        .faults_per_hr = 0.22,
        .charge_time = 2232s
    },
    aircraft::attributes{
        .manufacturer = "Echo",
        .passenger_count = 2,
        .flight_speed_mph = 30,
        .flight_consumption_kwh_per_mi = 5.8,
        .battery_capacity_kwh = 150,
        .faults_per_hr = 0.61,
        .charge_time = 18min
    }
};

int main(int argc, char **argv)
{
    runner runner;

    /* Apply a fixed seed to the PRNG for reproducible random sequences */
    runner.seed_random_engine(1);

    /*
     * Simulate a charging network with three ports available.
     */
    charging_network &chargers = runner.add<charging_network>("Charging Network", 3);

    /*
     * Simulate 20 eVTOLs with pseudo-randomized models.
     */
    std::uniform_int_distribution<size_t> dist{0, aircraft_types.size() - 1};
    for (unsigned id = 1; id <= 20; ++id) {
        const aircraft::attributes &attributes = aircraft_types[dist(runner.random_engine())];

        aircraft &a = runner.add<aircraft>(fmt::format("EV{:03}", id), attributes, chargers);

        /* Aircraft immediately take off */
        a.set_state(aircraft::state::FLYING);
    }

    /*
     * Simulate three hours of operation.
     */
    runner.run(3h);

    /*
     * Dump simulation data to stdout.
     */
    print_report(std::cout, runner);

    return EXIT_SUCCESS;
}
