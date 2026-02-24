#include "Selection.h"
#include <algorithm>

Selection::Selection()
:m_selectionStart(0),
m_selectionEnd(0),
m_hasSelection(false)
{
}



bool Selection::HasSelection(size_t index)
{
	size_t start = std::min(m_selectionStart, m_selectionEnd);
	size_t end = std::max(m_selectionStart, m_selectionEnd);
	if (m_hasSelection && index >= start && index <= end)
	{
		return true;
	}
	return false;
}

bool Selection::HasSelection()
{
	return m_hasSelection;
}



void Selection::Set(size_t start, size_t end)
{
	m_selectionStart = start;
	m_selectionEnd = start;
	if(m_selectionStart != m_selectionEnd)
	{
		m_hasSelection = true;
	}else
	{
		m_hasSelection = false;
	}
}

void Selection::Get(size_t& start, size_t& end) const
{
	start = std::min(m_selectionStart, m_selectionEnd);
	end = std::max(m_selectionStart, m_selectionEnd);
}

void Selection::Clear()
{
	m_selectionStart = 0;
	m_selectionEnd = 0;
	m_hasSelection = false;
}

void Selection::OnMouseDown(size_t index)
{
	m_selectionStart = m_selectionEnd = index;
	m_hasSelection = true;
}

void Selection::OnMouseUp(size_t index)
{
}

void Selection::OnMouseMove(size_t index)
{
	m_selectionEnd = index;
}

