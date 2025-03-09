@echo off
echo filename,dist_2opt,time_2opt_ms,dist_3opt,time_3opt_ms > results_2opt_3opt.csv

chcp 65001 && g++ 2opt_3opt.cpp -static -static-libgcc -static-libstdc++ -std=c++17 -o 2_opt_3opt.exe

for %%f in (data_2opt_3opt\*) do (
    echo Running on %%f
    rem Передаем первым аргументом имя файла для CSV-результатов, вторым — имя файла для маршрута
    2_opt_3opt.exe %%~nxf %%~nxf_route.txt < %%f >> results_2opt_3opt.csv
)

echo Done!
pause
