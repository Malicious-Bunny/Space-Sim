#include "application.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
	Application app;

	try {
		app.Start();
	} catch(const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}