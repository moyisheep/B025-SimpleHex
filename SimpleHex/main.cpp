#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>
#include <fstream>
#include <vector>
#include <memory>
#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <unordered_map>

class HexViewer : public tgui::Panel
{
public:
    typedef std::shared_ptr<HexViewer> Ptr;
    typedef std::shared_ptr<const HexViewer> ConstPtr;

    HexViewer()
    {
        m_scrollbar = tgui::Scrollbar::create();
        m_scrollbar->setSize(20, getSize().y);
        m_scrollbar->setPosition(getSize().x - 20, 0);
        m_scrollbar->setAutoHide(false);
        m_scrollbar->onValueChange([this] { updateDisplay(); });
        add(m_scrollbar);

        m_addressPanel = tgui::Panel::create();
        m_addressPanel->setPosition(0, 0);
        m_addressPanel->getRenderer()->setBackgroundColor(tgui::Color(240, 240, 240));
        add(m_addressPanel);

        m_hexPanel = tgui::Panel::create();
        m_hexPanel->setPosition(m_addressPanel->getSize().x, 0);
        add(m_hexPanel);

        m_asciiPanel = tgui::Panel::create();
        m_asciiPanel->setPosition(m_hexPanel->getPosition().x + m_hexPanel->getSize().x, 0);
        m_asciiPanel->getRenderer()->setBackgroundColor(tgui::Color(250, 250, 250));
        add(m_asciiPanel);

        m_selectionStart = -1;
        m_selectionEnd = -1;

        onSizeChange([this] {
            updateLayout();
            updateDisplay();
            });
    }

    static HexViewer::Ptr create()
    {
        return std::make_shared<HexViewer>();
    }

    bool loadFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open())
            return false;

        m_fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        const size_t CHUNK_SIZE = 1024 * 1024; // 1MB chunks
        m_data.clear();
        m_data.reserve(m_fileSize);

        std::vector<char> buffer(CHUNK_SIZE);
        while (file.read(buffer.data(), buffer.size()))
        {
            m_data.insert(m_data.end(), buffer.begin(), buffer.begin() + file.gcount());
        }
        if (file.gcount() > 0)
        {
            m_data.insert(m_data.end(), buffer.begin(), buffer.begin() + file.gcount());
        }

        m_filename = filename;
        updateScrollbar();
        updateDisplay();
        return true;
    }

    void addAnnotation(size_t offset, const std::string& text)
    {
        m_annotations[offset] = text;
        updateDisplay();
    }

    void removeAnnotation(size_t offset)
    {
        m_annotations.erase(offset);
        updateDisplay();
    }

    void clearAnnotations()
    {
        m_annotations.clear();
        updateDisplay();
    }

