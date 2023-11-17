@echo off
for /f "delims=" %%i in ('git rev-parse --abbrev-ref HEAD 2^>nul') do set BRANCH=%%i
echo Current Git branch: %BRANCH%
   
    
     
