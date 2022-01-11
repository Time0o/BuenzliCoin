#pragma once

namespace bc
{

struct UndoHelper
{
  template<typename FUNC>
  UndoHelper(FUNC &&undo_action)
  : m_undo_action(std::forward<FUNC>(undo_action))
  {}

  ~UndoHelper()
  {
    if (m_undo)
      m_undo_action();
  }

  UndoHelper(UndoHelper const &) = delete;
  UndoHelper &operator=(UndoHelper const &) = delete;

  UndoHelper(UndoHelper &&other)
  {
    m_undo_action = std::move(other.m_undo_action);
    m_undo = other.m_undo;
    other.m_undo = false;
  }

  UndoHelper &operator=(UndoHelper &&other)
  {
    m_undo_action = std::move(other.m_undo_action);
    m_undo = other.m_undo;
    other.m_undo = false;

    return *this;
  }

  void doit()
  { m_undo = false; }

  void undoit()
  { m_undo = true; }

private:
  std::function<void()> m_undo_action;
  bool m_undo { true };
};

} // end namespace bc
