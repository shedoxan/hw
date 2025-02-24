#include <bits/stdc++.h>
#include <chrono>
#include <fstream>  
using namespace std;

// Функция вычисления евклидовой дистанции между двумя точками
inline double distEuclid(const pair<double,double> &a, const pair<double,double> &b){
    double dx = a.first - b.first;
    double dy = a.second - b.second;
    return sqrt(dx*dx + dy*dy);
}

// Функция вычисления длины маршрута
// route - последовательность индексов вершин
// coords - координаты вершин
// isCycle - true, если считаем цикл (возврат в начальную вершину)
double routeLength(const vector<int> &route,
                   const vector<pair<double,double>> &coords,
                   bool isCycle = true)
{
    double length = 0.0;
    for(int i = 0; i < (int)route.size() - 1; i++){
        length += distEuclid(coords[route[i]], coords[route[i+1]]);
    }
    if(isCycle){
        // замыкаем цикл
        length += distEuclid(coords[route.back()], coords[route.front()]);
    }
    return length;
}

// ------------------- 2-opt -------------------

// Одна «итерация» 2-opt (перебор всех (i, j) с разворотом подотрезка).
// Возвращает true, если было найдено улучшение.
bool twoOptIteration(vector<int> &route, const vector<pair<double,double>> &coords, bool isCycle){
    bool improved = false;
    int n = (int)route.size();
    double bestDist = routeLength(route, coords, isCycle);

    // Перебираем все пары (i, j)
    for(int i = 1; i < n - 2; i++){
        for(int j = i + 1; j < n - (isCycle ? 0 : 1); j++){
            // Реверсируем подотрезок [i, j)
            reverse(route.begin() + i, route.begin() + j);
            double newDist = routeLength(route, coords, isCycle);
            if(newDist < bestDist){
                bestDist = newDist;
                improved = true;
            } else {
                // Откатываем, если улучшений нет
                reverse(route.begin() + i, route.begin() + j);
            }
        }
    }
    return improved;
}

// Запуск 2-opt до тех пор, пока есть улучшения.
// Возвращает (расстояние, время_в_миллисекундах).
pair<double, long long> run2Opt(vector<int> &route, const vector<pair<double,double>> &coords, bool isCycle){
    using namespace std::chrono;
    auto start = high_resolution_clock::now();
    while(true){
        bool improved = twoOptIteration(route, coords, isCycle);
        if(!improved) break;
    }
    auto end = high_resolution_clock::now();
    long long elapsed_ms = duration_cast<milliseconds>(end - start).count();
    double dist = routeLength(route, coords, isCycle);
    return make_pair(dist, elapsed_ms);
}

// ------------------- 3-opt -------------------

// Одна «итерация» 3-opt (перебор троек (i, j, k), ревёрсы подотрезков).
// Возвращает true, если нашлось улучшение.
bool threeOptIteration(vector<int> &route, const vector<pair<double,double>> &coords, bool isCycle){
    bool improved = false;
    int n = (int)route.size();
    double bestDist = routeLength(route, coords, isCycle);

    for(int i = 0; i < n - 2; i++){
        for(int j = i + 1; j < n - 1; j++){
            for(int k = j + 1; k < n; k++){
                // Сохраним исходную последовательность
                vector<int> current = route;

                // Несколько вариантов 3-opt
                auto attemptRebuild = [&](int mode){
                    if(mode == 0){
                        reverse(current.begin() + i + 1, current.begin() + j + 1);
                    } else if(mode == 1){
                        reverse(current.begin() + j + 1, current.begin() + k + 1);
                    } else if(mode == 2){
                        reverse(current.begin() + i + 1, current.begin() + j + 1);
                        reverse(current.begin() + j + 1, current.begin() + k + 1);
                    } else if(mode == 3){
                        reverse(current.begin() + i + 1, current.begin() + k + 1);
                    } else if(mode == 4){
                        reverse(current.begin() + i + 1, current.begin() + j + 1);
                        reverse(current.begin() + i + 1, current.begin() + k + 1);
                    } else if(mode == 5){
                        reverse(current.begin() + j + 1, current.begin() + k + 1);
                        reverse(current.begin() + i + 1, current.begin() + k + 1);
                    } else if(mode == 6){
                        reverse(current.begin() + i + 1, current.begin() + j + 1);
                        reverse(current.begin() + j + 1, current.begin() + k + 1);
                        reverse(current.begin() + i + 1, current.begin() + k + 1);
                    }
                };

                double bestLocal = bestDist;
                vector<int> bestCurrent = route;

                // Перебираем 7 вариантов перестройки
                for(int mode = 0; mode < 7; mode++){
                    current = route;
                    attemptRebuild(mode);
                    double newDist = routeLength(current, coords, isCycle);
                    if(newDist < bestLocal){
                        bestLocal = newDist;
                        bestCurrent = current;
                    }
                }

                if(bestLocal < bestDist){
                    route = bestCurrent;
                    bestDist = bestLocal;
                    improved = true;
                    // Выходим, чтобы перезапустить цикл 3-opt заново
                    return true;
                }
            }
        }
    }
    return improved;
}

// Запуск 3-opt до тех пор, пока есть улучшения.
// Возвращает (расстояние, время_в_миллисекундах).
pair<double, long long> run3Opt(vector<int> &route, const vector<pair<double,double>> &coords, bool isCycle){
    using namespace std::chrono;
    auto start = high_resolution_clock::now();
    while(true){
        bool improved = threeOptIteration(route, coords, isCycle);
        if(!improved) break;
    }
    auto end = high_resolution_clock::now();
    long long elapsed_ms = duration_cast<milliseconds>(end - start).count();
    double dist = routeLength(route, coords, isCycle);
    return make_pair(dist, elapsed_ms);
}

// ------------------- main -------------------
int main(int argc, char* argv[]){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Считываем количество вершин
    int N;
    cin >> N;
    vector<pair<double,double>> coords(N);
    for(int i = 0; i < N; i++){
        double x, y;
        cin >> x >> y;
        coords[i] = {x, y};
    }

    // Начальный маршрут: 0..N-1
    vector<int> route(N);
    iota(route.begin(), route.end(), 0);

    bool isCycle = true; // Если хотим тур (замкнутый), иначе false.

    // 1) Отдельно прогоняем 2-opt (до локального минимума)
    auto [dist2Opt, time2Opt] = run2Opt(route, coords, isCycle);

    // 2) После этого запускаем 3-opt (до локального минимума)
    auto [dist3Opt, time3Opt] = run3Opt(route, coords, isCycle);

    // Имя файла для CSV (если передали параметром)
    string csvFilename = (argc > 1) ? argv[1] : "-";

    // Выведем 1 строку CSV:
    // csvFilename, dist_2opt, time_2opt(ms), dist_3opt, time_3opt(ms)
    cout << csvFilename << ","
         << fixed << setprecision(6) << dist2Opt << ","
         << time2Opt << ","
         << dist3Opt << ","
         << time3Opt << "\n";

    // Сохранение итогового маршрута в отдельный файл.
    // Если передан второй параметр, используем его как имя файла, иначе "route.txt".
    string routeFilename = (argc > 2) ? argv[2] : "route.txt";
    ofstream routeFile(routeFilename);
    if(routeFile.is_open()){
        for(int i = 0; i < N; i++){
            // Выводим вершины с 1 (можно изменить, если нужна индексация с 0)
            routeFile << route[i] + 1 << (i + 1 < N ? ' ' : '\n');
        }
        routeFile.close();
    } else {
        cerr << "Ошибка при открытии файла для записи маршрута." << endl;
    }

    return 0;
}
