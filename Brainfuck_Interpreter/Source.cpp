#include <windows.h>
#include <stdio.h>
#include <string>
#include <vector>

int utf8_to_unicode(std::string utf8_code)
{
    unsigned utf8_size = utf8_code.length();
    int unicode = 0;

    for (unsigned p = 0; p < utf8_size; ++p)
    {
        int bit_count = (p ? 6 : 8 - utf8_size - (utf8_size == 1 ? 0 : 1)),
            shift = (p < utf8_size - 1 ? (6 * (utf8_size - p - 1)) : 0);

        for (int k = 0; k < bit_count; ++k)
            unicode += ((utf8_code[p] & (1 << k)) << shift);
    }

    return unicode;
}


int main(void)
{
    HANDLE hStdout, hNewScreenBuffer;
    SMALL_RECT srctReadRect;
    CHAR_INFO chiBuffer[160]; // [2][80];
    COORD coordBufSize;
    COORD coordBufCoord;
    BOOL fSuccess;

    // Get a handle to the STDOUT screen buffer to copy from and
    // create a new screen buffer to copy to.

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hNewScreenBuffer = CreateConsoleScreenBuffer(
        GENERIC_READ |           // read/write access
        GENERIC_WRITE,
        FILE_SHARE_READ |
        FILE_SHARE_WRITE,        // shared
        NULL,                    // default security attributes
        CONSOLE_TEXTMODE_BUFFER, // must be TEXTMODE
        NULL);                   // reserved; must be NULL
    if (hStdout == INVALID_HANDLE_VALUE ||
        hNewScreenBuffer == INVALID_HANDLE_VALUE)
    {
        printf("CreateConsoleScreenBuffer failed - (%d)\n", GetLastError());
        return 1;
    }

    // Make the new screen buffer the active screen buffer.

    if (!SetConsoleActiveScreenBuffer(hNewScreenBuffer))
    {
        printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
        return 1;
    }

    // Set the source rectangle.

    srctReadRect.Top = 0;    // top left: row 0, col 0
    srctReadRect.Left = 0;
    srctReadRect.Bottom = 1; // bot. right: row 1, col 79
    srctReadRect.Right = 79;

    // The temporary buffer size is 2 rows x 80 columns.

    coordBufSize.Y = 2;
    coordBufSize.X = 80;

    // The top left destination cell of the temporary buffer is
    // row 0, col 0.

    coordBufCoord.X = 0;
    coordBufCoord.Y = 0;

    // Copy the block from the screen buffer to the temp. buffer.

    fSuccess = ReadConsoleOutput(
        hStdout,        // screen buffer to read from
        chiBuffer,      // buffer to copy into
        coordBufSize,   // col-row size of chiBuffer
        coordBufCoord,  // top left dest. cell in chiBuffer
        &srctReadRect); // screen buffer source rectangle
    if (!fSuccess)
    {
        printf("ReadConsoleOutput failed - (%d)\n", GetLastError());
        return 1;
    }

    // Set the destination rectangle.


    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns, rows, nColumns, nRows;

    GetConsoleScreenBufferInfo(hStdout, &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    nColumns = columns;
    nRows = rows;

    SMALL_RECT srctWriteRect;

    CHAR_INFO empty;
    empty.Char.UnicodeChar = (char)32;
    empty.Attributes = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN;


    CHAR_INFO c;
    c.Char.UnicodeChar = utf8_to_unicode("a");
    c.Attributes = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN;

    PCHAR_INFO wChiBuffer = new CHAR_INFO[rows * columns] {c};

    int curPos = 0;

    COORD wBufSize;
    COORD wBufCoord;

    wBufSize.Y = rows;
    wBufSize.X = columns;

    wBufCoord.X = 0;
    wBufCoord.Y = 0;

    srctWriteRect.Top = 0;    // top lt: row 10, col 0
    srctWriteRect.Left = 0;
    srctWriteRect.Bottom = rows; // bot. rt: row 11, col 79
    srctWriteRect.Right = columns;

    //setup interpreter
    std::vector<int> mem(40, 0);
    int curMem = 0;


    //setup console input
    DWORD cNumRead, i, nEvents;
    INPUT_RECORD irInBuf[128];

    // Copy from the temporary buffer to the new screen buffer.
    while (true) {
        GetConsoleScreenBufferInfo(hNewScreenBuffer, &csbi);
        nColumns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        nRows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

        if (nColumns != columns || nRows != rows) {
            delete[] wChiBuffer;
            wChiBuffer = new CHAR_INFO[nColumns * nRows]{};

            wBufSize.Y = nRows;
            wBufSize.X = nColumns;
            srctWriteRect.Bottom = nRows;
            srctWriteRect.Right = nColumns;
        }

        columns = nColumns;
        rows = nRows;

        //read input
        GetNumberOfConsoleInputEvents(hNewScreenBuffer, &nEvents);

        if (nEvents > 0) {
            fSuccess = ReadConsole(
                hNewScreenBuffer,
                irInBuf,
                128,
                &cNumRead,
                NULL);
            if (!fSuccess) {
                printf("ReadConsoleInput failed - (%d)\n", GetLastError());
            }

            for (i = 0; i < cNumRead; i++) {
                if (irInBuf[i].EventType == KEY_EVENT) {
                    KEY_EVENT_RECORD key = irInBuf[i].Event.KeyEvent;
                    if (key.bKeyDown) {
                        switch (key.wVirtualKeyCode) {
                        case VK_LEFT:
                            mem[0]--;
                            break;
                        case VK_RIGHT:
                            mem[0]++;
                            break;
                        }
                    }
                }
            }
        }

        //draw mem

        PCHAR_INFO curCharPointer = wChiBuffer;

        for (int val : mem) {
            for (auto chr : std::to_string(nEvents)) {
                CHAR_INFO nChar;
                nChar.Char.UnicodeChar = utf8_to_unicode(std::string(1, chr));
                nChar.Attributes = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN;
                *curCharPointer = nChar;
                curCharPointer++;
            }
            *curCharPointer = empty;
            curCharPointer++;
        }


        fSuccess = WriteConsoleOutput(
            hNewScreenBuffer, // screen buffer to write to
            wChiBuffer,        // buffer to copy from
            wBufSize,     // col-row size of chiBuffer
            wBufCoord,    // top left src cell in chiBuffer
            &srctWriteRect);  // dest. screen buffer rectangle
        if (!fSuccess)
        {
            printf("WriteConsoleOutput failed - (%d)\n", GetLastError());
            return 1;
        }

        Sleep(1000);
    }

    // Restore the original active screen buffer.

    if (!SetConsoleActiveScreenBuffer(hStdout))
    {
        printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
        return 1;
    }

    return 0;
}