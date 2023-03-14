//
// Created by Alex Shchelochkov on 05.03.2023.
//
#pragma once

#include "search.hpp"
#include "caf/all.hpp"

#ifdef __clang__
#  pragma clang diagnostic ignored "-Wshorten-64-to-32"
#  pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#  pragma clang diagnostic ignored "-Wunused-const-variable"
#endif // __clang__

CAF_BEGIN_TYPE_ID_BLOCK(custom_types_1, first_custom_type_id)
    CAF_ADD_TYPE_ID(custom_types_1, (search::search_request_t))
    CAF_ADD_TYPE_ID(custom_types_1, (search::search_result_t))
CAF_END_TYPE_ID_BLOCK(custom_types_1)

namespace caf {
    template<>
    struct inspector_access<search::search_system_t> : inspector_access_base<search::search_system_t> {
        template<class Inspector>
        static bool apply(Inspector& f, search::search_system_t& x) {
            if (f.has_human_readable_format()) {
                auto get = [&x] { return to_string(x); };
                auto set = [&x](std::string str) { return parse(str, x); };
                return f.apply(get, set);
            } else {
                auto get = [&x] { return static_cast<uint8_t>(x); };
                auto set = [&x](uint8_t val) {
                    if (val < search::all_search_system.size()) {
                        x = static_cast<search::search_system_t>(val);
                        return true;
                    } else {
                        return false;
                    }
                };
                return f.apply(get, set);
            }
        }
    };

    template<>
    struct inspector_access<search::search_result_item> : inspector_access_base<search::search_result_item> {
        template<class Inspector>
        static bool apply(Inspector& f, search::search_result_item& x) {
            return f.object(x).fields(f.field("url", x.url),
                                      f.field("tittle", x.tittle));
        }
    };

    template<>
    struct inspector_access<search::search_request_t> : inspector_access_base<search::search_request_t> {
        template<class Inspector>
        static bool apply(Inspector& f, search::search_request_t& x) {
            auto get_system = [&x] { return x.get_search_system(); };
            auto set_system = [&x](search::search_system_t val) {
                x.set_search_system(val);
                return true;
            };
            auto get_query = [&x] { return x.get_query(); };
            auto set_query = [&x](std::string val) {
                x.set_query(val);
                return true;
            };
            return f.object(x).fields(f.field("search_system", get_system, set_system),
                                      f.field("query", get_query, set_query));
        }
    };

    template<>
    struct inspector_access<search::search_result_t> : inspector_access_base<search::search_result_t> {
        template<class Inspector>
        static bool apply(Inspector& f, search::search_result_t& x) {
            auto get_res = [&x] { return x.get_search_result(); };
            auto set_res = [&x](std::unordered_map<search::search_system_t, std::vector<search::search_result_item>> val) {
                x.set_search_result(val);
                return true;
            };
            return f.object(x).fields(f.field("search_result", get_res, set_res));
        }
    };
}

namespace search_actor {
    caf::behavior child_actor(caf::event_based_actor* self,
                              std::shared_ptr<search::search_client_t> client) {
        return {
            [=](const search::search_request_t& request) -> search::search_result_t {
                return client->search_query(request);
            },
        };
    }

    struct master_state {
        caf::event_based_actor* self;
        std::chrono::duration<int> receive_timeout;
        std::unordered_map<search::search_system_t, std::shared_ptr<search::search_client_t>> clients;
        search::search_result_t result;

        static inline const char* name = "master";

        explicit master_state(caf::event_based_actor* self,
                              std::unordered_map<search::search_system_t, std::shared_ptr<search::search_client_t>> clients,
                              std::chrono::duration<int> timeout = std::chrono::seconds(1))
        : self(self), receive_timeout(timeout), clients(clients)
        {
            // nop
        }
        caf::behavior make_behavior() {
            return {
                [=](const std::string& msg) {
                    caf::scoped_actor scope{self->system()};
                    for (auto& p : clients) {
                        auto child = self->spawn<caf::detached + caf::linked>(child_actor, p.second);
                        scope->request(child, receive_timeout, search::search_request_t{p.first, msg})
                            .receive(
                                    [&](const search::search_result_t& res) {
                                        result.merge_result(res.get_search_result());
                                    },
                                    [&](caf::error& err) {
                                        if (err.compare(caf::sec::request_timeout) == 0)
                                            result.merge_result({{p.first, {}}});
                                    });
                    }
                    while (result.size() != clients.size());
                    return result;
                },
            };
        }
    };
}