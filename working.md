[------------------------------ __TODO__ ---------------------------------]
# [ FEATURE ] `ctrl+shift+(left/right)` arrow for word selection
# [ FEATURE ] Highlighting all occurances of selected word in buffer
# [ FEATURE ] Handle Horizontal Scrolling using `shift+mouseWheel`
# [ FEATURE ] Smooth Scrolling
# [ FEATURE ] Copying/Cutting/Deleting Line Selection
# [ FEATURE ] Pasting Multiline Text
# [ FEATURE ] Swapping Multiple Lines using `Ctrl+Shift+(up/down)`
# [  DEBUG  ] Setup for seeing current state variables in left pane
# [ FEATURE ] Using TAB or SHIFT+TAB for indentation of currently selected line
# [ UPGRADE ] Use AddItem(size) at top and bottom of visible window for smooth scrolling




########################################################################################


## Monday 18th Sept 2023
+ [   BUG   ] `mLinePosition.y` changes while Scrolling
+ [ FEATURE ] Multiline selection
+ [ FEATURE ] Line Selection Triple Click
+ [ FEATURE ] Mouse Drag and Selection
+ [ UPGRADE ] Show one width for empty line selection


########################################################################################


## Sunday 17th Sept 2023
+ [   BUG   ] insertion error when tabs ahead of `mState.mCursorPosition.mColumn`
+ [ FEATURE ] `shift+(left/right)` arrow for letter selection
+ [ FEATURE ] Scrolling with cursor position
+ [ FEATURE ] Add a Left Padding parameter to editor text
+ [   BUG   ] Fix Line Number Bar width based on max lines
+ [ FEATURE ] Build a `std::map` of theme color
+ [ FEATURE ] Copying/Cutting/Pasting Word Selection


########################################################################################


## Saturday 16th Sept 2023
+ [ FEATURE ] Added Word Selection using double click
+ [   BUG   ] Fixed while clicking on line selects neighbouring lines
+ [ FEATURE ] Handle when key pressed '{' and then '}' is pressed (aka ignore input for '}' after '{')
+ [ FEATURE ] Swapping Lines on `ctrl+shift+(up/down arrow)`
+ [ FEATURE ] `ctrl+(left/right)` arrow for word jump
+ [   BUG   ] Fix RightArrow and LeftArrow on '\t'


########################################################################################


## Monday 11th Sept 2023
+ [ FEATURE ] Tabbing Support (Tab Key)
+ [ FEATURE ] Double Click - Word Selection Backend
+ [   BUG   ] Minor Fixes
+ [ FEATURE ] Double Paranthesis and Quotations 


########################################################################################


## Sunday 10th Sept 2023
- [   BUG   ] Fix currentline selection on mouse click


########################################################################################