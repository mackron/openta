// Copyright (C) 2018 David Reid. See included LICENSE file.

// What I've learned about the <attribs> property for gadgets
// ==========================================================
// It appears <attribs> is a bit field.
//
// Buttons
// -------
// - Bit 2 seems to be always set, but not sure what it means.
// - Bit 5 I _think_ is used to indicate that it's a toggle button where it stays pressed when clicked
//   and then un-presses when another button in the same group is pressed. You can see it in action in
//   the New Game menu with the Core and Arm side selection buttons.
// - Bit 11 I _think_ is used to disable keyboard focus.
//
//
// Scrollbars
// ----------
// 1 = Horizontal; 2 = Vertical (thanks to http://units.tauniverse.com/tutorials/tadesign)

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

#define TA_GUI_BUTTON_STATE_NORMAL          0
#define TA_GUI_BUTTON_STATE_PRESSED         1
#define TA_GUI_BUTTON_STATE_DISABLED        2

#define TA_GUI_EVENT_TYPE_BUTTON_PRESSED    1
#define TA_GUI_EVENT_TYPE_SCROLL_UP         2
#define TA_GUI_EVENT_TYPE_SCROLL_DOWN       3

#define TA_GUI_GADGET_ATTRIB_STICKY_BUTTON  (1 << 4)
#define TA_GUI_GADGET_ATTRIB_SKIP_FOCUS     (1 << 10)

// TODO: Change these strings to dynamic strings. Can allocate these from a single pool.
typedef struct
{
    taBool32 isHeld;   // Whether or not the user has clicked on a control but not released it yet.
    taUInt32 heldMB;   // The mouse button that triggered the hold.

    // [COMMON]
    taInt32 id;
    taInt32 assoc;
    char* name;
    taInt32 xpos;
    taInt32 ypos;
    taInt32 width;
    taInt32 height;
    taInt32 attribs;   // For scrollbars, 1 = horizontal and 2 = vertical.
    taInt32 colorf;
    taInt32 colorb;
    taInt32 texturenumber;
    taInt32 fontnumber;
    taBool32 active;
    taInt32 commonattribs;
    char* help;

    union
    {
        struct  // id = 0
        {
            taInt32 totalgadgets;
            char* panel;
            char* crdefault;
            char* escdefault;
            char* defaultfocus;
            struct
            {
                taInt32 major;
                taInt32 minor;
                taInt32 revision;
            } version;
        } root;

        struct   // id = 1
        {
            taInt32 status;        // The frame inside the GAF file the button starts on. Should always be 0 for multi-stage buttons.
            char* text;             // For multi-stage buttons, the text for each stage is separated with '|'.
            taUInt32 quickkey;     // Shortcut key. ASCII.
            taBool32 grayedout;    // Whether or not the button is disabled, but still visible.
            taInt32 stages;        // The number of stages in a multi-stage button. Set to 0 for simple buttons.

            // ...
            ta_gaf_texture_group* pBackgroundTextureGroup;
            taUInt32 iBackgroundFrame; // +0 for normal, +1 for pressed, +2 for disabled.
            taUInt32 currentStage;     // Used by multi-stage buttons.
        } button;

        struct  // id = 2
        {
            char** pItems;
            taUInt32 itemCount;
            taUInt32 iSelectedItem;    // The index of the currently selected item. Set to -1 if nothing is slected.
            taUInt32 scrollPos;
            taUInt32 pageSize;         // The number of items that can fit on one page of the list box. 
        } listbox;

        struct  // id = 3
        {
            taInt32 maxchars;
        } textbox;

        struct  // id = 4
        {
            taInt32 range;
            taInt32 thick;
            taInt32 knobpos;   // "knob" is the scrollbar's thumb.
            taInt32 knobsize;

            // ...
            ta_gaf_texture_group* pTextureGroup;
            taUInt32 iArrow0Frame;     // UP/LEFT arrow
            taUInt32 iArrow1Frame;     // DOWN/RIGHT arrow
            taUInt32 iTrackBegFrame;   // TOP/LEFT end of the track.
            taUInt32 iTrackEndFrame;   // RIGHT/BOTTOM end of the track.
            taUInt32 iTrackMidFrame;   // The middle piece of the track.
            taUInt32 iThumbFrame;
            taUInt32 iThumbCapTopFrame;
            taUInt32 iThumbCapBotFrame;
        } scrollbar;

        struct  // id = 5
        {
            char* text;
            char* link;     // For linking the label to a button. It just means that when the label is clicked it is the same as pressing the linked button.

            // ...
            taUInt32 iLinkedGadget;     // Set to -1 if there is no link.
        } label;

        struct  // id = 6
        {
            taBool32 hotornot;
        } surface;

        struct  // id = 7
        {
            char* filename; // Should not include the "fonts/" section or the extension.
        } font;

        struct
        {
            int unused;
        } picture;
    };
} ta_gui_gadget;

struct ta_gui
{
    taEngineContext* pEngine;
    ta_gaf_texture_group textureGroupGAF;
    taBool32 hasGAF;
    ta_texture* pBackgroundTexture;

