#define _USE_MATH_DEFINES

#include <vector>
#include "../src/collision_detector.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <ranges>

using namespace std::literals;

namespace Catch {
	template<>
	struct StringMaker<collision_detector::GatheringEvent> {
	  static std::string convert(collision_detector::GatheringEvent const& value) {
	      std::ostringstream tmp;
	      tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";

	      return tmp.str();
	  }
	};
}  // namespace Catch 

class ProviderForTests: public collision_detector::ItemGathererProvider
{
public:
	size_t ItemsCount() const
	{
		return items_.size();
	}

	collision_detector::Item GetItem(size_t idx) const
	{
		return items_[idx];
	}

	size_t GatherersCount() const
	{
		return gatherers_.size();
	}

	collision_detector::Gatherer GetGatherer(size_t idx) const
	{
		return gatherers_[idx];
	}

	void AddGatherer(collision_detector::Gatherer& new_gath)
	{
		gatherers_.emplace_back(new_gath);
	}

	void AddItem(collision_detector::Item& new_item)
	{
		items_.emplace_back(new_item);
	}

private:
	std::vector<collision_detector::Item> items_;
	std::vector<collision_detector::Gatherer> gatherers_;
};


template <typename Range>
struct IsPermutationMatcher : Catch::Matchers::MatcherGenericBase {
    IsPermutationMatcher(Range range)
        : range_{std::move(range)} {
        // std::sort(std::begin(range_), std::end(range_));
    }
    IsPermutationMatcher(IsPermutationMatcher&&) = default;

    template <typename OtherRange>
    bool match(OtherRange& other) const {
        using std::begin;
        using std::end;
        using Catch::Matchers::WithinAbs;

        size_t range_elements = 0;
        size_t other_elements = 0;

        for(auto it_range = begin(range_); it_range != end(range_); it_range++)
        {
        	range_elements++;
        }
        for(auto other_range = begin(other); other_range != end(other); other_range++)
        {
        	other_elements++;
        }

        CHECK(range_elements == other_elements);

        for(int i = 0; i < range_elements; i++)
        {
        	CHECK(range_[i].item_id == other[i].item_id);
        	CHECK(range_[i].gatherer_id == other[i].gatherer_id);
			CHECK_THAT(range_[i].sq_distance, WithinAbs(other[i].sq_distance, 1e-10));
			CHECK_THAT(range_[i].time, WithinAbs(other[i].time, 1e-10));
        }

        return true;
    }

    std::string describe() const override {
        // Описание свойства, проверяемого матчером:
        return "Is permutation of GatheringEvent "s;
    }

private:
    Range range_;
}; 

template<typename Range>
IsPermutationMatcher<Range> IsEqualGatherEvents(Range&& range) {
    return IsPermutationMatcher<Range>{std::forward<Range>(range)};
} 

SCENARIO("FindGatherer simple mechanics testing") {
		ProviderForTests test_provider;
        GIVEN("lonely gatherer") {
        	collision_detector::Gatherer gath_1{.start_pos = {0,0}, .end_pos = {0,0}, .width = 0.1};
        	WHEN("There is no one") {
	            THEN("nothing happens, all counts are eq to 0") {
	                REQUIRE(test_provider.GatherersCount() == 0);
	                REQUIRE(test_provider.ItemsCount() == 0);
	            }
	        }
        	AND_WHEN("Adding gatherer to provider") {
	            THEN("gatherer count grows") {
	            	test_provider.AddGatherer(gath_1);
	                REQUIRE(test_provider.GatherersCount() == 1);
	            }
	        }
	        AND_WHEN("Adding item to provider") {
	            THEN("item count grows") {
	            	collision_detector::Item item_1 = {.position = {1,1}, .width = 0.2};
	            	test_provider.AddItem(item_1);
	                REQUIRE(test_provider.ItemsCount() == 1);
	            }
	        }

        }
    }


