# ExprParse

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
    e.RegisterVariable("x", x);
    e.RegisterVariable("y", y);



    // Parse expression
    exprparse::Status status;
    status = e.Parse("1 + 1 / (x + y)");

    if(status != exprparse::Success)
        std::cout << "Failed to parse expression!" << std::endl;



    // Set variables
    exprparse::SetVariable(x, 2.0);
    exprparse::SetVariable(y, 1.0);



    // Evaluate expression
    double z = e.Eval(status);

    if(status != exprparse::Success)
        std::cout << "Failed to evaluate expression!" << std::endl;

    std::cout << z << std::endl;

    return 0;

}
```