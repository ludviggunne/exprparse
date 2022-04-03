# ExprParse

Single header template library for parsing and evaluating math expressions.  
Not very effective.

## Integration
### CMake
#### CMakeLists.txt:  
`add_subdirectory(exprparse)`  
`target_link_libraries(YOUR_TARGET exprparse)`  
#### Source file:  
`#include "exprparse.h"`  

## Usage

#### Example program:  

`main.cpp:`  

```C++
#include "exprparse.hpp"
#include <iostream>

double square(double x)
{
    return x * x;
}

int main() {

    exprparse::Expression<double> e;


    // Register variables
    auto x = std::make_shared<double>(0);
    auto y = std::make_shared<double>(0);

    e.RegisterVariable("x", x);
    e.RegisterVariable("y", y);

    e.RegisterFunction("square", square);



    // Parse expression
    exprparse::Status status;
    status = e.Parse("x + square(y)");

    if(status != exprparse::Success)
    {
        std::cout << "Couldn't parse expression string" << std::endl;
        return -1;
    }



    // Set variables
    *x = 1;
    *y = 3;


    // Evaluate expression
    double u = e.Eval(status);

    if(status != exprparse::Success)
    {
        std::cout << "Couldn't evaluate expression" << std::endl;
        return -1;
    }

    std::cout << "Expression evaluated to: " << u << std::endl;

    return 0;

}
```