/*
 * Copyright (c) 2024 David Leeds <davidesleeds@gmail.com>
 *
 * jstore is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "../sim.hpp"

#include <ranges>

#include <catch2/catch_test_macros.hpp>

using namespace std;
using namespace dl;

struct test_sim : sim {
    void step(duration time_slice) override
    {
        ++step_count;
    }

    unsigned step_count = 0;
};


TEST_CASE("dl::sim", "[evtol-sim]")
{
    SECTION("no sims")
    {
        runner r;

        /* Check idle state */
        REQUIRE(r.events().empty());
        REQUIRE(std::ranges::empty(r.sims<test_sim>()));
        REQUIRE(r.elapsed() == 0s);

        /* Run should work without sims */
        r.run(100s);

        REQUIRE(r.elapsed() == 100s);
    }

    SECTION("two sims")
    {
        runner r{1s};

        test_sim &sim1 = r.add<test_sim>("sim1");
        test_sim &sim2 = r.add<test_sim>("sim2");

        /* Check idle state */
        REQUIRE(r.events().empty());
        REQUIRE(std::ranges::distance(r.sims<test_sim>()) == 2);
        REQUIRE(r.elapsed() == 0s);
        REQUIRE(sim1.step_count == 0);
        REQUIRE(sim2.step_count == 0);

        /* Advance 100 seconds */
        r.run(100s);

        REQUIRE(r.elapsed() == 100s);
        REQUIRE(r.events().empty());
        REQUIRE(r.elapsed() == 100s);
        REQUIRE(sim1.step_count == 100);
        REQUIRE(sim2.step_count == 100);
    }

    SECTION("record events")
    {
        SECTION("record from runner")
        {
            runner r;

            REQUIRE(r.events().empty());

            /* Report first event at time 0 */
            r.record_event("runner", "Event1", { { "key", 1 } });

            REQUIRE(r.events().size() == 1);
            REQUIRE(r.events()[0] ==
                    json::parse(R"({ "timestamp": 0.0, "id": "runner", "event": "Event1", "data": { "key": 1 } })"));

            r.run(1h);

            /* Report second event at time 3600 */
            r.record_event("runner", "Event2");

            REQUIRE(r.events().size() == 2);
            REQUIRE(r.events()[1] ==
                    json::parse(R"({ "timestamp": 3600.0, "id": "runner", "event": "Event2" })"));
        }

        SECTION("record from sim")
        {
            runner r;

            test_sim &sim1 = r.add<test_sim>("sim1");
            test_sim &sim2 = r.add<test_sim>("sim2");

            REQUIRE(r.events().empty());

            /* Report first event at time 0 */
            sim1.record_event("Event1", { { "key", 1 } });

            REQUIRE(r.events().size() == 1);
            REQUIRE(r.events()[0] ==
                    json::parse(R"({ "timestamp": 0.0, "id": "sim1", "event": "Event1", "data": { "key": 1 } })"));

            r.run(1h);

            /* Report second event at time 3600 */
            sim2.record_event("Event2");

            REQUIRE(r.events().size() == 2);
            REQUIRE(r.events()[1] ==
                    json::parse(R"({ "timestamp": 3600.0, "id": "sim2", "event": "Event2" })"));
        }
    }
}
