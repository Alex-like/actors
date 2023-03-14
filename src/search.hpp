//
// Created by Alex Shchelochkov on 04.03.2023.
//
#pragma once

#include <unordered_map>
#include <chrono>
#include <thread>
#include <vector>
#include <sstream>
#include <unistd.h>

namespace search {
    enum class search_system_t : uint8_t {
        Google=0,
        Yandex,
        Bing
    };

    constexpr std::initializer_list<search_system_t> all_search_system = {
        search_system_t::Google,
        search_system_t::Yandex,
        search_system_t::Bing
    };

    std::string to_string(search_system_t ss) {
        switch (ss) {
            case search_system_t::Google    : return "Google";
            case search_system_t::Yandex    : return "Yandex";
            case search_system_t::Bing      : return "Bing";
            default                         : return "";
        }
    }

    bool parse(std::string_view input, search::search_system_t& dest) {
        if (input == "Google")
            dest = search_system_t::Google;
        else if (input == "Yandex")
            dest = search_system_t::Yandex;
        else if (input == "Bing")
            dest = search_system_t::Bing;
        else return false;
        return true;
    }

    class search_request_t {
    private:
        search_system_t search_system;
        std::string query;
    public:
        explicit search_request_t(search_system_t ss = search_system_t::Google, const std::string& query = "")
        : search_system(ss), query(query)
        {}
        void set_search_system(search_system_t new_system) {
            search_system = new_system;
        }
        void set_query(const std::string &new_query) {
            query = new_query;
        }
        std::string get_query() const {
            return query;
        }
        search_system_t get_search_system() const {
            return search_system;
        }
    };

    struct search_result_item {
        std::string url;
        std::string tittle;

        std::string to_string() const {
            return "{url : \"" + url + "\" , tittle : \"" + tittle + "\" }";
        }
    };

    class search_result_t {
    private:
        using result_query = std::vector<search_result_item>;
        using result_map = std::unordered_map<search_system_t, result_query>;
        result_map search_result;
    public:
        explicit search_result_t(search_system_t ss = search_system_t::Google, const result_query& result = {}) {
            if (!result.empty())
                search_result.insert({ss, result});
        }
        void merge_result(result_map sr) {
            search_result.merge(sr);
        }
        void set_search_result(result_map new_res) {
            search_result = std::move(new_res);
        }
        result_map get_search_result() const {
            return search_result;
        }
        std::string to_string() const {
            std::stringstream ss;
            ss << "[";
            for (search_system_t system : all_search_system) {
                std::stringstream items;
                for (const auto& item : search_result.at(system))
                    items << "\t\t" << item.to_string() << '\n';
                ss << "{\n";
                ss << "\tsearch-system : \"" << search::to_string(system) << "\", \n";
                ss << "\titems : [\n" << items.str() << "\t]\n";
                ss << "}";
            }
            ss << "]";
            return ss.str();
        }
        size_t size() const {
            return search_result.size();
        }
    };

    class search_client_t {
    public:
        virtual search_result_t search_query(search_request_t request) const = 0;
    };

    class search_client_stub : public search_client_t {
    private:
        const uint32_t BATCH_SIZE = 15;
        std::chrono::duration<int> response_delay;

        std::string gen_url(uint32_t id, const std::string& query_text) const {
            return "https://search-result/" + query_text + "/" + std::to_string(id);
        }
        std::string gen_title(uint32_t id, const std::string& query_text) const {
            return "Search result title #" + std::to_string(id) + " for query \"" + query_text + "\" #";
        }
    public:
        explicit search_client_stub(std::chrono::duration<int> delay) : response_delay(delay) {}
        search_result_t search_query(search_request_t request) const override {
            std::vector<search_result_item> result_query = {};
            for (uint32_t id = 0; id < BATCH_SIZE; id++)
                result_query.push_back({
                    gen_url(id, request.get_query()),
                    gen_title(id, request.get_query())
                });
            std::this_thread::sleep_for(response_delay);
            return search_result_t{request.get_search_system(), result_query};
        }
    };
}