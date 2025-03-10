#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <random>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <iomanip>
#include <chrono>

using namespace std;
using namespace chrono;

typedef vector<pair<double, int>> Chromosome;

struct Item {
    int weight, value;
    double value_per_weight;
};

// Parâmetros do BRKGA
constexpr int P = 100;        // Tamanho da população
constexpr double pe = 0.3;    // Percentual de elite (30%)
constexpr double pm = 0.2;    // Percentual de mutantes (20%)
constexpr double rhoe = 0.7;  // Probabilidade de herança do elite (70%)
constexpr int G = 200;       // Número de gerações

int N, C;
vector<Item> items;
vector<pair<Chromosome, int>> population;

random_device rd;
mt19937 rng(rd());
uniform_real_distribution<double> dist(0.001, 0.999);
uniform_real_distribution<double> mutation_prob(0.0, 1.0);

// Função para gerar a população inicial
void generate_population() {
    population.clear();
    population.reserve(P);

    for (int i = 0; i < P; ++i) {
        Chromosome chromosome;
        chromosome.reserve(N);

        for (int j = 0; j < N; ++j) {
            double gene = dist(rng);
            chromosome.emplace_back(gene * items[j].value_per_weight, j);
        }
        sort(chromosome.rbegin(), chromosome.rend());
        population.emplace_back(chromosome, 0);
    }
}

// Decodifica um cromossomo e calcula o fitness
int decode_chromosome(const Chromosome &chromosome, vector<bool> &solution) {
    solution.assign(N, false);
    int total_weight = 0, total_value = 0;

    for (const auto &c : chromosome) {
        int idx = c.second;
        if (total_weight + items[idx].weight <= C) {
            solution[idx] = true;
            total_weight += items[idx].weight;
            total_value += items[idx].value;
        }
    }
    return total_value;
}

// Avalia toda a população
void evaluate_population() {
    for (auto &individual : population) {
        vector<bool> solution;
        individual.second = decode_chromosome(individual.first, solution);
    }
    sort(population.begin(), population.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });
}

// Crossover entre elite e não-elite
Chromosome crossover(const Chromosome &elite, const Chromosome &non_elite) {
    Chromosome offspring = elite;
    uniform_int_distribution<int> cut_point_dist(1, N - 1);
    int cut = cut_point_dist(rng);

    for (size_t i = cut; i < elite.size(); ++i) {
        offspring[i].first = (dist(rng) < rhoe) ? elite[i].first : non_elite[i].first;
    }
    sort(offspring.rbegin(), offspring.rend());
    return offspring;
}

// Aplica mutação a um cromossomo
void mutate(Chromosome &chromosome) {
    for (auto &c : chromosome) {
        int idx = c.second;
        if (mutation_prob(rng) < 0.05) {
            c.first = dist(rng) * items[idx].value_per_weight;
        }
    }
    sort(chromosome.rbegin(), chromosome.rend());
}

// Evolui a população seguindo o modelo BRKGA
void evolve_population() {
    int elite_size = static_cast<int>(P * pe);
    int mutant_size = static_cast<int>(P * pm);
    int crossover_size = P - elite_size - mutant_size;

    vector<pair<Chromosome, int>> new_population;
    new_population.reserve(P);

    // Mantém a elite
    new_population.insert(new_population.end(), population.begin(), population.begin() + elite_size);

    // Gera mutantes
    for (int i = 0; i < mutant_size; ++i) {
        Chromosome new_chromosome;
        new_chromosome.reserve(N);

        for (int j = 0; j < N; ++j) {
            double gene = dist(rng) * items[j].value_per_weight;
            new_chromosome.emplace_back(gene, j);
        }
        sort(new_chromosome.rbegin(), new_chromosome.rend());
        new_population.emplace_back(new_chromosome, 0);
    }

    // Gera filhos por cruzamento
    for (int i = 0; i < crossover_size; ++i) {
        int elite_idx = uniform_int_distribution<int>(0, elite_size - 1)(rng);
        int normal_idx = uniform_int_distribution<int>(elite_size, P - 1)(rng);

        Chromosome offspring = crossover(population[elite_idx].first, population[normal_idx].first);
        mutate(offspring);
        new_population.emplace_back(offspring, 0);
    }

    population = move(new_population);
    evaluate_population();
}

// Leitura dos dados do arquivo
void read_input(const string &filename) {
    ifstream infile(filename);
    if (!infile) {
        throw runtime_error("Erro ao abrir o arquivo: " + filename);
    }

    infile >> N >> C;
    if (infile.fail()) {
        throw runtime_error("Erro na leitura dos parâmetros do problema.");
    }

    items.resize(N);
    for (int i = 0; i < N; ++i) {
        int id;
        infile >> id >> items[i].weight >> items[i].value;
        if (infile.fail()) {
            throw runtime_error("Erro na leitura dos itens.");
        }
        items[i].value_per_weight = static_cast<double>(items[i].value) / items[i].weight;
    }
}

// Imprime os melhores indivíduos de cada geração
void print_population(int generation) {
    cout << "\nGeração " << generation << ":\n";
    for (size_t i = 0; i < min(size_t(10), population.size()); ++i) {
        cout << "Cromossomo " << i + 1 << " | Fitness: " << population[i].second << endl;
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Uso: " << argv[0] << " <arquivo_de_entrada>\n";
        return EXIT_FAILURE;
    }

    try {
        auto start = high_resolution_clock::now(); // Inicia o contador de tempo

        read_input(argv[1]);
        generate_population();
        evaluate_population();

        for (int gen = 1; gen <= G; ++gen) {
            evolve_population();
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);
        cout << argv[1] << " " << population[0].second << " "<< duration.count()/1000.0 << endl;

    } catch (const exception &e) {
        cerr << "Erro: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
