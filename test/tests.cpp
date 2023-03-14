//
// Created by Alex Shchelochkov on 14.03.2023.
//
#define CAF_SUITE child_actor

#include "caf/test/dsl.hpp"

#include "caf/all.hpp"
#include "../src/search_actor.hpp"

#define CAF_TEST_NO_MAIN
int main(int argc, char** argv) {
    caf::exec_main_init_meta_objects<caf::id_block::custom_types_1>();
    caf::core::init_global_meta_objects();
    return caf::test::main(argc, argv);
}

#include "caf/test/unit_test_impl.hpp"

using namespace std;
using namespace caf;
using namespace search;
using namespace search_actor;

namespace {
    struct fixture {
        caf::actor_system_config cfg;
        caf::actor_system sys;
        caf::scoped_actor self;

        fixture() : sys(cfg), self(sys) {
            // nop
        }
    };
} // namespace

CAF_TEST_FIXTURE_SCOPE(actor_tests, fixture)

    CAF_TEST(child_actor_test) {
        // Our Actor-Under-Test.
        shared_ptr<search_client_t> client = make_shared<search_client_stub>(0s);
        auto child = self->spawn<caf::detached + caf::linked>(child_actor, client);
        self->request(child, caf::infinite, search::search_request_t{search_system_t::Yandex, "test"}).receive(
                [=](const search::search_result_t& res) {
                    CAF_CHECK(res.size() == 1);
                    CAF_CHECK(!res.get_search_result().contains(search_system_t::Google));
                },
                [&](caf::error& err) {
                    // Must not happen, stop test.
                    CAF_FAIL(err);
                });
    }

    CAF_TEST(timeout_master_actor_test) {
        unordered_map<search_system_t, shared_ptr<search_client_t>> clients;
        for (const auto& sys : all_search_system)
            clients[sys] = make_shared<search_client_stub>(10s);
        auto master =  self->spawn<stateful_actor<master_state>>(clients, 1s);
        self->request(master, caf::infinite, "test").receive(
                [&](const search_result_t& res) {
                    CAF_CHECK(res.size() == clients.size());
                    auto map = res.get_search_result();
                    CAF_CHECK(all_of(map.begin(), map.end(), [](auto p){ return p.second.empty(); }));
                },
                [&](error& err) {
                    // Must not happen, stop test.
                    CAF_FAIL(err);
                });
    }

    CAF_TEST(full_result_master_actor_test) {
        unordered_map<search_system_t, shared_ptr<search_client_t>> clients;
        for (const auto& sys : all_search_system)
            clients[sys] = make_shared<search_client_stub>(1s);
        auto master =  self->spawn<stateful_actor<master_state>>(clients, 2s);
        self->request(master, caf::infinite, "test").receive(
                [&](const search_result_t& res) {
                    CAF_CHECK(res.size() == clients.size());
                    auto map = res.get_search_result();
                    CAF_CHECK(all_of(map.begin(), map.end(), [](auto p){ return !p.second.empty(); }));
                },
                [&](error& err) {
                    // Must not happen, stop test.
                    CAF_FAIL(err);
                });
    }

    CAF_TEST(semiful_result_master_actor_test) {
        unordered_map<search_system_t, shared_ptr<search_client_t>> clients = {
                {search_system_t::Google, make_shared<search_client_stub>(1s)},
                {search_system_t::Yandex, make_shared<search_client_stub>(10s)},
                {search_system_t::Bing, make_shared<search_client_stub>(2s)}
        };
        auto master =  self->spawn<stateful_actor<master_state>>(clients, 2s);
        self->request(master, caf::infinite, "test").receive(
                [&](const search_result_t& res) {
                    CAF_CHECK(res.size() == clients.size());
                    auto map = res.get_search_result();
                    CAF_CHECK(any_of(map.begin(), map.end(), [](auto p){ return p.second.empty(); }));
                    CAF_CHECK(any_of(map.begin(), map.end(), [](auto p){ return !p.second.empty(); }));
                },
                [&](error& err) {
                    // Must not happen, stop test.
                    CAF_FAIL(err);
                });
    }

CAF_TEST_FIXTURE_SCOPE_END()