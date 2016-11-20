// Copyright (C) 2016 David Reid. See included LICENSE file.

#define TA_GUI_GADGET_TYPE_ROOT             0
#define TA_GUI_GADGET_TYPE_BUTTON           1
#define TA_GUI_GADGET_TYPE_LISTBOX          2
#define TA_GUI_GADGET_TYPE_TEXTBOX          3
#define TA_GUI_GADGET_TYPE_SCROLLBAR        4
#define TA_GUI_GADGET_TYPE_LABEL            5
#define TA_GUI_GADGET_TYPE_SURFACE          6
#define TA_GUI_GADGET_TYPE_FONT             7
#define TA_GUI_GADGET_TYPE_PICTURE          12

#define TA_GUI_SCROLLBAR_TYPE_HORIZONTAL    1
#define TA_GUI_SCROLLBAR_TYPE_VERTICAL      2

// TODO: Change these strings to dynamic strings. Can allocate these from a single pool.
typedef struct
{
    // COMMON
    ta_int32 id;
    ta_int32 assoc;
    char name[128];
    ta_int32 xpos;
    ta_int32 ypos;
    ta_int32 width;
    ta_int32 height;
    ta_int32 attribs;   // For scrollbars, 1 = horizontal and 2 = vertical.
    ta_int32 colorf;
    ta_int32 colorb;
    ta_int32 texturenumber;
    ta_int32 fontnumber;
    ta_bool32 active;
    ta_int32 commonattribs;
    char help[128];

    union
    {
        struct  // id = 0
        {
            ta_int32 totalgadgets;
            char panel[128];
            char crdefault[128];
            char escdefault[128];
            char defaultfocus[128];
        } root;

        struct   // id = 1
        {
            ta_int32 status;        // The frame inside the GAF file the button starts on. Should always be 0 for multi-stage buttons.
            char text[128];         // For multi-stage buttons, the text for each stage is separated with '|'.
            ta_uint32 quickkey;     // Shortcut key. ASCII.
            ta_bool32 grayedout;    // Whether or not the button is disabled, but still visible.
            ta_int32 stages;        // The number of stages in a multi-stage button. Set to 0 for simple buttons.
        } button;

        struct  // id = 2
        {
            int unused;
        } listbox;

        struct  // id = 3
        {
            ta_int32 maxchars;
        } textbox;

        struct  // id = 4
        {
            ta_int32 range;
            ta_int32 thick;
            ta_int32 knobpos;   // "knob" is the scrollbar's thumb.
            ta_int32 knobsize;
        } scrollbar;

        struct  // id = 5
        {
            char text[128];
            char link[128];     // For linking the label to a button. It just means that when the label is clicked it is the same as pressing the linked button.
        } label;

        struct  // id = 6
        {
            ta_bool32 hotornot;
        } surface;

        struct  // id = 7
        {
            char filename[128]; // Should not include the "fonts/" section or the extension.
        } font;

        struct
        {
            int unused;
        } picture;
    };
} ta_gui_gadget;

struct ta_gui
{
    ta_game* pGame;
};

ta_result ta_gui_load(ta_game* pGame, const char* filePath, ta_gui* pGUI);
ta_result ta_gui_unload(ta_gui* pGUI);