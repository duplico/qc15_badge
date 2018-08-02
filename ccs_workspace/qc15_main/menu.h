/// Declarations for status and control menus.
/**
 **
 ** \file menu.c
 ** \author George Louthan
 ** \date   2018
 ** \copyright (c) 2018 George Louthan @duplico. MIT License.
 */



#ifndef MENU_H_
#define MENU_H_

void enter_menu();
void enter_menu_status();
void enter_menu_controller();
void status_handle_loop();
void controller_handle_loop();
void status_render_choice();

#endif /* MENU_H_ */
