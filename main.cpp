#include <iostream>

#include <curses.h>
#include <vector>
#include <memory>
#include <functional>
#include <windows.h>
#include <algorithm>
#include "Point.H"

using namespace std;


class BaseVisualElement{
public:
    BaseVisualElement(WINDOW* parentWindow, int x, int y, int width, int height) :
        subwindow(subwin(parentWindow, height, width, y, x)),
        x(x), y(y), width(width), height(height)
        {
            keypad(subwindow, TRUE); // Enable special keys
        }
    virtual ~BaseVisualElement() { delwin(subwindow); }

    virtual void refresh() { wrefresh(subwindow); }
    virtual void draw() { wattroff(subwindow, A_STANDOUT); }
    void setColor(int foreground, int background) {
        init_pair(objectOrder, foreground, background);
        wattron(subwindow, COLOR_PAIR(objectOrder));
        wbkgd(subwindow, COLOR_PAIR(objectOrder)); // Set both text and background color for clarity. You can remove this if needed.
        refresh();
    }

    virtual void handleInput(int input_) = 0; // Pure virtual for input handling
    virtual void onFocus() { //wattron(subwindow, A_STANDOUT);
    draw(); }
    virtual void onFocusLost() {  }

    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int objectOrder = 1;

    WINDOW *subwindow;
    int x, y, width, height;
};

class BaseVisualElement_Scroller : public BaseVisualElement{
public:
    BaseVisualElement_Scroller(WINDOW* parentWindow, int x, int y, int width, int height) :
        BaseVisualElement(parentWindow, x, y, width, height)
        {
            // Additional initialization specific to BaseVisualElement_Scroller if needed
        }
    virtual ~BaseVisualElement_Scroller() { delwin(subwindow); }


    // Scroll bar features
    bool hasVerticalScrollbar = false;
    bool hasHorizontalScrollbar = false;

    int scrollPosition = 0;  // Vertical scroll position (for now)
    int verticalScrollPosition = 0;   // Current vertical scroll position
    int horizontalScrollPosition = 0; // Current horizontal scroll position
    int maxVerticalScrollPosition = 0;   // Maximum for vertical scrolling
    int maxHorizontalScrollPosition = 0; // Maximum for horizontal scrolling

    void drawVerticalScrollbar(){
        int scrollbarHeight = height; // Leave some border
        int maxThumbSize = scrollbarHeight;
        int thumbSize = std::max(1, (int)((float)scrollbarHeight * height / maxVerticalScrollPosition));
        int thumbOffset = (int)((float)scrollbarHeight * verticalScrollPosition / maxVerticalScrollPosition);

        for(int i = 0; i < height; i++) {
            mvwaddch(subwindow, i, width - 1, (i == verticalScrollPosition ? '#' : '|'));
        }
        curs_set(0);
    }
    void drawHorizontalScrollbar(){
        int scrollbarWidth = width; // Leave some border
        int maxThumbSize = scrollbarWidth;
        int thumbSize = std::max(1, (int)((float)scrollbarWidth * width / maxHorizontalScrollPosition));
        int thumbOffset = (int)((float)scrollbarWidth * horizontalScrollPosition / maxHorizontalScrollPosition);

        for(int i = 0; i < width; i++) {
            mvwaddch(subwindow, height - 1, i, (i == horizontalScrollPosition ? '#' : '-'));
        }
        curs_set(0);
    }
};

/////////////////////////////////////////////////////////////////////////
// New Label element
class Label : public BaseVisualElement {
public:
    Label(WINDOW* parentWindow, const std::string& text, int x, int y,  int width, int height) :
        BaseVisualElement(parentWindow, x, y, width, height),
        text(text)
    {
        draw();
    }

    virtual void draw() override {
        // No box - just print the text, you can add attributes as needed
        mvwprintw(subwindow, 0, 0, text.c_str());
        wrefresh(subwindow);
    }

