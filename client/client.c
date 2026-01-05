#include "ui.h"

//TODO
//implement loading from file option from mainMenu
//improve UI/UX
//change all user inputs to read char and then transform to desired data, otherwise read will get bugged

//bug in load_game when trying to print error messages e.g. typing 'a' in port and it doesn't print any error. Just waits for another input.

/**
 * Starts client
 */
int main(int argc, char** argv) {
  main_menu();
  return 0;
}

