@echo off

chcp 65001 && g++ knapsack_solvers.cpp -static -static-libgcc -static-libstdc++ -std=c++17 -o knapsack_solvers.exe

for %%F in (data_BnB_GA\*) do (
    echo Запуск для файла %%F
    knapsack_solvers.exe %%F
    echo.
)


pause
