#pragma once
#include <chrono>
#include <functional>
#include <random>

namespace loot_gen {



// double inline loot_random()
// {
//     std::random_device rd;  // Will be used to obtain a seed for the random number engine
//     std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
//     std::uniform_real_distribution<> dis(0.0, 1.0);
//     return dis(gen);
// }

/*
 *  Генератор трофеев
 */
class LootGenerator {
public:
    using RandomGenerator = std::function<double()>;
    using TimeInterval = std::chrono::milliseconds;

    /*
     * base_interval - базовый отрезок времени > 0
     * probability - вероятность появления трофея в течение базового интервала времени
     * random_generator - генератор псевдослучайных чисел в диапазоне от [0 до 1]
     */
    LootGenerator(TimeInterval base_interval, double probability = 0.5,
                  RandomGenerator random_gen = DefaultGenerator)
        : base_interval_{base_interval}
        , probability_{probability}
        , random_generator_{std::move(random_gen)} {
    }

    /*
     * Возвращает количество трофеев, которые должны появиться на карте спустя
     * заданный промежуток времени.
     * Количество трофеев, появляющихся на карте не превышает количество мародёров.
     *
     * time_delta - отрезок времени, прошедший с момента предыдущего вызова Generate
     * loot_count - количество трофеев на карте до вызова Generate
     * looter_count - количество мародёров на карте
     */
    unsigned Generate(TimeInterval time_delta, unsigned loot_count, unsigned looter_count);

private:
    static double DefaultGenerator() noexcept {
        // std::random_device rd;  // Will be used to obtain a seed for the random number engine
        // std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
        // std::uniform_real_distribution<> dis(0.0, 1.0);
        // return dis(gen);
        return 1.0;
    };
    TimeInterval base_interval_;
    double probability_;
    TimeInterval time_without_loot_{};
    RandomGenerator random_generator_;
};

}  // namespace loot_gen