private:
    struct ByteLabel : public tgui::Label
    {
        size_t offset;
        bool isSelected = false;
        bool hasAnnotation = false;
        std::string annotationText;

        ByteLabel::Ptr create()
        {
            return std::make_shared<ByteLabel>();
        }

        void updateColor()
        {
            if (isSelected)
            {
                getRenderer()->setTextColor(tgui::Color::White);
                getRenderer()->setBackgroundColor(tgui::Color(0, 120, 215));
            }
            else if (hasAnnotation)
            {
                getRenderer()->setTextColor(tgui::Color::White);
                getRenderer()->setBackgroundColor(tgui::Color(255, 140, 0));
            }
            else
            {
                getRenderer()->setTextColor(tgui::Color::Black);
                getRenderer()->setBackgroundColor(tgui::Color::Transparent);
            }
        }
    };

    void updateLayout()
    {
        auto size = getSize();

        // Address panel width
        float addrWidth = 80;
        m_addressPanel->setSize(addrWidth, size.y - 20);

        // Hex panel width (16 bytes * 3 chars per byte)
        float hexWidth = 16 * 3 * 12;
        m_hexPanel->setSize(hexWidth, size.y - 20);

        // ASCII panel width (16 chars)
        float asciiWidth = 16 * 12;
        m_asciiPanel->setSize(asciiWidth, size.y - 20);

        m_scrollbar->setSize(20, size.y - 20);
        m_scrollbar->setPosition(size.x - 20, 0);

        m_bytesPerRow = 16;
        m_rowHeight = 20;
        m_visibleRows = static_cast<int>((size.y - 20) / m_rowHeight);
    }

    void updateScrollbar()
    {
        m_scrollbar->setMaximum(static_cast<unsigned int>(
            (m_data.size() + m_bytesPerRow - 1) / m_bytesPerRow));
        m_scrollbar->setLowValue(m_visibleRows);
        m_scrollbar->setValue(0);
    }

    void updateDisplay()
    {
        m_addressPanel->removeAllWidgets();
        m_hexPanel->removeAllWidgets();
        m_asciiPanel->removeAllWidgets();

        if (m_data.empty())
            return;

        size_t startRow = m_scrollbar->getValue();
        size_t endRow = std::min(startRow + m_visibleRows,
            (m_data.size() + m_bytesPerRow - 1) / m_bytesPerRow);

        for (size_t row = startRow; row < endRow; ++row)
        {
            // Address column
            auto addrLabel = tgui::Label::create();
            addrLabel->setText(tgui::String("0x") +
                tgui::String::fromNumber(row * m_bytesPerRow, 16, 8, '0').toUpper());
            addrLabel->setPosition(5, (row - startRow) * m_rowHeight);
            addrLabel->setTextSize(12);
            m_addressPanel->add(addrLabel);

            // Hex bytes
            for (int col = 0; col < m_bytesPerRow; ++col)
            {
                size_t offset = row * m_bytesPerRow + col;
                if (offset >= m_data.size())
                    break;

                auto byteLabel = ByteLabel::create();
                byteLabel->offset = offset;

                // Format hex byte
                std::stringstream ss;
                ss << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(static_cast<unsigned char>(m_data[offset]));
                byteLabel->setText(tgui::String(ss.str()).toUpper());
                byteLabel->setTextSize(12);
                byteLabel->setSize(24, m_rowHeight);
                byteLabel->setPosition(col * 24, (row - startRow) * m_rowHeight);

                // Check selection
                byteLabel->isSelected = (offset >= m_selectionStart && offset <= m_selectionEnd);

                // Check annotation
                auto it = m_annotations.find(offset);
                if (it != m_annotations.end())
                {
                    byteLabel->hasAnnotation = true;
                    byteLabel->annotationText = it->second;

                    // Add tooltip for annotation
                    byteLabel->setToolTip(tgui::Label::create(it->second));
                }

                byteLabel->updateColor();

                // Mouse events
                byteLabel->onMousePress([this, byteLabel](tgui::Vector2f, tgui::Event::MouseButton button) {
                    if (button == tgui::Event::MouseButton::Left)
                    {
                        m_selectionStart = m_selectionEnd = byteLabel->offset;
                        updateDisplay();
                    }
                    else if (button == tgui::Event::MouseButton::Right)
                    {
                        showContextMenu(byteLabel->offset);
                    }
                    });

                byteLabel->onMouseMove([this, byteLabel](tgui::Vector2f pos) {
                    if (tgui::Mouse::isButtonPressed(tgui::Event::MouseButton::Left))
                    {
                        m_selectionEnd = byteLabel->offset;
                        updateDisplay();
                    }
                    });

                m_hexPanel->add(byteLabel);
            }

            // ASCII column
            for (int col = 0; col < m_bytesPerRow; ++col)
            {
                size_t offset = row * m_bytesPerRow + col;
                if (offset >= m_data.size())
                    break;

                char c = m_data[offset];
                auto asciiLabel = ByteLabel::create();
                asciiLabel->offset = offset;
                asciiLabel->setText(tgui::String(std::isprint(c) ? std::string(1, c) : "."));
                asciiLabel->setTextSize(12);
                asciiLabel->setSize(12, m_rowHeight);
                asciiLabel->setPosition(col * 12, (row - startRow) * m_rowHeight);

                // Check selection
                asciiLabel->isSelected = (offset >= m_selectionStart && offset <= m_selectionEnd);

                // Check annotation
                auto it = m_annotations.find(offset);
                if (it != m_annotations.end())
                {
                    asciiLabel->hasAnnotation = true;
                    asciiLabel->annotationText = it->second;
                    asciiLabel->setToolTip(tgui::Label::create(it->second));
                }

                asciiLabel->updateColor();

                // Mouse events (same as hex labels)
                asciiLabel->onMousePress([this, asciiLabel](tgui::Vector2f, tgui::Event::MouseButton button) {
                    if (button == tgui::Event::MouseButton::Left)
                    {
                        m_selectionStart = m_selectionEnd = asciiLabel->offset;
                        updateDisplay();
                    }
                    else if (button == tgui::Event::MouseButton::Right)
                    {
                        showContextMenu(asciiLabel->offset);
                    }
                    });

                asciiLabel->onMouseMove([this, asciiLabel](tgui::Vector2f pos) {
                    if (tgui::Mouse::isButtonPressed(tgui::Event::MouseButton::Left))
                    {
                        m_selectionEnd = asciiLabel->offset;
                        updateDisplay();
                    }
                    });

                m_asciiPanel->add(asciiLabel);
            }
        }
    }

    void showContextMenu(size_t offset)
    {
        auto menu = tgui::MenuBar::create();
        menu->addMenuItem("Add Annotation");
        menu->addMenuItem("Remove Annotation");
        menu->addMenuItem("Copy Selection");
        menu->addMenuItem("Go To Offset...");

        menu->setPosition(tgui::Mouse::getPosition(*getParent()));
        menu->setVisible(true);
        menu->onMenuItemClick([this, offset, menu](const tgui::String& item) {
            if (item == "Add Annotation")
            {
                showAnnotationDialog(offset);
            }
            else if (item == "Remove Annotation")
            {
                removeAnnotation(offset);
            }
            else if (item == "Copy Selection")
            {
                copySelectionToClipboard();
            }
            else if (item == "Go To Offset...")
            {
                showGoToDialog();
            }
            menu->setVisible(false);
            });

        getParent()->add(menu);
    }

    void showAnnotationDialog(size_t offset)
    {
        auto window = tgui::ChildWindow::create("Add Annotation");
        window->setSize(400, 200);
        window->setPosition("(&.size - size) / 2");
        window->setResizable(true);

        auto editBox = tgui::EditBox::create();
        editBox->setSize(380, 100);
        editBox->setPosition(10, 40);
        editBox->setDefaultText("Enter annotation text...");

        auto okButton = tgui::Button::create("OK");
        okButton->setSize(80, 30);
        okButton->setPosition(200, 150);
        okButton->onPress([this, window, editBox, offset] {
            addAnnotation(offset, editBox->getText().toStdString());
            window->close();
            });

        auto cancelButton = tgui::Button::create("Cancel");
        cancelButton->setSize(80, 30);
        cancelButton->setPosition(300, 150);
        cancelButton->onPress([window] { window->close(); });

        window->add(editBox);
        window->add(okButton);
        window->add(cancelButton);

        getParent()->add(window);
    }

    void copySelectionToClipboard()
    {
        if (m_selectionStart == -1 || m_selectionEnd == -1)
            return;

        size_t start = std::min(m_selectionStart, m_selectionEnd);
        size_t end = std::max(m_selectionStart, m_selectionEnd);

        std::stringstream hexStream, asciiStream;
        for (size_t i = start; i <= end && i < m_data.size(); ++i)
        {
            unsigned char byte = static_cast<unsigned char>(m_data[i]);
            hexStream << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(byte) << " ";
            asciiStream << (std::isprint(byte) ? static_cast<char>(byte) : '.');
        }

        // In real implementation, you would copy to system clipboard
        std::cout << "Hex: " << hexStream.str() << std::endl;
        std::cout << "ASCII: " << asciiStream.str() << std::endl;
    }

    void showGoToDialog()
    {
        auto window = tgui::ChildWindow::create("Go To Offset");
        window->setSize(300, 150);
        window->setPosition("(&.size - size) / 2");

        auto label = tgui::Label::create("Offset (hex):");
        label->setPosition(10, 40);

        auto editBox = tgui::EditBox::create();
        editBox->setSize(200, 30);
        editBox->setPosition(10, 70);
        editBox->setInputValidator("[0-9a-fA-F]*");
        editBox->setDefaultText("0x");

        auto okButton = tgui::Button::create("Go");
        okButton->setSize(80, 30);
        okButton->setPosition(100, 110);
        okButton->onPress([this, window, editBox] {
            try {
                size_t offset = std::stoul(editBox->getText().toStdString(), nullptr, 16);
                if (offset < m_data.size())
                {
                    size_t row = offset / m_bytesPerRow;
                    m_scrollbar->setValue(std::min(row,
                        m_scrollbar->getMaximum() - m_visibleRows));
                    updateDisplay();
                }
            }
            catch (...) {}
            window->close();
            });

        window->add(label);
        window->add(editBox);
        window->add(okButton);

        getParent()->add(window);
    }

