#include <iostream>
#include <utility>

#include "caf/all.hpp"
#include "src/search_actor.hpp"

using namespace std;
using namespace caf;
using namespace search;
using namespace search_actor;

void caf_main(actor_system& system) {
    string msg;
    unordered_map<search_system_t, shared_ptr<search_client_t>> clients;
    for (const auto& sys : all_search_system)
        clients[sys] = make_shared<search_client_stub>(1s);
    while (cin >> msg) {
        scoped_actor self{system};
        auto master =  self->spawn<stateful_actor<master_state>>(clients, 2s);
        self->request(master, caf::infinite, msg).receive(
                [&](const search_result_t& res) {
                    aout(self) << res.to_string() << std::endl;
                },
                [&](error& err) {
                    aout(self) << "receive error " << to_string(err) << " from "
                    << (self->current_sender() == master ? "master\n" : "child\n"); });
    }
}

CAF_MAIN(id_block::custom_types_1)