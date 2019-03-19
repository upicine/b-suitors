#include "blimit.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <queue>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <atomic>
#include <thread>

struct Edge {
    Edge(int n, int weight, int norm_id = -1) : id(n), weight(weight), new_id(norm_id) {}

    int id, weight, new_id;

    bool empty() { return (id == -1); }
};

auto cmp = [](Edge left, Edge right) {

    return ((left.weight > right.weight) ||
            (left.weight == right.weight && left.id > right.id));

};

typedef std::priority_queue<Edge, std::vector<Edge>, decltype(cmp)> edge_queue;
typedef std::unordered_map<int, std::vector<Edge>> mapped_graph;
typedef std::vector<std::vector<Edge>> graph;
typedef std::vector<edge_queue> suitors;

std::vector<std::unique_ptr<std::mutex>> s_mutexes;
std::mutex r_mutex;
int thread_count;


void parse_data(std::ifstream &file, mapped_graph &g) {
    std::string input;
    std::getline(file, input);

    while (!file.eof()) {
        if (input[0] != '#') {
            std::stringstream ss;
            int v, u, weight;
            ss << input;
            ss >> v >> u >> weight;
            g[v].emplace_back(u, weight);
            g[u].emplace_back(v, weight);
        }

        std::getline(file, input);
    }

    file.close();
}

void normalize_graph(
        const mapped_graph &mapped_g, 
        graph &g, 
        std::vector<int> &old_ids) {
    int i = 0;
    std::unordered_map<int, int> new_ids;

    for (auto &v : mapped_g) {
        old_ids[i] = v.first;
        new_ids[v.first] = i;
        i++;
    }

    i = 0;
    for (auto &v : mapped_g) {
        for (auto &e : v.second) {
            g[i].emplace_back(e.id, e.weight, new_ids.find(e.id)->second);
        }

        i++;
    }
}

void init(suitors &s, std::vector<int> &actual_t) {
    for (int &i : actual_t) {
        s.emplace_back(cmp);
        s_mutexes.push_back(std::make_unique<std::mutex>());
        i = 0;
    }
}

void sort_nodes(graph &g) {
    for (auto &v : g) {
        std::sort(v.begin(), v.end(), cmp);
    }
}

Edge last(
        const edge_queue &q, 
        const int b_method, 
        const int node_id) {
    if (q.size() < bvalue(b_method, node_id)) {
        return {-1, 0};
    }

    return q.top();
}

Edge arg_max(
        const suitors &s, 
        const std::vector<Edge> &neigh, 
        int &recent, 
        const int b_method, 
        const int node_id) {
    while (recent < neigh.size()) {
        if (bvalue(b_method, neigh[recent].id) > 0) {
            s_mutexes[neigh[recent].new_id]->lock();

            if (cmp({node_id, neigh[recent].weight},
                    last(s[neigh[recent].new_id], b_method, neigh[recent].id))) {
                s_mutexes[neigh[recent].new_id]->unlock();
                int tmp = recent;
                recent++;
                return neigh[tmp];
            } else {
                s_mutexes[neigh[recent].new_id]->unlock();
            }
        }

        recent++;
    }

    return {-1, 0};
}

long count_result(suitors &s) {
    long result = 0;

    for (auto &v : s) {
        while (!v.empty()) {
            result += v.top().weight;
            v.pop();
        }
    }

    return result;
}

void init_queue(
        std::vector<int> &q, 
        std::vector<int> &b_val, 
        const std::vector<int> &old_ids, 
        const int b_method) {
    for (int i = 0; i < q.size(); i++) {
        b_val[i] = bvalue(b_method, old_ids[i]);
        q[i] = i;
    }
}

bool is_eligible(
        const Edge &x, 
        const edge_queue &q, 
        const int b_method, 
        const int node_id) {
    return (bvalue(b_method, x.id) > 0) &&
           cmp({node_id, x.weight}, last(q, b_method, x.id));
}

void b_adorators(
        int u, 
        std::vector<int> &q, 
        std::vector<int> &r, 
        std::vector<int> &b_val, 
        std::vector<int> &recent_t,
        std::vector<int> &old_ids, 
        std::vector<int> &t_size, 
        suitors &s, 
        const graph &g, 
        const int b_method) {
    int i = 0;
    while (i < b_val[u]) {

        Edge x = arg_max(s, g[u], recent_t[u], b_method, old_ids[u]);

        if (x.empty()) {
            break;
        }
        s_mutexes[x.new_id]->lock();
        if (is_eligible(x, s[x.new_id], b_method, old_ids[u])) {
            edge_queue *x_suitors = &(s[x.new_id]);

            Edge y = last(*x_suitors, b_method, x.id);

            if (!y.empty()) {
                x_suitors->pop();
                r_mutex.lock();
                if (t_size[y.new_id] == 0) {
                    r.emplace_back(y.new_id);
                }
                t_size[y.new_id]++;
                r_mutex.unlock();
            }

            x_suitors->push({old_ids[u], x.weight, u});
            i++;
        }
        s_mutexes[x.new_id]->unlock();
    }
}

void find_adorators(
        std::vector<int> nodes, 
        std::vector<int> &q, 
        std::vector<int> &r, 
        std::vector<int> &b_val,
        std::vector<int> &recent_t,
        std::vector<int> &old_ids, 
        std::vector<int> &t_size, 
        suitors &s, 
        const graph &g,
        const int b_method) {

    for (int &node : nodes) {
        b_adorators(node, q, r, b_val, recent_t, old_ids, t_size, s, g, b_method);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "usage: " << argv[0] << " thread-count inputfile b-limit" << std::endl;
        return 1;
    }

    thread_count = std::stoi(argv[1]);
    int b_limit = std::stoi(argv[3]);
    std::string input_filename{argv[2]};

    std::ifstream file(input_filename);

    mapped_graph mapped_g;
    parse_data(file, mapped_g);

    graph g(mapped_g.size());
    std::vector<int> old_ids(mapped_g.size());
    normalize_graph(mapped_g, g, old_ids);
    sort_nodes(g);

    suitors s;
    std::vector<int> recent_t(g.size());

    init(s, recent_t);

    std::vector<int> t_size(recent_t);

    for (int b_method = 0; b_method < b_limit + 1; b_method++) {
        std::vector<int> q(g.size()), r, b_val(g.size());
        init_queue(q, b_val, old_ids, b_method);

        while (!q.empty()) {
            std::thread threads[thread_count];

            std::vector<int> thread_nodes[thread_count];

            for (int i = 0; i < q.size(); ++i) {
                thread_nodes[i % thread_count].push_back(q[i]);
            }

            for (int i = 0; i < thread_count; ++i) {
                threads[i] = std::thread(find_adorators, 
                                        thread_nodes[i], 
                                        std::ref(q), 
                                        std::ref(r), 
                                        std::ref(b_val),
                                        std::ref(recent_t),
                                        std::ref(old_ids), 
                                        std::ref(t_size), 
                                        std::ref(s), 
                                        std::ref(g), 
                                        b_method);
            }

            for (int i = 0; i < thread_count; ++i) {
                threads[i].join();
            }

            std::swap(b_val, t_size);
            std::fill(t_size.begin(), t_size.end(), 0);
            std::swap(q, r);
            r.clear();
        }

        std::fill(recent_t.begin(), recent_t.end(), 0);
        std::fill(t_size.begin(), t_size.end(), 0);
        std::cout << count_result(s) / 2 << std::endl;
    }
}
