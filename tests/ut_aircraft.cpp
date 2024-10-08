/*
 * Copyright (c) 2024 David Leeds <davidesleeds@gmail.com>
 *
 * jstore is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "../aircraft.hpp"
#include "../charging_network.hpp"

#include <ranges>

#include <catch2/catch_test_macros.hpp>

using namespace std;
using namespace dl;


TEST_CASE("dl::aircraft", "[evtol-sim]")
{
    runner r;

    charging_network &chargers = r.add<charging_network>("Test Charger", 1);

    aircraft::attributes attributes{
        .manufacturer = "eVTOLs R Us",
        .passenger_count = 4,
        .flight_speed_mph = 100,
        .flight_consumption_kwh_per_mi = 1.0,
        .battery_capacity_kwh = 100,
        .faults_per_hr = 1.0,
        .charge_time = 30min
    };

    aircraft &a = r.add<aircraft>("A0001", attributes, chargers);

    /* Check initial state */
    REQUIRE(a.get_statistics().flight_time == 0s);
    REQUIRE(a.get_statistics().charge_time == 0s);
    REQUIRE(a.get_statistics().flight_distance_mi == 0);
    REQUIRE(a.get_statistics().flight_count == 0);
    REQUIRE(a.get_statistics().charge_count == 0);
    REQUIRE(a.get_statistics().fault_count == 0);
    REQUIRE(a.get_state() == aircraft::state::LANDED);

    /* Advance 1 minute (nothing happens while landed) */
    r.run(1min);

    REQUIRE(a.get_statistics().flight_time == 0s);
    REQUIRE(a.get_statistics().charge_time == 0s);
    REQUIRE(a.get_statistics().flight_distance_mi == 0);
    REQUIRE(a.get_statistics().flight_count == 0);
    REQUIRE(a.get_statistics().charge_count == 0);
    REQUIRE(a.get_state() == aircraft::state::LANDED);

    /* Fly for 30 minutes (50% of range) */
    a.set_state(aircraft::state::FLYING);
    r.run(30min);

    REQUIRE(a.get_statistics().flight_time == 30min);
    REQUIRE(a.get_statistics().charge_time == 0s);
    REQUIRE(round(a.get_statistics().flight_distance_mi) == 50);
    REQUIRE(a.get_statistics().flight_count == 1);
    REQUIRE(a.get_statistics().charge_count == 0);
    REQUIRE(a.get_state() == aircraft::state::FLYING);

    /* Fly for another 30 minutes (100% of range, so auto land) */
    r.run(30min);

    REQUIRE(a.get_statistics().flight_time == 60min);
    REQUIRE(a.get_statistics().charge_time == 0s);
    REQUIRE(round(a.get_statistics().flight_distance_mi) == 100);
    REQUIRE(a.get_statistics().flight_count == 1);
    REQUIRE(a.get_statistics().charge_count == 1);
    REQUIRE(a.get_state() == aircraft::state::CHARGING);

    /* Charge for 30 minutes (100% charged, so auto take off) */
    r.run(30min);

    REQUIRE(a.get_statistics().flight_time == 60min);
    REQUIRE(a.get_statistics().charge_time == 30min);
    REQUIRE(round(a.get_statistics().flight_distance_mi) == 100);
    REQUIRE(a.get_statistics().flight_count == 2);
    REQUIRE(a.get_statistics().charge_count == 1);
    REQUIRE(a.get_state() == aircraft::state::FLYING);

    /* Operational time has been well over an hour, so highly likely to have a fault */
    CHECK(a.get_statistics().fault_count > 0);
}
