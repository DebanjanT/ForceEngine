#include "Force/Editor/UndoRedo.h"
#include "Force/Core/Logger.h"
#include <algorithm>

namespace Force
{
    static const std::string s_Empty = "";

    UndoRedoStack& UndoRedoStack::Get()
    {
        static UndoRedoStack instance;
        return instance;
    }

    void UndoRedoStack::Execute(std::unique_ptr<ICommand> cmd)
    {
        // Truncate redo history
        if (m_Head < (i32)m_Stack.size() - 1)
            m_Stack.erase(m_Stack.begin() + m_Head + 1, m_Stack.end());

        cmd->Execute();

        m_Stack.push_back(std::move(cmd));

        // Trim oldest if over limit
        if (m_Stack.size() > MAX_HISTORY)
        {
            m_Stack.erase(m_Stack.begin());
            m_Head = (i32)m_Stack.size() - 1;
        }
        else
        {
            m_Head = (i32)m_Stack.size() - 1;
        }
    }

    void UndoRedoStack::Undo()
    {
        if (!CanUndo()) return;
        FORCE_CORE_TRACE("UndoRedo: Undo '{}'", m_Stack[m_Head]->GetName());
        m_Stack[m_Head]->Undo();
        m_Head--;
    }

    void UndoRedoStack::Redo()
    {
        if (!CanRedo()) return;
        m_Head++;
        FORCE_CORE_TRACE("UndoRedo: Redo '{}'", m_Stack[m_Head]->GetName());
        m_Stack[m_Head]->Execute();
    }

    bool UndoRedoStack::CanUndo() const { return m_Head >= 0; }
    bool UndoRedoStack::CanRedo() const { return m_Head < (i32)m_Stack.size() - 1; }

    const std::string& UndoRedoStack::GetUndoName() const
    {
        return CanUndo() ? m_Stack[m_Head]->GetName() : s_Empty;
    }

    const std::string& UndoRedoStack::GetRedoName() const
    {
        return CanRedo() ? m_Stack[m_Head + 1]->GetName() : s_Empty;
    }

    void UndoRedoStack::Clear()
    {
        m_Stack.clear();
        m_Head = -1;
    }
}