    taUInt32 gadgetCount;
    ta_gui_gadget* pGadgets;    // This is an offset of _pPayload.
    taUInt32 heldGadgetIndex;
    taUInt32 hoveredGadgetIndex;
    taUInt32 focusedGadgetIndex;

    // Memory for each GUI is allocated in one big chunk which is stored in this buffer.
    taUInt8* _pPayload;
};

ta_result ta_gui_load(taEngineContext* pEngine, const char* filePath, ta_gui* pGUI);
ta_result ta_gui_unload(ta_gui* pGUI);

// Retrieves information about how the GUI is mapped to the screen of a specific resolution.
//
// Most GUIs are built based on a 640x480 resolution. When a GUI is drawn on the screen that is of a different resolution to this,
// it needs to be scaled. This function is used to retrieve the necessary information needed to draw the GUI at a given screen
// resolution.
ta_result ta_gui_get_screen_mapping(ta_gui* pGUI, taUInt32 screenSizeX, taUInt32 screenSizeY, float* pScale, float* pOffsetX, float* pOffsetY);

// Converts a position in screen coordinates to GUI coordinates.
ta_result ta_gui_map_screen_position(ta_gui* pGUI, taUInt32 screenSizeX, taUInt32 screenSizeY, taInt32 screenPosX, taInt32 screenPosY, taInt32* pGUIPosX, taInt32* pGUIPosY);

// Finds the gadget under the given point, in GUI coordinates. Returns false if the mouse is not under any gadget. This will
// include the root gadget.
taBool32 ta_gui_get_gadget_under_point(ta_gui* pGUI, taInt32 posX, taInt32 posY, taUInt32* pGadgetIndex);

// Marks the given gadget as held.
ta_result ta_gui_hold_gadget(ta_gui* pGUI, taUInt32 gadgetIndex, taUInt32 mouseButton);

// Releases the hold on the given gadget.
ta_result ta_gui_release_hold(ta_gui* pGUI, taUInt32 gadgetIndex);

// Retrieves the index of the gadget that's currently being held. Returns false if no gadget is held; true otherwise.
taBool32 ta_gui_get_held_gadget(ta_gui* pGUI, taUInt32* pGadgetIndex);

// Finds a gadget by it's name.
taBool32 ta_gui_find_gadget_by_name(ta_gui* pGUI, const char* name, taUInt32* pGadgetIndex);

// Retrieves the focused gadget, if any.
taBool32 ta_gui_get_focused_gadget(ta_gui* pGUI, taUInt32* pGadgetIndex);

// Gives keyboard focus to the next gadget.
void ta_gui_focus_next_gadget(ta_gui* pGUI);

// Gives keyboard focus to the previous gadget.
void ta_gui_focus_prev_gadget(ta_gui* pGUI);

// Retrieves the text for the given button for the given stage. Returns null if an error occurs.
const char* ta_gui_get_button_text(ta_gui_gadget* pGadget, taUInt32 stage);

// Sets the items in a listbox gadget.
//
// This will make it's own local copy of each item.
ta_result ta_gui_set_listbox_items(ta_gui_gadget* pGadget, const char** pItems, taUInt32 count);

// Retrieves the text of the listbox item at the given index.
const char* ta_gui_get_listbox_item(ta_gui_gadget* pGadget, taUInt32 index);


typedef struct
{
    taUInt32 frameIndex;
} ta_common_gui_texture_button;

typedef struct
{
    taEngineContext* pEngine;
    ta_gaf_texture_group textureGroup;

    // Button backgrounds for each size.
    ta_common_gui_texture_button buttons[6];

    struct
    {
        taUInt32 arrowUpFrameIndex;
        taUInt32 arrowUpPressedFrameIndex;
        taUInt32 arrowDownFrameIndex;
        taUInt32 arrowDownPressedFrameIndex;
        taUInt32 arrowLeftFrameIndex;
        taUInt32 arrowLeftPressedFrameIndex;
        taUInt32 arrowRightFrameIndex;
        taUInt32 arrowRightPressedFrameIndex;
        taUInt32 trackTopFrameIndex;
        taUInt32 trackBottomFrameIndex;
        taUInt32 trackMidVertFrameIndex;
        taUInt32 trackLeftFrameIndex;
        taUInt32 trackRightFrameIndex;
        taUInt32 trackMidHorzFrameIndex;
        taUInt32 thumbVertFrameIndex;
        taUInt32 thumbHorzFrameIndex;
        taUInt32 thumbCapTopFrameIndex;
        taUInt32 thumbCapBotFrameIndex;
    } scrollbar;
} ta_common_gui;

ta_result ta_common_gui_load(taEngineContext* pEngine, ta_common_gui* pCommonGUI);
ta_result ta_common_gui_unload(ta_common_gui* pCommonGUI);
ta_result ta_common_gui_get_button_frame(ta_common_gui* pCommonGUI, taUInt32 width, taUInt32 height, taUInt32* pFrameIndex);
ta_result ta_common_gui_get_multistage_button_frame(ta_common_gui* pCommonGUI, taUInt32 stages, taUInt32* pFrameIndex);
