#include <iostream>
#include "simulator/simulator.h"

int main() try
{
    Simulator simulator;

    simulator.Run();
        
    return 0;
}
catch (const std::runtime_error& e) {
    std::cerr << "Runtime error: " << e.what() << std::endl;
    return 1;
}
catch (const std::exception& e) {
    std::cerr << "Caught exception: " << e.what() << std::endl;
    return 2; 
}
catch (...) {
    std::cerr << "Caught unknown exception." << std::endl;
    return 3; 
}