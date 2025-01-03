@ECHO OFF

REM  version.bat -- Create a version.h header file from the git repository.
REM
REM  This batch file is called in the ".Before" rule of each WPJ project file.

SET VERSION_H=version.h
SET FORMAT=format:
SET FORMAT=%FORMAT%#define COMMIT_AUTHOR_EMAIL \"%%ae\"%%n
SET FORMAT=%FORMAT%#define COMMIT_AUTHOR_NAME  \"%%an\"%%n
SET FORMAT=%FORMAT%#define COMMIT_HASH_STRING  \"0x%%h\"%%n
SET FORMAT=%FORMAT%#define MAINTAINED_BY       \"%%ae (%%an)\"%%n

git rev-parse --is-inside-work-tree >NUL
IF ERRORLEVEL 1 GOTO GENERATE_DUMMY

git log HEAD -n1 --format="%FORMAT%" --output="%VERSION_H%"
GOTO END

:GENERATE_DUMMY
ECHO #define COMMIT_AUTHOR_EMAIL ""             >"%VERSION_H%"
ECHO #define COMMIT_AUTHOR_NAME  "local build" >>"%VERSION_H%"
ECHO #define COMMIT_HASH_STRING  "0x0000000"   >>"%VERSION_H%"
ECHO #define MAINTAINED_BY       "nobody"      >>"%VERSION_H%"

:END
