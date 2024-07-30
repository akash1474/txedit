[------------------------------ __TODO__ ---------------------------------]
# [ FEATURE ] Execute the terminal commands in background asynchronously.
# [ FEATURE ] Implement Context Menu Options for files and folders
# [ FEATURE ] Opening new file in new tab
<!-- # [ FEATURE ] Smooth Scrolling -->
# [ FEATURE ] Render Image for image file
# [ FEATURE ] Syntax Highlighting
<!-- # [ FEATURE ] Using Custum Title Bar -->
# [ FEATURE ] Support For UTF-8 characters
# [ FEATURE ] Search and Replace
# [ FEATURE ] File Operations
# [ FEATURE ] Tabs and Split Views
# [ FEATURE ] Full-Screen Mode
# [ FEATURE ] Auto-Save and Backup


## 28th July 2024
+ [ UPGRADE ] Added support for OpenGL 3.3
+ [ REFACTOR ] Separating core app initialization logic to Application.cpp
+ [ UPGRADE ] Switched to ImGui 1.90.9 from 1.89.9

## 27th July 2024
+ [   BUG   ] Fixed cursor type on scrollbar of editor
+ [ FEATURE ] Added SelectAll Feature


## 16th Dec 2023
+ [ UPGRADE ] Improve API


## 4th Dec 2023
+ [ FEATURE ] Highlight opening and closing end of brackets,quotations


## 27th Nov 2023
+ [   BUG   ] Fix Surround when tabs present and support for multiple cursors


## 20th-22nd Nov 2023
+ [   BUG   ] Fixed Scroll when cursor goes off screen while pressing enter
+ [   BUG   ] Fixed Line Render after some gap at top
+ [ FEATURE ] Surround selected word with Quotations/Brackets when clicked
+ [ FEATURE ] Added a Toolbar
+ [ FEATURE ] Set a default ini file location



## Friday 19th Nov 2023
+ [ FEATURE ] Drag and Drop File and Folders
+ [ FEATURE ] Show File Extension in Status Bar
+ [ FEATURE ] Take cmd arguments like folder/file path
+ [ FEATURE ] Support OpenWith Feature


## Friday 18th Nov 2023
+ [ FEATURE ] Place Cursor at 0,0 when loading a file


## Friday 15th - 17th Nov 2023
+ [ FEATURE ] Color Scheme Improvement
+ [ FEATURE ] File Saving



## Tuesday 14th Nov 2023
+ [ FEATURE ] Status Bar containing (lineNo,colNo,gitbranch,tabsize)
+ [   BUG   ] Update window size when new file is opened



## Monday 13th Nov 2023
+ [ FEATURE ] File Explorer Prototype (Selecting folder,selecting file, tree branching, file save)




## Sunday 14th Oct 2023
+ [   BUG   ] Fixing Quotations both single and double Quotations




##  Saturday 14th Oct 2023
+ [   BUG   ] Multi-Cursor '{' issue -- only adds '}' on mCursors[0]
+ [   BUG   ] Multi-Cursor sometime characters insert only on mCursors[0]
+ [ FEATURE ] Using Arrow Keys in Multi-Cursor
+ [ FEATURE ] Add Delete KeyBinding
+ [   BUG   ] Ctrl+Shift Multi-Cursor Selection




##  Thursday 7th Oct 2023

+ [ FEATURE ] Using Ctrl+Click to place multiple cursors
+ [ FEATURE ] Ctrl+D Selection and Editing at multiple positions




##  Sunday 24th Sept 2023
+ [   BUG   ] Scrolling when left and right key pressed at start and end of line
+ [   BUG   ] Fix Horizontal Scrolling Amount (Content Partly Clipped)
+ [ FEATURE ] Scroll if word selection using mouse and mouse out of bounds
+ [ FEATURE ] Scroll To Cursor Position if cursor not visible
+ [ FEATURE ] Render a line for indentation





##  Saturday 23th Sept 2023
+ [ IMPROVEMENT ] Improved Carriage Return and Auto indentation
+ [ IMPROVEMENT ] Lexer
+ [     BUG     ] Fix Last Line length
+ [     BUG     ] Calculate lineposition based on editor position.y





##  Friday 22th Sept 2023
+ [ FEATURE ] Working on Lexer





## Thursday 21th Sept 2023
+ [ FEATURE ] (Ctrl+D) Find Next occurances of current word in file
+ [ FEATURE ] Auto-Indentation (Add '\t' if '{' is encountered on currentline)
+ [   BUG   ] Fix MiddleLines Not unindenting on SHIFT+TAB
+ [   BUG   ] Selecting Word and using CTRL+D causes error





## Wednersday 20th Sept 2023
+ [ FEATURE ] Using TAB or SHIFT+TAB for indentation of currently selected line
+ [ FEATURE ] Highlighting all occurances of selected word in buffer






## Tuesday 19th Sept 2023
+ [ FEATURE ] `ctrl+shift+(left/right)` arrow for word selection
+ [ FEATURE ] Copying/Cutting/Deleting Line Selection
+ [ FEATURE ] Pasting Multiline Text
+ [ FEATURE ] Snapping to nearest tab
+ [ FEATURE ] Scrolling To a Line Number
+ [ FEATURE ] Swapping Multiple Lines using `Ctrl+Shift+(up/down)`
+ [ FEATURE ] Handle Horizontal Scrolling using `shift+mouseWheel`





## Monday 18th Sept 2023
+ [   BUG   ] `mLinePosition.y` changes while Scrolling
+ [ FEATURE ] Multiline selection
+ [ FEATURE ] Line Selection Triple Click
+ [ FEATURE ] Mouse Drag and Selection
+ [ UPGRADE ] Show one width for empty line selection





## Sunday 17th Sept 2023
+ [   BUG   ] insertion error when tabs ahead of `mState.mCursorPosition.mColumn`
+ [ FEATURE ] `shift+(left/right)` arrow for letter selection
+ [ FEATURE ] Scrolling with cursor position
+ [ FEATURE ] Add a Left Padding parameter to editor text
+ [   BUG   ] Fix Line Number Bar width based on max lines
+ [ FEATURE ] Build a `std::map` of theme color
+ [ FEATURE ] Copying/Cutting/Pasting Word Selection





## Saturday 16th Sept 2023
+ [ FEATURE ] Added Word Selection using double click
+ [   BUG   ] Fixed while clicking on line selects neighbouring lines
+ [ FEATURE ] Handle when key pressed '{' and then '}' is pressed (aka ignore input for '}' after '{')
+ [ FEATURE ] Swapping Lines on `ctrl+shift+(up/down arrow)`
+ [ FEATURE ] `ctrl+(left/right)` arrow for word jump
+ [   BUG   ] Fix RightArrow and LeftArrow on '\t'





## Monday 11th Sept 2023
+ [ FEATURE ] Tabbing Support (Tab Key)
+ [ FEATURE ] Double Click - Word Selection Backend
+ [   BUG   ] Minor Fixes
+ [ FEATURE ] Double Paranthesis and Quotations 





## Sunday 10th Sept 2023
- [   BUG   ] Fix currentline selection on mouse click


                                                                                                                                                                   
                                                                                                                                                                       
                                                                                                                                                                            
                                                                                                                                                                               
                                                                                                                                                                                                   
