example_parsing: example_parsing.cpp parser.hpp
	g++ -Wall -Wextra -std=c++17 $< -o $@
