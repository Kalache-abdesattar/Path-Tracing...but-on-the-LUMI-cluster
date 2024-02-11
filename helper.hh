#ifndef HELPER_HH
#define HELPER_HH


#include <iostream>
#include <chrono>


std::ostream& operator<<(std::ostream& out, std::chrono::nanoseconds diff)
{
    out << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count(); 
    return out;
}

#endif 