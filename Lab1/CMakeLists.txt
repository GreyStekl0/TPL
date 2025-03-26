cmake_minimum_required(VERSION 3.10)
project(Lab1)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Создаем исполняемый файл для 1.cpp
add_executable(Lab1.1 1.cpp)

# Создаем исполняемый файл для 2.cpp
add_executable(Lab1.2 2.cpp)