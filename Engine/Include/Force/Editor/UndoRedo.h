#pragma once
#include "Force/Core/Base.h"
#include <memory>
#include <vector>
#include <string>

namespace Force
{
    class ICommand
    {
    public:
        virtual ~ICommand() = default;
        virtual void Execute() = 0;
        virtual void Undo()    = 0;
        virtual std::string GetName() const = 0;
    };

    class UndoRedoStack
    {
    public:
        static UndoRedoStack& Get();

        // Execute a command and push it onto the stack (clears redo history)
        void Execute(std::unique_ptr<ICommand> cmd);

        void Undo();
        void Redo();

        bool CanUndo() const;
        bool CanRedo() const;

        const std::string& GetUndoName() const;
        const std::string& GetRedoName() const;

        void Clear();
        u32  GetHistorySize() const { return static_cast<u32>(m_Stack.size()); }

    private:
        UndoRedoStack() = default;

        static constexpr u32 MAX_HISTORY = 200;

        std::vector<std::unique_ptr<ICommand>> m_Stack;
        i32                                    m_Head = -1; // index of last executed command
    };

    // -------------------------------------------------------------------------
    // Convenience: lambda-based command for one-off reversible actions
    class LambdaCommand : public ICommand
    {
    public:
        using Fn = std::function<void()>;

        LambdaCommand(std::string name, Fn execute, Fn undo)
            : m_Name(std::move(name)), m_Execute(std::move(execute)), m_Undo(std::move(undo)) {}

        void Execute()              override { m_Execute(); }
        void Undo()                 override { m_Undo(); }
        std::string GetName() const override { return m_Name; }

    private:
        std::string      m_Name;
        Fn               m_Execute;
        Fn               m_Undo;
    };
}
