@echo off
echo filename,dist_2opt,time_2opt_ms,dist_3opt,time_3opt_ms > results.csv

for %%f in (data\*) do (
    echo Running on %%f
    rem Передаем первым аргументом имя файла для CSV-результатов, вторым — имя файла для маршрута
    main.exe %%~nxf %%~nxf_route.txt < %%f >> results.csv
)

echo Done!
pause