private:
    tgui::Scrollbar::Ptr m_scrollbar;
    tgui::Panel::Ptr m_addressPanel;
    tgui::Panel::Ptr m_hexPanel;
    tgui::Panel::Ptr m_asciiPanel;

    std::vector<char> m_data;
    std::string m_filename;
    size_t m_fileSize = 0;

    std::unordered_map<size_t, std::string> m_annotations;

    int m_bytesPerRow = 16;
    int m_rowHeight = 20;
    int m_visibleRows = 30;

    size_t m_selectionStart;
    size_t m_selectionEnd;
};

class MainWindow
{
public:
    void run()
    {
        // Create window
        sf::RenderWindow window(sf::VideoMode(1200, 800), "Hex Viewer");
        tgui::Gui gui(window);

        // Create menu
        auto menu = tgui::MenuBar::create();
        menu->setSize("100%", 30);
        menu->addMenu("File");
        menu->addMenuItem("File", "Open");
        menu->addMenuItem("File", "Exit");
        menu->addMenu("View");
        menu->addMenuItem("View", "Annotations");
        menu->addMenu("Help");
        menu->addMenuItem("Help", "About");

        menu->onMenuItemClick([this, &gui](const tgui::String& menuItem) {
            if (menuItem == "Open")
                openFile(gui);
            else if (menuItem == "Exit")
                exit();
            else if (menuItem == "Annotations")
                showAnnotations(gui);
            else if (menuItem == "About")
                showAbout(gui);
            });

        gui.add(menu);

        // Create hex viewer
        m_hexViewer = HexViewer::create();
        m_hexViewer->setPosition(0, 30);
        m_hexViewer->setSize("100%", "100% - 30");
        gui.add(m_hexViewer);

        // Create status bar
        auto statusBar = tgui::Label::create();
        statusBar->setPosition(0, "100% - 20");
        statusBar->setSize("100%", 20);
        statusBar->getRenderer()->setBackgroundColor(tgui::Color(240, 240, 240));
        statusBar->setText("Ready");
        gui.add(statusBar);

        // Main loop
        while (window.isOpen())
        {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window.close();

                gui.handleEvent(event);
            }

            window.clear(sf::Color::White);
            gui.draw();
            window.display();
        }
    }