    // Labels typically don't handle input
    virtual void handleInput(int input_) override {}

private:
    std::string text;
};

/////////////////////////////////////////////////////////////////////////
// New Textbox element
class TextBox : public BaseVisualElement_Scroller {
public:
    TextBox(WINDOW* parentWindow, int x, int y, int width, int height) :
        BaseVisualElement_Scroller(parentWindow, x, y, width, height),
        textLines(1),
        cursorPos(0, 0),
        isPassword(false),
        isMultiline(true)
    {

        draw();
    }

    void setText(const std::vector<std::string>& newTextLines) {
        textLines = newTextLines;
        cursorPos.Y = std::min(cursorPos.Y, static_cast<int>(textLines.size() - 1));
        cursorPos.X = std::min(cursorPos.X, static_cast<int>(textLines[cursorPos.Y].size()));
        draw();
    }

    const std::vector<std::string>& getTextLines() const {
        return textLines;
    }

    void setPasswordMode(bool enabled) { isPassword = enabled; }
    void setMultilineMode(bool enabled) { isMultiline = enabled; }

    void setConsoleTitle(const std::string& title) {
        SetConsoleTitleA(title.c_str());
    }


    virtual void draw() override {
        wbkgd(subwindow, COLOR_PAIR(objectOrder)); // Set the window color pair
        werase(subwindow); // Clear the window
        if (hasHorizontalScrollbar) {
            maxHorizontalScrollPosition = getLongestLineLength();
            horizontalScrollPosition = (int)((float)offsetX / maxHorizontalScrollPosition * width);
        }
        if (hasVerticalScrollbar) {
            maxVerticalScrollPosition = textLines.size();
            verticalScrollPosition = (int)((float)offsetY / maxVerticalScrollPosition * height);
        }

        // Fill the entire visible area with background color
        for (int i = 0; i < height; ++i) {
            mvwhline(subwindow, i, 0, ' ', width);
        }

        int numEmptyLines = std::count_if(textLines.begin(), textLines.begin() + offsetY, [](const std::string& line) {
            return line.empty();
        });

        offsetX = (cursorPos.X > width) ? cursorPos.X - width : 0;
        for (size_t i = 0; i < height - 1 && i < textLines.size(); ++i) {
            int lineIndex = offsetY + i;
            int lineLength = std::min(std::max(0, (int)textLines[lineIndex].size() - offsetX), width);
            if ((!textLines[lineIndex].empty())&& (textLines[lineIndex].size() >= offsetX)) {
                mvwprintw(subwindow, i, 0, textLines[lineIndex].substr(offsetX, lineLength).c_str());
            } else {
                mvwprintw(subwindow, i, 0, "%*s", width, " ");
            }
        }


            // Draw scrollbars
            if(hasVerticalScrollbar == true){
                drawVerticalScrollbar();
            }
            if(hasHorizontalScrollbar == true){
                drawHorizontalScrollbar();
            }
            // Draw the cursor at its current position
            curs_set(1);
            wmove(subwindow, cursorPos.Y, cursorPos.X);
            mvwchgat(subwindow, cursorPos.Y, cursorPos.X, 1, A_BLINK, 0, NULL);
            wrefresh(subwindow);
    }

    virtual void handleInput(int input_) override {
        if(input_ == KEY_MOUSE){
            MEVENT mouseEvent;
            nc_getmouse(&mouseEvent);
            if (mouseEvent.bstate & MOUSE_WHEEL_UP) {
                    // WHEEL UP
                    if (offsetY > 0) offsetY--;
                } else if (mouseEvent.bstate & MOUSE_WHEEL_DOWN) {
                    // WHEEL DOWN
                    if (offsetY < maxVerticalScrollPosition - height) offsetY++;
                }
        } else {
            switch (input_) {
                case KEY_LEFT:
                    moveCursorLeft();
                    break;
                case KEY_RIGHT:
                    moveCursorRight();
                    break;
                case KEY_UP:
                    moveCursorUp();
                    break;
                case KEY_DOWN:
                    moveCursorDown();
                    break;
                case '\b':
                case KEY_BACKSPACE:
                    handleBackspace();
                    break;
                case '\r':
                case '\n':
                case KEY_ENTER:
                    handleEnter();
                    break;
                default:
                    insertCharacter(static_cast<char>(input_));
                    break;
            }
        }
        draw();
    }

