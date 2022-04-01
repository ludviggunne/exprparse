#ifndef _exprparse_h_
#define _exprparse_h_

#include <map>
#include <string>
#include <memory>
#include <vector>

namespace exprparse {

    enum Status {

        Success,

        Error_Variable_Already_Registered,
        Error_Not_Compiled,
        Error_Division_By_Zero,

        Error_Unregistered_Symbol,
        Error_Syntax_Error

    };


    // TODO: Custom ref-counted class ???
    template<typename T>
    using Variable = std::shared_ptr<T>;

    template<typename T>
    Variable<T> CreateVariable(T value) { return std::make_shared<T>(value); }

    template<typename T>
    void SetVariable(Variable<T> &var, T value) { *var = value; }

    namespace internal {

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

                T leftValue  = _left->Eval();
                T rightValue = _right->Eval();

                switch(_operator) {

                    case Add:
                        return leftValue + rightValue;

                    case Sub:
                        return leftValue - rightValue;

                    case Mul:
                        return leftValue * rightValue;

                    case Div:
                        if(rightValue == T(0))
                        {
                            status = Error_Division_By_Zero;
                            return T(0);
                        }
                        return leftValue / rightValue;
                }

                return T(0);
            }

            void LinkLeft(std::shared_ptr<Node>  &&left)  { _left  = left; }
            void LinkRight(std::shared_ptr<Node> &&right) { _right = right; }

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

    public:
        Status RegisterVariable(const std::string &name, const Variable<T> &value)
        {
            auto pair = _symbols.try_emplace(name, value);
            return pair.second ? Success : Error_Variable_Already_Registered;
            //          ^^^^^^ --- False if key-value pair already exists
        }

        T Eval(Status &status) const
        {
            if(!_base) {
                status = Error_Not_Compiled;
                return T(0);
            } 

            status = Success;
            return _base->Eval(status);
        }

        Status Parse(const std::string &expr_string);

    private:
        std::shared_ptr<internal::Node<T>> ParseSubString(
            std::string::const_iterator begin, std::string::const_iterator end, Status &status);

    private:
        std::map<std::string, std::shared_ptr<T>> _symbols;
        std::shared_ptr<internal::Node<T>> _base;
    };



    template<typename T>
    Status Expression<T>::Parse(const std::string &expr_string)
    {
        Status status = Success;
        _base = ParseSubString(expr_string.begin(), expr_string.end(), status);

        _symbols.clear();
        return status;
    }
    
    template<typename T>
    std::shared_ptr<internal::Node<T>> Expression<T>::ParseSubString(
        std::string::const_iterator begin, std::string::const_iterator end, Status &status)
    {
        // TODO: Implementation

        // --- Dummy implementation for early testing ---
        return std::make_shared<internal::ConstantNode<T>>( 10.0 );
        // ----------------------------------------------
    }
}

#endif // _exprparse_h_