#!/bin/bash

# Компиляция
echo "=== КОМПИЛЯЦИЯ ==="
g++ -std=c++17 -O2 -Wall -o interpreter interpreter.cpp
if [ $? -ne 0 ]; then
    echo "Ошибка компиляции"
    exit 1
fi

# Запуск тестов
echo "=== ЗАПУСК ТЕСТОВ ==="
echo "Результаты: $(date)" > test_results.log

for test in test_a*.ml test_b*.ml; do
    echo "=== $test ===" >> test_results.log
    ./interpreter "$test" >> test_results.log 2>&1
    echo "Exit code: $?" >> test_results.log
    echo "" >> test_results.log
done

echo "Готово. Результаты в test_results.log"