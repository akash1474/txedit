[------------------------------ __TODO__ ---------------------------------]
# [   BUG   ] Fixing Quotations both single and double Quotations
# [ FEATURE ] Smooth Scrolling
<!-- # [ FEATURE ] Using Custum Title Bar -->
# [ FEATURE ] Support For UTF-8 characters
# [ FEATURE ] Bottom Bar containing (lineNo,colNo,gitbranch,tabsize,filetype)
# [ FEATURE ] Use AddItem(size) at top and bottom of visible window for smooth scrolling
# [ FEATURE ] Search and Replace
# [ FEATURE ] File Operations
# [ FEATURE ] Line Numbers and Column Indicators
# [ FEATURE ] Tabs and Split Views
# [ FEATURE ] Full-Screen Mode
# [ FEATURE ] Auto-Save and Backup



########################################################################################


##  Thursday 14th Oct 2023
+ [   BUG   ] Multi-Cursor '{' issue -- only adds '}' on mCursors[0]
+ [   BUG   ] Multi-Cursor sometime characters insert only on mCursors[0]
+ [ FEATURE ] Using Arrow Keys in Multi-Cursor
+ [ FEATURE ] Add Delete KeyBinding
+ [   BUG   ] Ctrl+Shift Multi-Cursor Selection

########################################################################################


##  Thursday 7th Oct 2023

+ [ FEATURE ] Using Ctrl+Click to place multiple cursors
+ [ FEATURE ] Ctrl+D Selection and Editing at multiple positions

########################################################################################


##  Sunday 24th Sept 2023
+ [   BUG   ] Scrolling when left and right key pressed at start and end of line
+ [   BUG   ] Fix Horizontal Scrolling Amount (Content Partly Clipped)
+ [ FEATURE ] Scroll if word selection using mouse and mouse out of bounds
+ [ FEATURE ] Scroll To Cursor Position if cursor not visible
+ [ FEATURE ] Render a line for indentation


########################################################################################


##  Saturday 23th Sept 2023
+ [ IMPROVEMENT ] Improved Carriage Return and Auto indentation
+ [ IMPROVEMENT ] Lexer
+ [     BUG     ] Fix Last Line length
+ [     BUG     ] Calculate lineposition based on editor position.y


########################################################################################


##  Friday 22th Sept 2023
+ [ FEATURE ] Working on Lexer


########################################################################################


## Thursday 21th Sept 2023
+ [ FEATURE ] (Ctrl+D) Find Next occurances of current word in file
+ [ FEATURE ] Auto-Indentation (Add '\t' if '{' is encountered on currentline)
+ [   BUG   ] Fix MiddleLines Not unindenting on SHIFT+TAB
+ [   BUG   ] Selecting Word and using CTRL+D causes error


########################################################################################


## Wednersday 20th Sept 2023
+ [ FEATURE ] Using TAB or SHIFT+TAB for indentation of currently selected line
+ [ FEATURE ] Highlighting all occurances of selected word in buffer



########################################################################################


## Tuesday 19th Sept 2023
+ [ FEATURE ] `ctrl+shift+(left/right)` arrow for word selection
+ [ FEATURE ] Copying/Cutting/Deleting Line Selection
+ [ FEATURE ] Pasting Multiline Text
+ [ FEATURE ] Snapping to nearest tab
+ [ FEATURE ] Scrolling To a Line Number
+ [ FEATURE ] Swapping Multiple Lines using `Ctrl+Shift+(up/down)`
+ [ FEATURE ] Handle Horizontal Scrolling using `shift+mouseWheel`


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