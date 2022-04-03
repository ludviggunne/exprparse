#ifndef _exprparse_h_
#define _exprparse_h_

#include <map>          // std::map
#include <string>       // std::string
#include <memory>       // std::shared_ptr
#include <sstream>      // std::stringstream
#include <algorithm>    // std::remove
#include <type_traits>  // std::is_floating_point
#include <functional>   // std::function

#ifdef EP_DEBUG
#include <iostream>     // std::cout
#define EP_LOG(x) std::cout << x << "\n\n";
#else
#define EP_LOG(x)
#endif

namespace exprparse {

    enum Status {

        Success,

        Error_Variable_Already_Registered,
        Error_Function_Already_Registered,
        Error_Variable_Function_Name_Clash,

        Error_Not_Compiled,
        Error_Division_By_Zero,

        Error_Unregistered_Symbol,
        Error_Syntax_Error,

        Error_Unknown

    };


    namespace _internal {

        template<typename T>
        class Node {
        public:
            virtual T Eval(Status &status) const = 0;
        };



        template<typename T>
        class OperatorNode : public Node<T> {
        public:
            enum class Operator { Add, Sub, Mul, Div };
        public:
            OperatorNode(Operator op) : _operator(op) {}

            virtual T Eval(Status &status) const override
            {

                T leftValue  = _left->Eval(status);
                T rightValue = _right->Eval(status);

                switch (_operator) {
                    
                    // TODO?: Use derived classes for each operator instead of enum + switch
                    case Operator::Add:
                        return leftValue + rightValue;

                    case Operator::Sub:
                        return leftValue - rightValue;

                    case Operator::Mul:
                        return leftValue * rightValue;

                    case Operator::Div:
                        if(rightValue == T(0))
                        {
                            status = Error_Division_By_Zero;
                            return T(0);
                        }
                        return leftValue / rightValue;

                    default:
                        status = Error_Unknown; // What the hell ?
                        return T(0);
                }
            }

            void LinkLeft(const std::shared_ptr<Node>  &left)  { _left  = left; }
            void LinkRight(const std::shared_ptr<Node> &right) { _right = right; }

        private:
            Operator _operator;

            std::shared_ptr<Node> _left;
            std::shared_ptr<Node> _right;
        };



        template<typename T>
        class VariableNode : public Node<T> {
        public:
            VariableNode(const std::shared_ptr<T> &value) : _value(value) {}

            virtual T Eval(Status &status) const override { return *_value; }

        private:
            std::shared_ptr<T> _value;
        };



        template<typename T>
        class ConstantNode : public Node<T> {
        public:
            ConstantNode(T value) : _value(value) {}

            virtual T Eval(Status &status) const override { return _value; }

        private:
            const T _value;
        };



        template<typename T>
        class FunctionNode : public Node<T> {
        public:
            FunctionNode(const std::function<T(T)> &function) : _function(function) {}

            virtual T Eval(Status &status) const override { return _function(_argument->Eval(status)); }

            void LinkArgument(const std::shared_ptr<Node<T>> &arg) { _argument = arg; }

        private:
            std::function<T(T)> _function;
            std::shared_ptr<Node<T>> _argument;
        };
    }



    template<typename T>
    class Expression {
        
        static_assert(std::is_floating_point<T>::value, "T is not floating point type");

    public:
        Status RegisterVariable(const std::string &name, const std::shared_ptr<T> &variable)
        {
            EP_LOG("Registered variable " << name);

            // Check for function with same name
            auto it = _functions.find(name);
            if (it != _functions.end())
                return Error_Variable_Function_Name_Clash;

            auto pair = _symbols.try_emplace(name, value);
            return pair.second ? Success : Error_Variable_Already_Registered;
            //          ^^^^^^ --- False if key-value pair already exists
        }

        Status RegisterFunction(const std::string &name, const std::function<T(T)> &function)
        {
            EP_LOG("Registering function " << name);

            // Check for variable with same name
            auto it = _symbols.find(name);
            if (it != _symbols.end())
                return Error_Variable_Function_Name_Clash;

            // Try inserting new function
            auto pair = _functions.try_emplace(name, function);
            return pair.second ? Success : Error_Function_Already_Registered;
            //          ^^^^^^ --- False if key-value pair already exists
        }

        T Eval(Status &status) const
        {
            EP_LOG("Evaluating expression");

            if (!_base) { // The AST is empty
                status = Error_Not_Compiled;
                return T(0);
            } 

            status = Success;
            return _base->Eval(status);
        }

        Status Parse(std::string expr_string);

    private:
        std::shared_ptr<_internal::Node<T>> ParseSubString(
            std::string::const_iterator begin, std::string::const_iterator end, Status &status);

    private:
        std::map<std::string, std::shared_ptr<T>> _symbols;
        std::map<std::string, std::function<T(T)>> _functions;

        std::shared_ptr<_internal::Node<T>> _base;
    };