SCENARIO("FindGatherer at work testing") {
		ProviderForTests test_provider;
		GIVEN("one gatherer and one item") {
        	collision_detector::Gatherer gath_1{.start_pos = {0,0}, .end_pos = {0,0}, .width = 0.6};
        	collision_detector::Item item_1{.position = {0,0.2}, .width = 0.0};
        	WHEN("gatherer not moving") {
	            THEN("nothing happens") {
	            	test_provider.AddGatherer(gath_1);
	            	test_provider.AddItem(item_1);
	                std::vector<collision_detector::GatheringEvent> test_event = collision_detector::FindGatherEvents(test_provider);
	                REQUIRE(test_event.size() == 0);

	            }
	        }
        }
        GIVEN("one gatherer and one item") {
        	collision_detector::Gatherer gath_1{.start_pos = {0,0}, .end_pos = {0,1.0}, .width = 0.6};
        	collision_detector::Item item_1{.position = {0,0.2}, .width = 0.0};
        	WHEN("gatherer moves close to target") {
	            THEN("gather item") {
	            	test_provider.AddGatherer(gath_1);
	            	test_provider.AddItem(item_1);
	                std::vector<collision_detector::GatheringEvent> test_event = collision_detector::FindGatherEvents(test_provider);
	                REQUIRE(test_event.size() == 1);

	            }
	        }
        }
        GIVEN("one gatherer and one item, not crossing") {
        	collision_detector::Gatherer gath_1{.start_pos = {0,0}, .end_pos = {0,1.0}, .width = 0.6};
        	collision_detector::Item item_1{.position = {0,2.0}, .width = 0.0};
        	WHEN("gatherer moves close to target") {
	            THEN("not gather item") {
	            	test_provider.AddGatherer(gath_1);
	            	test_provider.AddItem(item_1);
	                std::vector<collision_detector::GatheringEvent> test_event = collision_detector::FindGatherEvents(test_provider);
	                REQUIRE(test_event.size() == 0);

	            }
	        }
        }
        GIVEN("two gatherers and one item") {
        	collision_detector::Gatherer gath_1{.start_pos = {0,0}, .end_pos = {0,1.0}, .width = 0.6};
        	collision_detector::Gatherer gath_2{.start_pos = {0,0}, .end_pos = {0,1.01}, .width = 0.6};
        	collision_detector::Item item_1{.position = {0,1.0}, .width = 0.0};
        	WHEN("gatherer moves close to target") {
	            THEN("not gather item") {
	            	test_provider.AddGatherer(gath_1);
	            	test_provider.AddGatherer(gath_2);
	            	test_provider.AddItem(item_1);
	                std::vector<collision_detector::GatheringEvent> test_event = collision_detector::FindGatherEvents(test_provider);
	                REQUIRE(test_event.size() == 2);

	            }
	        }
        }
        GIVEN("two gatherers and one item") {
        	collision_detector::Gatherer gath_1{.start_pos = {0,0}, .end_pos = {0,1.0}, .width = 0.6};
        	collision_detector::Gatherer gath_2{.start_pos = {0,0}, .end_pos = {0,1.01}, .width = 0.6};
        	collision_detector::Item item_1{.position = {0,1.0}, .width = 0.0};
        	WHEN("gatherer moves close to target") {
	            THEN("not gather item") {
	            	test_provider.AddGatherer(gath_1);
	            	test_provider.AddGatherer(gath_2);
	            	test_provider.AddItem(item_1);
	                std::vector<collision_detector::GatheringEvent> test_event = collision_detector::FindGatherEvents(test_provider);
	                // prepare to test
	                std::vector<collision_detector::GatheringEvent> check_result_event;
	            	auto res_1 = collision_detector::TryCollectPoint(gath_1.start_pos, gath_1.end_pos, item_1.position);
	            	auto res_2 = collision_detector::TryCollectPoint(gath_2.start_pos, gath_2.end_pos, item_1.position);
	                check_result_event.emplace_back(collision_detector::GatheringEvent{.item_id = 0, .gatherer_id = 1, .sq_distance = res_2.sq_distance, .time = res_2.proj_ratio});
	                check_result_event.emplace_back(collision_detector::GatheringEvent{.item_id = 0, .gatherer_id = 0, .sq_distance = res_1.sq_distance, .time = res_1.proj_ratio});
	                std::sort(check_result_event.begin(), check_result_event.end(),
              			[](const collision_detector::GatheringEvent& e_l, const collision_detector::GatheringEvent& e_r) {
                  			return e_l.time < e_r.time;
              			});
	                CHECK_THAT(test_event, IsEqualGatherEvents(std::move(check_result_event)));

	            }
	        }
        }
        GIVEN("two gatherers and two items") {
        	collision_detector::Gatherer gath_1{.start_pos = {0,0}, .end_pos = {0,1.0}, .width = 0.6};
        	collision_detector::Gatherer gath_2{.start_pos = {0,1.8}, .end_pos = {0,2.01}, .width = 0.6};
        	collision_detector::Item item_1{.position = {0,1.0}, .width = 0.0};
        	collision_detector::Item item_2{.position = {0,2.0}, .width = 0.0};
        	WHEN("gatherer moves close to target") {
	            THEN("not gather item") {
	            	test_provider.AddGatherer(gath_1);
	            	test_provider.AddGatherer(gath_2);
	            	test_provider.AddItem(item_1);
	            	test_provider.AddItem(item_2);
	                std::vector<collision_detector::GatheringEvent> test_event = collision_detector::FindGatherEvents(test_provider);
	                // prepare to test
	                std::vector<collision_detector::GatheringEvent> check_result_event;
	            	auto res_1 = collision_detector::TryCollectPoint(gath_1.start_pos, gath_1.end_pos, item_1.position);
	            	// auto res_2 = collision_detector::TryCollectPoint(gath_1.start_pos, gath_1.end_pos, item_2.position);
	            	// auto res_3 = collision_detector::TryCollectPoint(gath_2.start_pos, gath_2.end_pos, item_1.position);
	            	auto res_2 = collision_detector::TryCollectPoint(gath_2.start_pos, gath_2.end_pos, item_2.position);
	                check_result_event.emplace_back(collision_detector::GatheringEvent{.item_id = 1, .gatherer_id = 1, .sq_distance = res_2.sq_distance, .time = res_2.proj_ratio});
	                check_result_event.emplace_back(collision_detector::GatheringEvent{.item_id = 0, .gatherer_id = 0, .sq_distance = res_1.sq_distance, .time = res_1.proj_ratio});
	                std::sort(check_result_event.begin(), check_result_event.end(),
              			[](const collision_detector::GatheringEvent& e_l, const collision_detector::GatheringEvent& e_r) {
                  			return e_l.time < e_r.time;
              			});
	                CHECK_THAT(test_event, IsEqualGatherEvents(std::move(check_result_event)));

	            }
	        }
        }
    }

// Напишите здесь тесты для функции collision_detector::FindGatherEvents