    std::vector<std::string> splitIntoLines(const std::string& text, int width) {
           std::vector<std::string> lines;
           std::string currentLine;

           for (const char c: text) {
               currentLine += c;
               if (c == '\n' || currentLine.size() >= width) {
                   lines.push_back(currentLine);
                   currentLine = "";
               }
           }

           if (!currentLine.empty()) {
               lines.push_back(currentLine);
           }
       return lines;
   }

    virtual void onFocus() override {
        leaveok(subwindow, FALSE); // Show the cursor
        draw();
    }

    virtual void onFocusLost() override {
        leaveok(subwindow, TRUE); // Hide the cursor
        draw();
    }


private:
    void moveCursorLeft() {
        if (cursorPos.X > 0) {
            cursorPos.X--;

        } else if (cursorPos.Y > 0) {
            cursorPos.Y--;
            cursorPos.X = textLines[cursorPos.Y].size();
        }
        draw();
    }

    void moveCursorRight() {
        if (cursorPos.X < static_cast<int>(textLines[cursorPos.Y].size())) {
            cursorPos.X++;
            if (cursorPos.X >= width + offsetX) {
                offsetX++;
            }
        } else if (cursorPos.Y < static_cast<int>(textLines.size() - 1)) {
            cursorPos.Y++;
            cursorPos.X = 0;
            if (cursorPos.X >= width + offsetX) {
                offsetX++;
            }
        }
        draw();
    }

    void moveCursorUp() {
        if (cursorPos.Y > 0) {
            cursorPos.Y--;
            if(cursorPos.Y < offsetY){
                offsetY--;
            }
            cursorPos.X = std::min(cursorPos.X, static_cast<int>(textLines[cursorPos.Y].size()));
        }
        draw();
    }

    void moveCursorDown() {
        if (cursorPos.Y < static_cast<int>(textLines.size()) - 1) {
            cursorPos.Y++;
            cursorPos.X = std::min(cursorPos.X, static_cast<int>(textLines[cursorPos.Y].size()));
        }
        // Adjust offsetY if necessary for scrolling
        if(cursorPos.Y >= offsetY + height - 1){
            offsetY++;
        }
        draw();
    }

    void handleBackspace() {
        if (cursorPos.X > 0) {
            textLines[cursorPos.Y].erase(cursorPos.X - 1, 1);
            cursorPos.X--;
        } else if (cursorPos.Y > 0) {
            // Merge the current line with the previous one
            cursorPos.X = textLines[cursorPos.Y - 1].size();
            textLines[cursorPos.Y - 1] += textLines[cursorPos.Y];
            textLines.erase(textLines.begin() + cursorPos.Y);
            cursorPos.Y--;
            beep();
        }
        draw();
    }

    void handleEnter() {
        // Reset the offsetX to 0
        offsetX = 0;

        // Split the current line at the cursor position
        std::string& currentLine = textLines[cursorPos.Y];
        std::string newLine = currentLine.substr(cursorPos.X);
        currentLine.erase(cursorPos.X);
        cursorPos.Y++;
        cursorPos.X = 0;

        // Adjust offsetY if necessary for scrolling
        if(cursorPos.Y >= offsetY + height - 1){
            offsetY++;
        }

        // Insert the new line
        textLines.insert(textLines.begin() + cursorPos.Y, newLine);
        draw();
    }

