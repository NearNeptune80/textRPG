#pragma once

#include <algorithm>
#include <concepts>
#include <random>
#include <vector>

/**
 * High-performance, uniform pseudo-random number generator utility.
 * Eliminates C-style rand() modulo bias and provides clean modern C++26 distributions.
 */
class dice
{
public:
    static std::mt19937& getEngine()
    {
        thread_local std::mt19937 engine{ std::random_device{}() };
        return engine;
    }

    // Roll an integer in the inclusive range [min, max]
    template<std::integral T>
    [[nodiscard]] static T rollInt(T min, T max)
    {
        if (min >= max) return min;
        std::uniform_int_distribution<T> dist(min, max);
        return dist(getEngine());
    }

    // Roll a float in the range [min, max]
    template<std::floating_point T>
    [[nodiscard]] static T rollFloat(T min, T max)
    {
        if (min >= max) return min;
        std::uniform_real_distribution<T> dist(min, max);
        return dist(getEngine());
    }

    // Roll a normalised float in [0.0, 1.0)
    [[nodiscard]] static float roll01()
    {
        return rollFloat(0.0f, 1.0f);
    }

    // Roll a percentage check in [0, 100)
    [[nodiscard]] static bool rollPercent(float chance)
    {
        if (chance <= 0.0f) return false;
        if (chance >= 100.0f) return true;
        return rollFloat(0.0f, 100.0f) < chance;
    }

    // Select a random element from a vector
    template<typename T>
    [[nodiscard]] static const T* choose(const std::vector<T>& vec)
    {
        if (vec.empty()) return nullptr;
        size_t idx = rollInt<size_t>(0, vec.size() - 1);
        return &vec[idx];
    }

    // Select a random weighted index from weights array
    [[nodiscard]] static size_t rollWeighted(const std::vector<int>& weights)
    {
        if (weights.empty()) return 0;
        int totalWeight = 0;
        for (int w : weights) totalWeight += std::max(0, w);
        if (totalWeight <= 0) return 0;

        int roll = rollInt(0, totalWeight - 1);
        int accumulated = 0;
        for (size_t i = 0; i < weights.size(); ++i)
        {
            accumulated += std::max(0, weights[i]);
            if (roll < accumulated) return i;
        }
        return weights.size() - 1;
    }
};
