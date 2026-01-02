#ifndef PANCEXPR_HPP
#define PANCEXPR_HPP

#include "pancarray.hpp"
#include "pancstring.hpp"
#include "pancdef.hpp"
#include <cstdint>
#include <cstring>
#include <new>

namespace panc
{

    struct IRVisitor;

    enum class ExprKind : uint8_t
    {
        LITERAL,
        VARIABLE,
        FUNC_CALL
    };

    struct Expr
    {
        ExprKind kind;

        union
        {
            struct
            {
                int32_t value;
            } literal;

            struct
            {
                uint32_t name_idx;
            } variable;

            struct
            {
                uint32_t func_id;
                uint32_t arg_count;
                Expr* args[panc::MAX_FUNC_ARGS];
            } func_call;
        };

        static Expr* createLiteral(int32_t value, void* memory)
        {
            Expr* expr{ new (memory) Expr };
            expr->kind = ExprKind::LITERAL;
            expr->literal.value = value;
            return expr;
        }

        static Expr* createVariable(uint32_t name_idx, void* memory)
        {
            Expr* expr{ new (memory) Expr };
            expr->kind = ExprKind::VARIABLE;
            expr->variable.name_idx = name_idx;
            return expr;
        }

        static Expr* createFuncCall(uint32_t func_id, Expr** args, uint32_t arg_count, void* memory)
        {
            Expr* expr{ new (memory) Expr };
            expr->kind = ExprKind::FUNC_CALL;
            expr->func_call.func_id = func_id;
            expr->func_call.arg_count = arg_count;

            for (uint32_t i{}; i < arg_count && i < panc::MAX_FUNC_ARGS; ++i)
                expr->func_call.args[i] = args[i];

            return expr;
        }

        void accept(IRVisitor& visitor);

        bool isLiteral() const
        {
            return kind == ExprKind::LITERAL;
        }

        bool isVariable() const
        {
            return kind == ExprKind::VARIABLE;
        }

        bool isFuncCall() const
        {
            return kind == ExprKind::FUNC_CALL;
        }

        int32_t getLiteralValue() const
        {
            return literal.value;
        }

        uint32_t getVariableNameIdx() const
        {
            return variable.name_idx;
        }

        uint32_t getFuncId() const
        {
            return func_call.func_id;
        }

        uint32_t getArgCount() const
        {
            return func_call.arg_count;
        }

        Expr* getArg(uint32_t index) const
        {
            return func_call.args[index];
        }
    };

    struct StringTable
    {
        panc::array<char, 4096> buffer{};
        uint32_t offset{};

        uint32_t add(char const* str)
        {
            uint32_t start{ offset };
            uint32_t len{};

            while (str[len] && (offset + len + 1) < buffer.size())
            {
                buffer[offset + len] = str[len];
                ++len;
            }

            if (offset + len < buffer.size())
            {
                buffer[offset + len] = '\0';
                offset += len + 1;
                return start;
            }

            return 0;
        }

        char const* get(uint32_t idx) const
        {
            if (idx < offset)
                return &buffer[idx];
            return nullptr;
        }

        void clear()
        {
            offset = 0;
        }
    };

    struct IRVisitor
    {
        virtual ~IRVisitor() {}

        virtual void visitLiteral(Expr* expr) = 0;
        virtual void visitVariable(Expr* expr) = 0;
        virtual void visitFuncCall(Expr* expr) = 0;
    };

    inline void Expr::accept(IRVisitor& visitor)
    {
        switch (kind)
        {
        case ExprKind::LITERAL:
            visitor.visitLiteral(this);
            break;
        case ExprKind::VARIABLE:
            visitor.visitVariable(this);
            break;
        case ExprKind::FUNC_CALL:
            visitor.visitFuncCall(this);
            break;
        }
    }
}

#endif