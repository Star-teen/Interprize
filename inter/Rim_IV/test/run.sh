#!/bin/bash

echo " Compilation"
g++ -std=c++17 -o rim main.cpp
if [ $? -ne 0 ]; then
    echo "Error of compilation"
    exit 1
fi

# Запуск тестов
echo " TEST PART"
echo "Result : $(date)" > test_results.log

for test in test_a*.ml test_b*.ml; do
    echo "=== $test ===" >> test_results.log
    ./interpreter "$test" >> test_results.log 2>&1
    echo "Exit code: $?" >> test_results.log
    echo "" >> test_results.log
done

echo "Well DONE. All results in test_results.log"