    void insertCharacter(char ch) {
        if (cursorPos.X < width + offsetX) {
            textLines[cursorPos.Y].insert(cursorPos.X, 1, ch);
            cursorPos.X++;

                // Adjust offsetX if the cursor has moved past the right edge
                if (cursorPos.X >= width + offsetX) {
                    offsetX++;
                }
            } else { // Scroll horizontally
                offsetX++;
                textLines[cursorPos.Y].insert(cursorPos.X, 1, ch);
                cursorPos.X++;
            }

        draw();

    }

    std::vector<std::string> textLines;
    Point cursorPos;
    bool isPassword;
    bool isMultiline;
    int offsetX = 0;
    int offsetY = 0;

    int getLongestLineLength() const {
        int longest = 0;
        for (const std::string& line : textLines) {
            longest = std::max(longest, (int)line.size());
        }
        return longest;
    }
};

///////////////////////////////////////////////////////////////////////////
// New Checkbox element
class CheckboxList : public BaseVisualElement {
public:
    CheckboxList(WINDOW* parentWindow, const std::string &title, const std::vector<std::string> &items,
                 int x, int y, int width, int height) :
        BaseVisualElement(parentWindow, x, y, width, height),
        title(title),
        items(items)
    {
        initialize();  // We'll define this next
        draw();
    }

    const std::vector<bool>& getSelectedIndices() const { return selectedIndices; }

    void initialize() {
        selectedIndices.resize(items.size(), false); // Start with all unselected
    }

    void toggleCheckbox(int index) {
        selectedIndices[index] = !selectedIndices[index];
        draw();
    }

    virtual void refresh() override {
        wrefresh(subwindow);
    }

    virtual void draw() override {
        box(subwindow, 0, 0);
        mvwprintw(subwindow, 1, 2, title.c_str());
        int y = 2;
        for (int i = 0; i < items.size(); ++i, ++y) {

            mvwaddch(subwindow, y, 3, (selectedIndices[i] ? L'X' : L' ')); // Checkbox symbol
            mvwprintw(subwindow, y, 5, items[i].c_str());
        }
        refresh();
    }

    virtual void handleInput(int input_) override {
        if (input_ == KEY_MOUSE) {
            MEVENT mouseEvent;
            nc_getmouse(&mouseEvent);

            // Calculate offset relative to the top-left corner of the CheckboxList subwindow
            int offsetX = mouseEvent.x - getX();
            int offsetY = mouseEvent.y - getY();

            // Check if the click is within the CheckboxList bounds
            if (offsetX >= 0 && offsetX < getWidth() && offsetY >= 0 && offsetY < getHeight()) {
                int clickedIndex = findClickedItem(offsetY);
                if (clickedIndex != -1) {
                    toggleCheckbox(clickedIndex);
                }
            }
        }
        // Handle other keys like KEY_UP, KEY_DOWN if needed
    }

    int findClickedItem(int clickY) {
        if (clickY >= 2 && clickY < 2 + items.size()) { // Within list bounds
            return clickY - 2; // Calculate index
        }
        return -1;
    }

    std::string title;
    std::vector<std::string> items;
    std::vector<bool> selectedIndices;
};

///////////////////////////////////////////////////////////////////////////
// New Button Element
class Button : public BaseVisualElement {
public:
    Button(WINDOW* parentWindow, std::string text, int x, int y, int width, int height) :
        BaseVisualElement(parentWindow, x, y, width, height),
        text(std::move(text)) {
            draw();
        }

    virtual void refresh() override {
        wrefresh(subwindow);
    }

    virtual void draw() override {
        box(subwindow, 0, 0);
        wattrset(subwindow, A_NORMAL);
        mvwprintw(subwindow, 1, (width - text.length()) / 2, text.c_str()); // Center the text
        refresh();
    }

    virtual void handleInput(int input_) override {
         if (input_ == KEY_MOUSE) {
            MEVENT mouseEvent;
            nc_getmouse(&mouseEvent);
            if (wenclose(subwindow, mouseEvent.y, mouseEvent.x)) {
                if (mouseEvent.bstate & BUTTON1_PRESSED) {
                    onPress();
                } else if (mouseEvent.bstate & BUTTON1_RELEASED) {
                    onRelease();
                }
            }
        }
    }

