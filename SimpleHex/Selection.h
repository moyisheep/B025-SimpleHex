#pragma once


class Selection
{
public:
    Selection();


    bool HasSelection(size_t index);
    bool HasSelection();
    void Set(size_t start, size_t end);
    void Get(size_t& start, size_t& end) const;
    void Clear();

    void OnMouseDown(size_t index);

    void OnMouseUp(size_t index);

    void OnMouseMove(size_t index);

private:
    // 选择状态
    size_t m_selectionStart;
    size_t m_selectionEnd;
    bool m_hasSelection;
    
};