# PDCursesGUI
A starter template for making some basic GUI elements in PDCurses.

This is a very preliminary sample program I've put together that implements features in PDCurses to create basic GUI elements.
Some of the basic elements are working near perfect. The textbox still has some glitchy behavior with the scrolling I'm working on and 
the button click event needs some closer examination. At some point once all the kinks are worked out, I'll likely refactor this into 
.DLL  class library for portability. Feel free to use this for your own projects.

Elements are derived from either `BaseVisualElement` *or* `BaseVisualElement_Scroller` and their constructor will usually take a reference to the target window first,
typically followed by any element-specific parameters (ie text, string vector as options, ect) then finally the X, Y, width and height values. An example of
a simple label:
```cpp
    // Example label implementation
    Label *label1 = new Label(mainWindow, "Label testing!", 8, 3, 10, 1);
    label1->subwindow = subwin(mainWindow, label1->getHeight(), label1->getWidth(), label1->getY() , label1->getX() );
```

To add an element, you must push it to the elements vector:
```cpp
    elements.push_back(std::unique_ptr<BaseVisualElement>(YOU_NEW_ELEMENT));
```

**Demonstration:**

https://github.com/realChrisDeBon/PDCursesGUI/assets/97779307/842da802-7e78-4a09-b8ec-2c96ac730ad7

**Note:** The Windows tone can be removed on-click, it's placed in for debugging and demonstration reasons.
