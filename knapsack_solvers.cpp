#include <iostream> 
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>
#include <chrono>
#include <queue>
#include <filesystem>


namespace fs = std::filesystem;
// --------------------------- Структуры --------------------------- //
struct Item {
    int weight;
    int value;
};

// Для BnB
struct BBNode {
    int level;           // индекс предмета (в отсортированном массиве)
    int totalWeight;     // суммарный вес
    int totalValue;      // суммарная ценность
    double bound;        // верхняя оценка
    std::vector<int> selection; // 0/1 для выбранных предметов (в отсортированном порядке)
};

// Результат ветвей и границ
struct BBResult {
    int bestValue;                 // итоговая максимальная ценность
    std::vector<int> bestSelection; // 0/1 для предметов (в отсортированном порядке)
};

// Для ГА
struct Individual {
    std::vector<int> chromosome; // 0/1 для каждого предмета (в исходном порядке)
    int fitness;
};

// Результат ГА
struct GAResult {
    int bestValue;
    std::vector<int> bestChromosome; // 0/1 (в исходном порядке)
};

// --------------------------- Функции времени --------------------------- //
long long currentTimeMillis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// --------------------------- Функция ремонта для GA --------------------------- //
// Если особь перегружена, удаляем предметы с наименьшим соотношением value/weight
void repairIndividual(Individual &ind, const std::vector<Item> &items, int capacity) {
    int totalWeight = 0;
    for (size_t i = 0; i < ind.chromosome.size(); i++) {
        if (ind.chromosome[i] == 1)
            totalWeight += items[i].weight;
    }
    if (totalWeight > capacity) {
        // Собираем индексы выбранных предметов
        std::vector<int> indices;
        for (size_t i = 0; i < ind.chromosome.size(); i++) {
            if (ind.chromosome[i] == 1) {
                indices.push_back(i);
            }
        }
        // Сортируем индексы по возрастанию отношения value/weight
        std::sort(indices.begin(), indices.end(), [&](int a, int b) {
            double ratioA = (double)items[a].value / items[a].weight;
            double ratioB = (double)items[b].value / items[b].weight;
            return ratioA < ratioB;
        });
        // Убираем предметы до тех пор, пока не достигнем допустимого веса
        for (int idx : indices) {
            if (totalWeight <= capacity)
                break;
            ind.chromosome[idx] = 0; // исключаем предмет
            totalWeight -= items[idx].weight;
        }
    }
}

// ============= Генетический алгоритм (GA) ============= //
int computeFitness(const Individual &ind, const std::vector<Item> &items, int capacity) {
    int totalWeight = 0;
    int totalValue = 0;
    for (size_t i = 0; i < ind.chromosome.size(); i++) {
        if (ind.chromosome[i] == 1) {
            totalWeight += items[i].weight;
            totalValue += items[i].value;
        }
    }
    // Если по каким-то причинам особь перегружена, возвращаем 0
    if (totalWeight > capacity)
        return 0;
    return totalValue;
}

Individual makeIndividual(int n) {
    Individual ind;
    ind.chromosome.resize(n);
    for (int i = 0; i < n; i++) {
        ind.chromosome[i] = rand() % 2; 
    }
    ind.fitness = 0;
    return ind;
}

void crossover(Individual &parent1, Individual &parent2) {
    int size = (int)parent1.chromosome.size();
    int point = rand() % size;
    for (int i = point; i < size; i++) {
        std::swap(parent1.chromosome[i], parent2.chromosome[i]);
    }
}

void mutate(Individual &ind, double mutationRate) {
    for (size_t i = 0; i < ind.chromosome.size(); i++) {
        double r = (double)rand() / RAND_MAX;
        if (r < mutationRate) {
            ind.chromosome[i] = 1 - ind.chromosome[i];
        }
    }
}

// простая турнирная селекция
Individual tournamentSelection(const std::vector<Individual> &population) {
    int size = (int)population.size();
    int i1 = rand() % size;
    int i2 = rand() % size;
    return (population[i1].fitness > population[i2].fitness) ? population[i1] : population[i2];
}

