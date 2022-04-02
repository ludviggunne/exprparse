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

int main() {

    exprparse::Expression<double> e;


    // Register variables
    auto x = exprparse::CreateVariable<double>(0);
    auto y = exprparse::CreateVariable<double>(0);
    auto z = exprparse::CreateVariable<double>(0);

    e.RegisterVariable("x", x);
    e.RegisterVariable("y", y);
    e.RegisterVariable("z", z);



    // Parse expression
    exprparse::Status status;
    status = e.Parse("-(2 * x + y) / z");

    if(status != exprparse::Success)
    {
        std::cout << "Couldn't parse expression string" << std::endl;
        return -1;
    }



    // Set variables
    exprparse::SetVariable(x, 3.0);
    exprparse::SetVariable(y, 4.0);
    exprparse::SetVariable(z, 2.0);


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