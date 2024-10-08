/*
 * Copyright (c) 2024 David Leeds <davidesleeds@gmail.com>
 *
 * evtol-sim is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#pragma once

#include <nlohmann/json.hpp>

/*
 * Define JSON ADL serializers in the nlohmann namespace
 */

namespace nlohmann {

/*
 * std::chrono::duration (floating point second representation)
 */
template <typename Rep, typename Period>
struct adl_serializer<std::chrono::duration<Rep, Period>> {
    static void to_json(json &j, const std::chrono::duration<Rep, Period> &v)
    {
        j = std::chrono::duration<double>(v).count();
    }

    static void from_json(const json &j, std::chrono::duration<Rep, Period> &v)
    {
        v = std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(
                std::chrono::duration<double>(j.get<double>()));
    }
};

} /* namespace nlohmann */