GAResult geneticKnapsack(const std::vector<Item> &items, int capacity) {
    const int POP_SIZE = 50;       
    const int MAX_GEN = 200;      
    const double MUTATION_RATE = 0.05;

    int n = (int)items.size();

    // Инициализируем популяцию
    std::vector<Individual> population(POP_SIZE);
    for (int i = 0; i < POP_SIZE; i++) {
        population[i] = makeIndividual(n);
        // Ремонтируем, если особь перегружена
        repairIndividual(population[i], items, capacity);
        population[i].fitness = computeFitness(population[i], items, capacity);
    }

    int bestFitness = 0;
    std::vector<int> bestChromosome(n, 0);

    for (int gen = 0; gen < MAX_GEN; gen++) {
        std::vector<Individual> newPopulation;

        // Элитизм: сохраняем лучшую особь поколения
        Individual bestInd = population[0];
        for (auto &ind : population) {
            if (ind.fitness > bestInd.fitness) {
                bestInd = ind;
            }
        }
        if (bestInd.fitness > bestFitness) {
            bestFitness = bestInd.fitness;
            bestChromosome = bestInd.chromosome;
        }
        newPopulation.push_back(bestInd);

        // Формируем новое поколение
        while ((int)newPopulation.size() < POP_SIZE) {
            Individual parent1 = tournamentSelection(population);
            Individual parent2 = tournamentSelection(population);

            crossover(parent1, parent2);

            mutate(parent1, MUTATION_RATE);
            mutate(parent2, MUTATION_RATE);

            // Применяем ремонт к новым особям
            repairIndividual(parent1, items, capacity);
            repairIndividual(parent2, items, capacity);

            parent1.fitness = computeFitness(parent1, items, capacity);
            parent2.fitness = computeFitness(parent2, items, capacity);

            newPopulation.push_back(parent1);
            if ((int)newPopulation.size() < POP_SIZE) {
                newPopulation.push_back(parent2);
            }
        }

        population = newPopulation;
    }

    // Финальная проверка лучшей особи в популяции
    for (auto &ind : population) {
        if (ind.fitness > bestFitness) {
            bestFitness = ind.fitness;
            bestChromosome = ind.chromosome;
        }
    }

    GAResult res;
    res.bestValue = bestFitness;
    res.bestChromosome = bestChromosome;
    return res;
}

// ============= Branch & Bound (best-first) ============= //

// Улучшенная функция для вычисления верхней границы (bound)
// Добавлен коэффициент 0.95 для дробной части, чтобы сделать оценку более жёсткой.
double boundBB(const BBNode &u, int n, int capacity, const std::vector<Item> &items) {
    if (u.totalWeight >= capacity) {
        return 0.0;
    }
    double result = (double)u.totalValue;
    int curWeight = u.totalWeight;
    int idx = u.level;

    // Жадное добавление оставшихся предметов (возможно дробное)
    while (idx < n && curWeight + items[idx].weight <= capacity) {
        curWeight += items[idx].weight;
        result += items[idx].value;
        idx++;
    }
    if (idx < n) {
        // Коэффициент 0.95 делает оценку чуть более реалистичной (строгой)
        result += (capacity - curWeight) * (double)items[idx].value / items[idx].weight * 0.95;
    }
    return result;
}

// Для BnB используем приоритетную очередь (best-first search)
struct CompareNode {
    bool operator()(const BBNode &a, const BBNode &b) {
        return a.bound < b.bound; 
    }
};

BBResult branchAndBoundKnapsack(const std::vector<Item> &origItems, int capacity) {
    // Ограничения по времени и глубине поиска:
    const long long TIME_LIMIT_MS = 100000; // Ограничение по времени в миллисекундах (например, 5 секунд)
    const int MAX_DEPTH = 75;             // Ограничение по глубине поиска

    long long startTime = currentTimeMillis();

    // Сортируем предметы по убыванию удельной ценности
    std::vector<Item> items = origItems;
    std::sort(items.begin(), items.end(), [](const Item &a, const Item &b){
        return (double)a.value / a.weight > (double)b.value / b.weight;
    });

    std::priority_queue<BBNode, std::vector<BBNode>, CompareNode> pq;
    
    BBNode u, v;
    u.level = 0;
    u.totalWeight = 0;
    u.totalValue = 0;
    u.selection.resize(items.size(), 0);
    u.bound = boundBB(u, (int)items.size(), capacity, items);
    pq.push(u);

    int maxValue = 0;
    std::vector<int> bestSel(items.size(), 0);

    while (!pq.empty()) {
        // Ограничение по времени: если время работы превышено, выходим из цикла
        if (currentTimeMillis() - startTime > TIME_LIMIT_MS) {
            break;
        }

        u = pq.top();
        pq.pop();

        // Ограничение по глубине: не расширяем узлы, достигшие MAX_DEPTH
        if (u.level >= MAX_DEPTH)
            continue;

        if (u.bound <= (double)maxValue)
            continue;

        if (u.level < (int)items.size()) {
            // Рассматриваем вариант выбора предмета u.level
            v.level = u.level + 1;
            v.totalWeight = u.totalWeight + items[u.level].weight;
            v.totalValue = u.totalValue + items[u.level].value;
            v.selection = u.selection;
            v.selection[u.level] = 1;

            if (v.totalWeight <= capacity && v.totalValue > maxValue) {
                maxValue = v.totalValue;
                bestSel = v.selection;
            }
            v.bound = boundBB(v, (int)items.size(), capacity, items);
            if (v.bound > (double)maxValue)
                pq.push(v);

            // Рассматриваем вариант без выбора предмета u.level
            v.level = u.level + 1;
            v.totalWeight = u.totalWeight;
            v.totalValue = u.totalValue;
            v.selection = u.selection;
            v.selection[u.level] = 0;

            v.bound = boundBB(v, (int)items.size(), capacity, items);
            if (v.bound > (double)maxValue)
                pq.push(v);
        }
    }

    BBResult res;
    res.bestValue = maxValue;
    res.bestSelection = bestSel; // в отсортированном порядке
    return res;
}

