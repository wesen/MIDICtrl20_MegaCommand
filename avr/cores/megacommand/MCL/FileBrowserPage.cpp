#include "FileBrowserPage.h"
#include "MCL.h"

const menu_t file_menu PROGMEM = {
    "File",
    5,
    {
        {"NEW FOLDER", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
        {"DELETE", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
        {"RENAME", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
        {"OVERWRITE", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
        {"CANCEL", 0, 0, 0, (uint8_t *)NULL, (Page *)NULL, NULL, {}},
    },
    NULL,
    (Page *)NULL,
};

MCLEncoder file_menu_encoder(0, 1, ENCODER_RES_PAT);
MCLEncoder file_menu_encoder2(0, 4, ENCODER_RES_PAT);
MenuPage file_menu_page((menu_t *)&file_menu, &file_menu_encoder,
                        &file_menu_encoder2);

void FileBrowserPage::setup() {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
  classic_display = false;
  // char *mcl = ".mcl";
  // strcpy(match, mcl);
#endif
  strcpy(title, "Files");
  DEBUG_PRINT_FN();
}

void FileBrowserPage::add_entry(char *entry) {
  uint32_t pos = BANK1_FILE_ENTRIES_START + numEntries * 16;
  volatile uint8_t *ptr = (uint8_t *)pos;
  memcpy_bank1(ptr, entry, 16);
  numEntries++;
}

void FileBrowserPage::init() {
  DEBUG_PRINT_FN();
  char temp_entry[16];

  // config menu
  file_menu_page.menu.enable_entry(0, show_new_folder);
  file_menu_page.menu.enable_entry(1, true); // delete
  file_menu_page.menu.enable_entry(2, true); // rename
  file_menu_page.menu.enable_entry(3, show_overwrite);
  file_menu_page.menu.enable_entry(4, true); // cancel
  file_menu_encoder2.cur = file_menu_encoder2.old = 0;
  file_menu_encoder2.max = file_menu_page.menu.get_number_of_items() - 1;
  filemenu_active = false;

  int index = 0;
  //  reset directory pointer
  SD.vwd()->rewind();
  numEntries = 0;
  cur_file = 255;
  if (show_save) {
    char create_new[9] = "[ SAVE ]";
    add_entry(&create_new[0]);
  }

  char up_one_dir[3] = "..";
  SD.vwd()->getName(temp_entry, 16);
  DEBUG_DUMP(temp_entry);

  if ((show_parent) && !(strcmp(temp_entry, "/") == 0)) {
    add_entry(up_one_dir);
  }

  encoders[1]->cur = 1;

  //  iterate through the files
  while (file.openNext(SD.vwd(), O_READ) && (numEntries < MAX_ENTRIES)) {
    for (uint8_t c = 0; c < 16; c++) {
      temp_entry[c] = 0;
    }
    file.getName(temp_entry, 16);
    bool is_match_file = false;
    DEBUG_DUMP(temp_entry);
    if (temp_entry[0] == '.') {
      is_match_file = false;
    } else if (file.isDirectory() && show_dirs) {
      is_match_file = true;
    } else {
      // XXX only 3char suffix
      char *arg1 = &temp_entry[strlen(temp_entry) - 4];
      DEBUG_DUMP(arg1);
      if (strcmp(arg1, match) == 0) {
        is_match_file = true;
      }
    }
    if (is_match_file) {
      DEBUG_PRINTLN("file matched");
      add_entry(temp_entry);
      if (strcmp(temp_entry, mcl_cfg.project) == 0) {
        DEBUG_DUMP(temp_entry);
        DEBUG_DUMP(mcl_cfg.project);

        cur_file = numEntries - 1;
        encoders[1]->cur = numEntries - 1;
      }
    }
    index++;
    file.close();
    DEBUG_DUMP(numEntries);
  }

  if (numEntries <= 0) {
    numEntries = 0;
    ((MCLEncoder *)encoders[1])->max = 0;
  }
  ((MCLEncoder *)encoders[1])->max = numEntries - 1;
  DEBUG_PRINTLN("finished list files");
}

void FileBrowserPage::display() {
#ifdef OLED_DISPLAY
  if (filemenu_active) {
    oled_display.fillRect(0, 8, 38, 24, BLACK);
    file_menu_page.draw_menu(0, 14, 38);
    oled_display.display();
    return;
  }

  constexpr uint8_t x_offset = 43, y_offset = 8, width = MENU_WIDTH;
  oled_display.clearDisplay();
  oled_display.setFont(&TomThumb);
  oled_display.setCursor(0, 8);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.println(title);
  mcl_gui.draw_vertical_dashline(x_offset - 6);

  oled_display.setCursor(x_offset, 8);
  uint8_t max_items;
  if (numEntries > MAX_VISIBLE_ROWS) {
    max_items = MAX_VISIBLE_ROWS;
  } else {
    max_items = numEntries;
  }
  for (uint8_t n = 0; n < max_items; n++) {

    oled_display.setCursor(x_offset, y_offset + 8 * n);
    if (n == cur_row) {
      oled_display.setTextColor(BLACK, WHITE);
      oled_display.fillRect(oled_display.getCursorX() - 3,
                            oled_display.getCursorY() - 6, width, 7, WHITE);
    } else {
      oled_display.setTextColor(WHITE, BLACK);
      if (encoders[1]->cur - cur_row + n == cur_file) {
        oled_display.setCursor(x_offset - 4, y_offset + n * 8);
        oled_display.print(">");
      }
    }
    char temp_entry[16];
    uint16_t entry_num = encoders[1]->cur - cur_row + n;
    uint32_t pos = BANK1_FILE_ENTRIES_START + entry_num * 16;
    volatile uint8_t *ptr = (uint8_t *)pos;
    memcpy_bank1(temp_entry, ptr, 16);
    oled_display.println(temp_entry);
  }
  if (numEntries > MAX_VISIBLE_ROWS) {
    draw_scrollbar(120);
  }

  oled_display.display();
#else
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, title);
  GUI.setLine(GUI.LINE2);
  if (cur_file == encoders[1]->cur) {
    GUI.put_string_at_fill(0, ">");
  } else {
    GUI.put_string_at_fill(0, " ");
  }
  char temp_entry[17];
  uint16_t entry_num = encoders[1]->cur;
  uint32_t pos = BANK1_FILE_ENTRIES_START + entry_num * 16;
  volatile uint8_t *ptr = pos;
  memcpy_bank1(temp_entry, ptr, 16);
  temp_entry[16] = '\0';
  GUI.put_string_at(1, temp_entry);

#endif
  return;
}

void FileBrowserPage::draw_scrollbar(uint8_t x_offset) {
#ifdef OLED_DISPLAY
  mcl_gui.draw_vertical_scrollbar(x_offset, numEntries, MAX_VISIBLE_ROWS,
                                  encoders[1]->cur - cur_row);
#endif
}

void FileBrowserPage::loop() {

  if (filemenu_active) {
    file_menu_page.loop();
    return;
  }

  if (encoders[1]->hasChanged()) {

    uint8_t diff = encoders[1]->cur - encoders[1]->old;
    int8_t new_val = cur_row + diff;
#ifdef OLED_DISPLAY
    if (new_val > MAX_VISIBLE_ROWS - 1) {
      new_val = MAX_VISIBLE_ROWS - 1;
    }
    if (new_val < 0) {
      new_val = 0;
    }
#endif
    // MD.assignMachine(0, encoders[1]->cur);
    cur_row = new_val;
  }
}

bool FileBrowserPage::create_folder() {
  char new_dir[17] = "new_folder      ";
  if (mcl_gui.wait_for_input(new_dir, "Create Folder", 8)) {
    for (uint8_t n = 0; n < strlen(new_dir); n++) {
      if (new_dir[n] == ' ') {
        new_dir[n] = '\0';
      }
    }
    SD.mkdir(new_dir);
    init();
  }
  return true;
}

void FileBrowserPage::_calcindices(int &saveidx) {
  saveidx = show_save ? 0 : -1;
}

void FileBrowserPage::_cd_up() {
  char dir_entry[16];
  file.close();
  SD.chdir(lwd);

  SD.vwd()->getName(dir_entry, 16);
  auto len_lwd = strlen(lwd);
  auto len_dir_entry = strlen(dir_entry);

  // trim ending '/'
  if (lwd[len_lwd - 1] == '/') {
    lwd[--len_lwd] = '\0';
  }
  if (dir_entry[len_dir_entry - 1] == '/') {
    dir_entry[--len_dir_entry] = '\0';
  }

  lwd[len_lwd - len_dir_entry] = '\0';
  DEBUG_DUMP(dir_entry);
  DEBUG_DUMP(lwd);

  init();
}

void FileBrowserPage::_cd(const char *child) {
  char dir_entry[16];
  file.close();
  SD.vwd()->getName(dir_entry, 16);
  strcat(lwd, dir_entry);
  if (dir_entry[strlen(dir_entry) - 1] != '/') {
    strcat(lwd, "/");
  }
  DEBUG_DUMP(lwd);
  DEBUG_DUMP(child);
  SD.chdir(child);
  init();
}

void FileBrowserPage::_handle_filemenu() {
  char buf1[16];
  uint32_t pos = BANK1_FILE_ENTRIES_START + encoders[1]->getValue() * 16;
  volatile uint8_t *ptr = (uint8_t *)pos;
  memcpy_bank1(&buf1[0], ptr, 16);

  char buf2[32] = {'\0'};

  switch (file_menu_page.menu.get_item_index(file_menu_encoder2.cur)) {
  case 0: // new folder
    create_folder();
    break;
  case 1: // delete
    strcat(buf2, "Delete ");
    strcat(buf2, buf1);
    strcat(buf2, "?");
    if (mcl_gui.wait_for_confirm("CONFIRM", buf2)) {
      on_delete(buf1);
    }
    break;
  case 2: // overwrite
    strcat(buf2, "Overwrite ");
    strcat(buf2, buf1);
    strcat(buf2, "?");
    if (mcl_gui.wait_for_confirm("CONFIRM", buf2)) {
      file.open(buf1, O_READ);
      on_select(buf1);
    }
    break;
  case 3:
    strcat(buf2, buf1);
    if (mcl_gui.wait_for_input(buf2, "RENAME TO:", 16)) {
      on_rename(buf1, buf2);
    }
    break;
  }
}

bool FileBrowserPage::handleEvent(gui_event_t *event) {

  DEBUG_PRINT_FN();

  if (note_interface.is_event(event)) {
    return false;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    filemenu_active = true;
    encoders[0] = &file_menu_encoder;
    encoders[1] = &file_menu_encoder2;
    file_menu_page.init();
    return false;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    filemenu_active = false;
    encoders[0] = param1;
    encoders[1] = param2;
    _handle_filemenu();
    return false;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {

    int i_save;
    _calcindices(i_save);

    if (encoders[1]->getValue() == i_save) {
      on_new();
      return true;
    }

    char temp_entry[16];
    uint32_t pos = BANK1_FILE_ENTRIES_START + encoders[1]->getValue() * 16;
    volatile uint8_t *ptr = (uint8_t *)pos;
    memcpy_bank1(&temp_entry[0], ptr, 16);

    // chdir to parent
    if ((temp_entry[0] == '.') && (temp_entry[1] == '.')) {
      _cd_up();
      return false;
    }

    DEBUG_DUMP(temp_entry);
    file.open(temp_entry, O_READ);

    // chdir to child
    if (file.isDirectory()) {
      _cd(temp_entry);
      return false;
    }

    // select an entry
    on_select(temp_entry);
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
    // TODO shift menu
    // TODO delete
    // TODO rename
    // TODO copy/paste
  }

  // cancel
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON3) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    on_cancel();
    return true;
  }

  return false;
}