    // Event handlers
    std::function<void()> onPress; // Function to call on button press
    std::function<void()> onRelease; // Function to call on button click (release)

private:
    std::string text;
};

///////////////////////////////////////////////////////////////////////////
// New Selection List Element
class SelectionList : public BaseVisualElement {
public:
    SelectionList(WINDOW* parentWindow, const std::string &title, const std::vector<std::string> &items,
                  int x, int y, int width, int height) :
        BaseVisualElement(parentWindow, x, y, width, height),
        title(title),
        items(items),
        selectedIndex(0)
    {
        draw();
    }

    virtual void handleInput(int input_) override {
        switch (input_) {
            case KEY_UP:
                selectedIndex--;
                if (selectedIndex < 0) {
                    selectedIndex = items.size() - 1; // Wrap around
                }
                adjustView();  // Ensure the selected item is visible
                break;
            case KEY_DOWN:
                selectedIndex++;
                if (selectedIndex >= items.size()) {
                    selectedIndex = 0;
                }
                adjustView();
                break;
            // ... Add cases for Enter (selection), etc.
            default:
                return;
        }
        draw();
        return; // Might change based on other input handling
    }

    virtual void refresh() override {
        wrefresh(subwindow);
    }

    virtual void draw() override {
        box(subwindow, 0, 0);
        mvwprintw(subwindow, 1, 2, title.c_str());

        int start = currentOffset;
        int y = 2;
        for (int i = start; i < items.size() && y < height - 1; i++, y++) {
            if (i == selectedIndex) {
                wattron(subwindow, A_REVERSE); // Highlight selected
            }
            mvwprintw(subwindow, y, 2, items[i].c_str());
            wattroff(subwindow, A_REVERSE);
        }
        refresh();
    }
private:
    void adjustView() {
        if (selectedIndex < currentOffset) {
            currentOffset = selectedIndex;
        } else if (selectedIndex >= currentOffset + height - 3) {
            currentOffset = selectedIndex - height + 4; // Keep a bit visible above
        }
    }

    std::string title;
    std::vector<std::string> items;
    int selectedIndex;
    int currentOffset = 0; // Offset for scrolling within the list
};

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

