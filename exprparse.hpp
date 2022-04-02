#ifndef _exprparse_h_
#define _exprparse_h_

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <sstream>
#include <algorithm>
#include <type_traits>

#ifdef EP_DEBUG
#include <iostream>
#define EP_LOG(x) std::cout << x << "\n";
#else
#define EP_LOG(x)
#endif

namespace exprparse {

    enum Status {

        Success,

        Error_Variable_Already_Registered,
        Error_Not_Compiled,
        Error_Division_By_Zero,

        Error_Unregistered_Symbol,
        Error_Syntax_Error,

        Error_Unknown
    };


    // TODO: Custom ref-counted class ???
    template<typename T>
    using Variable = std::shared_ptr<T>;

    template<typename T>
    Variable<T> CreateVariable(T value) { return std::make_shared<T>(value); }

    template<typename T>
    void SetVariable(Variable<T> &var, T value) { *var = value; }

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

            T Eval(Status &status) const override
            {

                T leftValue  = _left->Eval(status);
                T rightValue = _right->Eval(status);

                switch(_operator) {

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

            T Eval(Status &status) const override { return *_value; }

        private:
            std::shared_ptr<T> _value;
        };



        template<typename T>
        class ConstantNode : public Node<T> {
        public:
            ConstantNode(T value) : _value(value) {}

            T Eval(Status &status) const override { return _value; }

        private:
            const T _value;
        };

    }


    template<typename T>
    class Expression {
        static_assert(std::is_floating_point<T>::value, "T is not floating point type");

    public:
        Status RegisterVariable(const std::string &name, const Variable<T> &value)
        {
            EP_LOG("Registered variable " << name);

            auto pair = _symbols.try_emplace(name, value);
            return pair.second ? Success : Error_Variable_Already_Registered;
            //          ^^^^^^ --- False if key-value pair already exists
        }

        T Eval(Status &status) const
        {
            EP_LOG("Evaluating expression");

            if(!_base) {
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
        std::shared_ptr<_internal::Node<T>> _base;
    };



    template<typename T>
    Status Expression<T>::Parse(std::string expr_string)
    {
        EP_LOG("Parsing expression");

        auto new_end = std::remove(expr_string.begin(), expr_string.end(), ' ');

        Status status = Success;
        _base = ParseSubString(expr_string.begin(), new_end, status);

        if(status == Success)
            _symbols.clear(); // Successfully parsed, clear symbol table
        else
            _base.reset();    // Don't clear symbol table, we might try again

        return status;
    }



#define _exprparse_parse_error(error) { status = error; return nullptr; }
    template<typename T>
    std::shared_ptr<_internal::Node<T>> Expression<T>::ParseSubString(
        std::string::const_iterator begin, std::string::const_iterator end, Status &status)
    {
        EP_LOG(" Parsing sub-expression " << std::string(begin, end));

        auto it = begin;
        int  bracket_depth        = 0;
        char operator_symbol      = '\0';
        bool found_plus_or_minus  = false;

        while(it != end) {
            
            if(*it == '(') bracket_depth++;
            if(*it == ')') bracket_depth--;

            if(bracket_depth == 0) { // Only look for operator if outside paranthesis

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

        if(bracket_depth != 0)
            _exprparse_parse_error(Error_Syntax_Error);

        if(!found_plus_or_minus) {

            it = begin;
            while(it != end) {
            
                if(*it == '(') bracket_depth++;
                if(*it == ')') bracket_depth--;

                if(bracket_depth == 0) { // Only look for operator if outside paranthesis

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

        if(operator_symbol != '\0') {

            using namespace _internal;

            std::shared_ptr<OperatorNode<T>> node;

            switch(operator_symbol) {
                case '+':
                    node = std::make_shared<OperatorNode<T>>(OperatorNode<T>::Operator::Add);
                    EP_LOG("   Appending ADD node");
                    break;

                case '-':
                    node = std::make_shared<OperatorNode<T>>(OperatorNode<T>::Operator::Sub);
                    EP_LOG("   Appending SUB node");
                    break;

                case '*':
                    node = std::make_shared<OperatorNode<T>>(OperatorNode<T>::Operator::Mul);
                    EP_LOG("   Appending MUL node");
                    break;

                case '/':
                    node = std::make_shared<OperatorNode<T>>(OperatorNode<T>::Operator::Div);
                    EP_LOG("   Appending DIV node");
                    break;

                default:
                    _exprparse_parse_error(Error_Unknown); // What the hell ?
                    break;
            }

            node->LinkLeft( ParseSubString(begin,  it,   status));
            node->LinkRight(ParseSubString(it + 1, end,  status));
            //                             ^^^^^^ --- plus one to ommit operator        

            return node;

        } else {
            
            // Strip brackets
            if(*begin == '(' && *(end - 1) == ')')
            {
                EP_LOG("  Stripping brackets");

                begin++; 
                end--;

                if(begin == end) // Empty bracket
                    return std::make_shared<_internal::ConstantNode<T>>(T(0));

                if(begin > end) // Syntax error
                    _exprparse_parse_error(Error_Syntax_Error);

                return ParseSubString(begin, end, status);
            }

            // Check for constant
            std::stringstream ss;
            ss << std::string(begin, end);
            T value;
            ss >> value;

            if(!ss.fail()) {
                EP_LOG("   Appending constant node: " << value);

                return std::make_shared<_internal::ConstantNode<T>>(value);
            } 
            else 
            {
                // Look for variable
                auto it = _symbols.find(std::string(begin, end));

                if(it == _symbols.end())
                {
                    _exprparse_parse_error(Error_Syntax_Error);
                } 
                else 
                {
                    EP_LOG("   Appending variable node: " << it->first);

                    return std::make_shared<_internal::VariableNode<T>>(it->second);
                    //                                           SYMBOL --- ^^^^^^
                }
            }

        }
    }
}

#endif // _exprparse_h_