// --------------------------- MAIN --------------------------- //

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    // Инициализируем генератор случайных чисел один раз
    srand((unsigned)time(nullptr));

    std::string inputFile = argv[1];

    // Считывание данных из файла
    std::ifstream fin(inputFile);
    if (!fin.is_open()) {
        std::cerr << "Не удалось открыть файл: " << inputFile << std::endl;
        return 1;
    }

    int N, CAP;
    fin >> N >> CAP;
    std::vector<Item> items(N);
    for (int i = 0; i < N; i++) {
        fin >> items[i].weight >> items[i].value;
    }
    fin.close();

    // =============== 1) Метод ветвей и границ ===============
    long long startBB = currentTimeMillis();
    BBResult bbRes = branchAndBoundKnapsack(items, CAP);
    long long endBB = currentTimeMillis();
    long long timeBB = endBB - startBB; // миллисекунды
    int bestValBB = bbRes.bestValue;

    // Вычисляем суммарный вес для решения BnB
    int totalWBB = 0;
    {
        std::vector<Item> sortedItems = items;
        std::sort(sortedItems.begin(), sortedItems.end(), [](const Item &a, const Item &b){
            return (double)a.value/a.weight > (double)b.value/b.weight;
        });
        for (size_t i = 0; i < bbRes.bestSelection.size(); i++) {
            if (bbRes.bestSelection[i] == 1) {
                totalWBB += sortedItems[i].weight;
            }
        }
    }

    // =============== 2) Генетический алгоритм ===============
    long long startGA = currentTimeMillis();
    GAResult gaRes = geneticKnapsack(items, CAP);
    long long endGA = currentTimeMillis();
    long long timeGA = endGA - startGA; // миллисекунды
    int bestValGA = gaRes.bestValue;

    // Вычисляем суммарный вес для решения GA
    int totalWGA = 0;
    for (int i = 0; i < N; i++) {
        if (gaRes.bestChromosome[i] == 1) {
            totalWGA += items[i].weight;
        }
    }

    // Вывод результатов в консоль
    std::cout << "File: " << inputFile << "\n";
    std::cout << "  [BnB]   Value=" << bestValBB << ", Weight=" << totalWBB 
              << ", Time=" << timeBB << " ms\n";
    std::cout << "  [GenGA] Value=" << bestValGA << ", Weight=" << totalWGA
              << ", Time=" << timeGA << " ms\n";

    // Запись в общий CSV-файл results.csv (добавляем строку)
    {
        std::ofstream fout("results_BnB_GA.csv", std::ios::app);
        if (fout.tellp() == 0) {
            fout << "File,weight_BnB,time_BnB_ms,weight_GA,time_GA_ms\n";
        }
        fout << inputFile << ","
             << totalWBB << ","
             << timeBB << ","
             << totalWGA << ","
             << timeGA << "\n";
    }

    // Вывод результатов каждого метода в отдельный CSV-файл (имя файла основано на inputFile)
    {
        if (!fs::exists("result_BnB_GA")) {
            if (!fs::create_directory("result_BnB_GA")) {
                std::cerr << "Не удалось создать директорию 'result'" << std::endl;
                return 1;
            }
        }
    
        // Извлекаем только имя файла из пути inputFile
        fs::path p(inputFile);
        std::string filename = p.filename().string();  // например, "ks_100_1" или "ks_100_1.txt"
        // Удаляем расширение, если оно есть:
        filename = fs::path(filename).stem().string();
    
        // Формируем путь для сохранения результатов
        std::string outName = "result_BnB_GA/" + filename + ".csv";

        std::ofstream fout(outName, std::ios::app);

        // 1) Branch & Bound: вывод весов выбранных предметов
        fout << "BnB,";
        {
            std::vector<Item> sortedItems = items;
            std::sort(sortedItems.begin(), sortedItems.end(), [](const Item &a, const Item &b){
                return (double)a.value/a.weight > (double)b.value/b.weight;
            });

            for (size_t i = 0; i < bbRes.bestSelection.size(); i++) {
                if (bbRes.bestSelection[i] == 1) {
                    fout << sortedItems[i].weight;
                } else {
                    fout << 0;
                }
                if (i < bbRes.bestSelection.size() - 1) {
                    fout << ",";
                }
            }
        }
        fout << "\n";

        // 2) Genetic Algorithm: вывод весов выбранных предметов
        fout << "GA,";
        for (int i = 0; i < N; i++) {
            if (gaRes.bestChromosome[i] == 1) {
                fout << items[i].weight;
            } else {
                fout << 0;
            }
            if (i < N - 1) {
                fout << ",";
            }
        }
        fout << "\n";
    }

    return 0;
}