int main()
{
    initscr(); // Initialize PDCurses
    cbreak();

    mouse_set(ALL_MOUSE_EVENTS);
    PDC_return_key_modifiers(TRUE);
    noecho();

    // CREATE THE MAIN WINDOW
    WINDOW *mainWindow = newwin(LINES - 1, COLS - 1, 0, 0);
    chtype ch;
    MEVENT mouseinput;
    keypad(mainWindow, TRUE); // Enable special keys
    box(mainWindow, 0, 0); // Draw a border
    wrefresh(mainWindow); // Refresh
    start_color();

    leaveok(mainWindow, TRUE); // Hide the cursor

    // CREATE MASTER LIST OF ELEMENTS
    std::vector<std::unique_ptr<BaseVisualElement>> elements; // Master list
    BaseVisualElement* focusedElement = nullptr; // Points to element in focus

    // CREATE EXAMPLE ELEMENTS
    std::vector<std::string> options = {"Option 1", "Option 2", "Option 3"};
    SelectionList *selectionlist1 = new SelectionList(mainWindow, "Choose One", options, 5, 10, 20, 10);
    selectionlist1->subwindow = subwin(mainWindow, selectionlist1->getHeight(), selectionlist1->getWidth(), selectionlist1->getY(), selectionlist1->getX());

    // Example label implementation
    Label *label1 = new Label(mainWindow, "Label testing!", 8, 3, 10, 1);
    label1->subwindow = subwin(mainWindow, label1->getHeight(), label1->getWidth(), label1->getY() , label1->getX() );

    // Example button implementation
    Button *button1 = new Button(mainWindow, "Click Me!", 5, 5, 20, 4);
    button1->subwindow = subwin(mainWindow, button1->getHeight(), button1->getWidth(), button1->getY() , button1->getX() );
    button1->onPress = [&button1]() {
        mvwprintw(button1->subwindow, 1, 1, "Button pressed!"); // call back example
    };

    button1->onRelease = [&button1]() {
        mvwprintw(button1->subwindow, 1, 1, "Button clicked!"); // call back example
    };

    // Example checkbox implementation
    CheckboxList *checkbox1 = new CheckboxList(mainWindow, "options", options, 30, 2, 20, 8);
    checkbox1->subwindow = subwin(mainWindow, checkbox1->getHeight(), checkbox1->getWidth(), checkbox1->getY(), checkbox1->getX());

    // Example textbox implementation
    TextBox *txtbox1 = new TextBox(mainWindow, 50, 5, 20, 10);
    txtbox1->subwindow = subwin(mainWindow, txtbox1->getHeight(), txtbox1->getWidth(), txtbox1->getY(), txtbox1->getX());
    txtbox1->hasVerticalScrollbar = true;
    txtbox1->hasHorizontalScrollbar = true;

    // PUSH CREATED ELEMENTS TO MASTER LIST
    elements.push_back(std::unique_ptr<BaseVisualElement>(selectionlist1));
    elements.push_back(std::unique_ptr<BaseVisualElement>(button1));
    elements.push_back(std::unique_ptr<BaseVisualElement>(checkbox1));
    elements.push_back(std::unique_ptr<BaseVisualElement>(txtbox1));
    elements.push_back(std::unique_ptr<BaseVisualElement>(label1));

    int x = 1;
    for (auto & element : elements) {
            element->objectOrder = x;
            x++;
            element->setColor(COLOR_BLACK, COLOR_WHITE);
    }
    selectionlist1->setColor(COLOR_RED, COLOR_WHITE); // Testing coloration of selection list


    MEVENT event;
    auto isPointInside = [](int clickX, int clickY, int elementX, int elementY, int elementWidth, int elementHeight) {
        if((clickX >= elementX) && (clickY >= elementY)){
            if ((clickX <= elementWidth) && (clickY <= elementHeight)){
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }

    };

    // Function handleMouseClick will process mouse event args
    auto handleMouseClick = [&](int x, int y) {
        bool withinElement = false;
        for (auto & element : elements) {
            // Check if the click is within the element's subwindow
            if (wenclose(element->subwindow, y, x)) {
                focusedElement = element.get();
                focusedElement->onFocus(); // Call onFocus on the new element
                withinElement = true;
                beep();
                break;
                }
        }
        if(withinElement == false){
            focusedElement = nullptr;
            wrefresh(mainWindow); // Refresh main window to show changes
            for (auto & element : elements) {
                element->draw();
            }
        }
        if (focusedElement && withinElement) {
            // Check if the focused element supports mouse input before dispatching the event
            if (dynamic_cast<Button*>(focusedElement) || dynamic_cast<CheckboxList*>(focusedElement)) {
                focusedElement->handleInput(KEY_MOUSE); // Pass mouse event to the focused element
            }
        }
    };

     while (1) {
            while(1){
                ch = wgetch(mainWindow);
                if(ch == ERR){

                } else {

                    break;
                }
            }

            if (ch == KEY_MOUSE) { // KEY_MOUSE indicates a mouse event
                    nc_getmouse (&mouseinput);
                   handleMouseClick(mouseinput.x, mouseinput.y); // Pass mouse event coords to handleMouseClick
            } else {
                // If it's not a mouse event, send input to the focused element
                if (focusedElement) {
                    focusedElement->handleInput(ch);
                    focusedElement->refresh();
                }
            }
        wrefresh(mainWindow); // Refresh main window to show changes
        for (auto & element : elements) {
            element->refresh();
        }

        }
    endwin(); // End PDCurses
    return 0;
}