    template<typename T>
    Status Expression<T>::Parse(std::string expr_string)
    {
        EP_LOG("Parsing expression");

        // Remove blank spaces
        auto new_end = std::remove(expr_string.begin(), expr_string.end(), ' ');

        // Parse
        Status status = Success;
        _base = ParseSubString(expr_string.begin(), new_end, status);

        if (status != Success) // Clear the AST since it's invalid
            _base.reset();

        return status;
    }



#define _exprparse_parse_error(error) { status = error; return nullptr; }
    template<typename T>
    std::shared_ptr<_internal::Node<T>> Expression<T>::ParseSubString(
        std::string::const_iterator begin, std::string::const_iterator end, Status &status)
    {
        EP_LOG(" Parsing sub-expression '" << std::string(begin, end) << "'");

        auto it                   = begin;
        int  bracket_depth        = 0;     
        char operator_symbol      = '\0';
        bool found_plus_or_minus  = false; // +/- has precedence over *// when constructing tree

        while (it != end) {
            
            if (*it == '(') bracket_depth++;
            if (*it == ')') bracket_depth--;

            if (bracket_depth == 0) { // Only look for operator if outside paranthesis

                if (*it == '+' || *it == '-')
                {
                    found_plus_or_minus = true;
                    operator_symbol     = *it;

                    EP_LOG("  Found operator " << *it);
                    break;
                }
            }

            it++;
        }

        if (bracket_depth != 0) // Start- and end brackets should cancel out
            _exprparse_parse_error(Error_Syntax_Error);

        if (!found_plus_or_minus) { // Only pick * or / operators if + or - aren't found
            
            it = begin;
            while (it != end) {
            
                if (*it == '(') bracket_depth++;
                if (*it == ')') bracket_depth--;

                if (bracket_depth == 0) { // Only look for operator if outside paranthesis

                    if (*it == '*' || *it == '/')
                    {
                        operator_symbol = *it;

                        EP_LOG("  Found operator " << *it);
                        break;
                    }
                }

                it++;
            }
        }

        if (operator_symbol != '\0') {

            using namespace _internal;

            std::shared_ptr<OperatorNode<T>> node;

            switch (operator_symbol) {
                case '+':
                    node = std::make_shared<OperatorNode<T>>(OperatorNode<T>::Operator::Add);
                    EP_LOG("   Appending Addition node");
                    break;

                case '-':
                    node = std::make_shared<OperatorNode<T>>(OperatorNode<T>::Operator::Sub);
                    EP_LOG("   Appending Subtraction node");
                    break;

                case '*':
                    node = std::make_shared<OperatorNode<T>>(OperatorNode<T>::Operator::Mul);
                    EP_LOG("   Appending Multiplication node");
                    break;

                case '/':
                    node = std::make_shared<OperatorNode<T>>(OperatorNode<T>::Operator::Div);
                    EP_LOG("   Appending Division node");
                    break;

                default:
                    _exprparse_parse_error(Error_Unknown); // What the hell ?
                    break;
            }

            if (begin != it) // Right hand side is non-empty
            {
                node->LinkLeft( ParseSubString(begin, it, status));
            }
            else if (operator_symbol == '-') // Negative sign (-x becomes 0 - x)
            {
                EP_LOG("   Negating sub-expression")
                node->LinkLeft(std::make_shared<_internal::ConstantNode<T>>(T(0)));
            }
            else // RH is empty but operator is not minus
            {
                _exprparse_parse_error(Error_Syntax_Error);
            }

            node->LinkRight(ParseSubString(it + 1, end,  status));
            //                             ^^^^^^ --- plus one to omit operator        

            if (status == Success)
                return node;
            else
                return nullptr;

        } else {
            
            // Strip potential brackets
            if (*begin == '(' && *(end - 1) == ')')
            {
                EP_LOG("  Stripping brackets");

                begin++; 
                end--;

                if (begin == end) // Empty bracket
                    return std::make_shared<_internal::ConstantNode<T>>(T(0));

                if (begin > end) 
                    // Only happens if substring is odd length 
                    // (meaning start- and end brackets aren't one-to-one)
                    _exprparse_parse_error(Error_Syntax_Error);

                return ParseSubString(begin, end, status);
            }

            // Check for constant
            std::stringstream ss;
            ss << std::string(begin, end);
            T value;
            ss >> value; // Try converting to T (double / float)

            if (!ss.fail()) {
                EP_LOG("   Appending constant node: " << value);

                return std::make_shared<_internal::ConstantNode<T>>(value);
            } 
            else 
            {
                // Look for variable
                auto v_it = _symbols.find(std::string(begin, end));

                if (v_it != _symbols.end())
                {
                    EP_LOG("   Appending variable node: " << v_it->first);

                    return std::make_shared<_internal::VariableNode<T>>(v_it->second);
                    //                                           SYMBOL --- ^^^^^^
                }

                // Look for function
                auto func_end = begin;

                // Sub expression must end with left bracket
                if(*(end - 1) != ')')
                    _exprparse_parse_error(Error_Syntax_Error);

                // Extract function name before parenthesis
                while (func_end != end && *func_end != '(')
                    func_end++;
                if (*func_end != '(') // Function has no arguments
                    _exprparse_parse_error(Error_Syntax_Error);

                
                auto f_it = _functions.find(std::string(begin, func_end));
                if (f_it != _functions.end())
                {
                    EP_LOG("   Appending function node: " << f_it->first);

                    auto node = std::make_shared<_internal::FunctionNode<T>>(f_it->second);

                    // Evaluate argument
                    auto arg = ParseSubString(func_end + 1, end - 1, status);
                    if (status != Success)
                        return nullptr;

                    node->LinkArgument(arg);
                    return node;

                } else {
                    _exprparse_parse_error(Error_Unregistered_Symbol);
                }
            }

        }
    }
}

#endif // _exprparse_h_