private:
    HexViewer::Ptr m_hexViewer;

    void openFile(tgui::Gui& gui)
    {
        auto fileDialog = tgui::FileDialog::create("Open File", "Open", false);
        fileDialog->setFileMustExist(true);
        fileDialog->onFileSelect([this, &gui](const tgui::String& filename) {
            if (m_hexViewer->loadFile(filename.toStdString()))
            {
                // Update status
                auto status = gui.get<tgui::Label>("status");
                if (status)
                    status->setText("Loaded: " + filename);
            }
            });
        gui.add(fileDialog);
    }

    void showAnnotations(tgui::Gui& gui)
    {
        auto window = tgui::ChildWindow::create("Annotations");
        window->setSize(500, 400);
        window->setPosition("(&.size - size) / 2");

        auto listBox = tgui::ListBox::create();
        listBox->setSize(480, 350);
        listBox->setPosition(10, 40);

        // Here you would populate with actual annotations
        listBox->addItem("No annotations available");
        listBox->setItemHeight(25);

        window->add(listBox);
        gui.add(window);
    }

    void showAbout(tgui::Gui& gui)
    {
        auto window = tgui::ChildWindow::create("About");
        window->setSize(300, 200);
        window->setPosition("(&.size - size) / 2");

        auto label = tgui::Label::create("Hex Viewer v1.0\n\n"
            "A simple hex viewer with annotation support\n"
            "Built with TGUI");
        label->setPosition(20, 40);
        label->setTextSize(14);

        auto closeButton = tgui::Button::create("Close");
        closeButton->setSize(80, 30);
        closeButton->setPosition(110, 150);
        closeButton->onPress([window] { window->close(); });

        window->add(label);
        window->add(closeButton);
        gui.add(window);
    }

    void exit()
    {
        // Implement clean exit
    }
};

int main()
{
    MainWindow app;
    app.run();
    return 0;
}