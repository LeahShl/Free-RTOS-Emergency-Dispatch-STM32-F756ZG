# Free-RTOS-Emergency-Dispatch-STM32-F756ZG
An adaptation of my Free-RTOS-Emergency-Dispatch simulator project, made to work on a STM32 Nucleo-144 F756ZG board.

## The original project
Can be found at https://github.com/LeahShl/Free-RTOS-Emergency-Dispatch-Sim. 

## What's different?
  1. The original code was adapted into the board's `main.c`
  2. Priority values were changed to match CMSIS-OS2's range
  3. Queue size was reduced to accomodate for smaller heap size
  4. `perror` calls were replaced by `printf` calls, which are directed to uart3

## How to run this on my board?
  1. Clone this repo or straight up download is as a zip
  2. Open your Cube IDE
  3. Click `File` > `Open Projects from File System...`
  4. In `Import source` pick the project's directory or archive
  5. Click `Finish`
  6. Build and run the project on your board
