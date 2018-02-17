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
    ta_bool32 isHeld;   // Whether or not the user has clicked on a control but not released it yet.
    ta_uint32 heldMB;   // The mouse button that triggered the hold.

    // [COMMON]
    ta_int32 id;
    ta_int32 assoc;
    char* name;
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
    char* help;

    union
    {
        struct  // id = 0
        {
            ta_int32 totalgadgets;
            char* panel;
            char* crdefault;
            char* escdefault;
            char* defaultfocus;
            struct
            {
                ta_int32 major;
                ta_int32 minor;
                ta_int32 revision;
            } version;
        } root;

        struct   // id = 1
        {
            ta_int32 status;        // The frame inside the GAF file the button starts on. Should always be 0 for multi-stage buttons.
            char* text;             // For multi-stage buttons, the text for each stage is separated with '|'.
            ta_uint32 quickkey;     // Shortcut key. ASCII.
            ta_bool32 grayedout;    // Whether or not the button is disabled, but still visible.
            ta_int32 stages;        // The number of stages in a multi-stage button. Set to 0 for simple buttons.

            // ...
            ta_gaf_texture_group* pBackgroundTextureGroup;
            ta_uint32 iBackgroundFrame; // +0 for normal, +1 for pressed, +2 for disabled.
            ta_uint32 currentStage;     // Used by multi-stage buttons.
        } button;

        struct  // id = 2
        {
            char** pItems;
            ta_uint32 itemCount;
            ta_uint32 iSelectedItem;    // The index of the currently selected item. Set to -1 if nothing is slected.
            ta_uint32 scrollPos;
            ta_uint32 pageSize;         // The number of items that can fit on one page of the list box. 
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

            // ...
            ta_gaf_texture_group* pTextureGroup;
            ta_uint32 iArrow0Frame;     // UP/LEFT arrow
            ta_uint32 iArrow1Frame;     // DOWN/RIGHT arrow
            ta_uint32 iTrackBegFrame;   // TOP/LEFT end of the track.
            ta_uint32 iTrackEndFrame;   // RIGHT/BOTTOM end of the track.
            ta_uint32 iTrackMidFrame;   // The middle piece of the track.
            ta_uint32 iThumbFrame;
            ta_uint32 iThumbCapTopFrame;
            ta_uint32 iThumbCapBotFrame;
        } scrollbar;

        struct  // id = 5
        {
            char* text;
            char* link;     // For linking the label to a button. It just means that when the label is clicked it is the same as pressing the linked button.

            // ...
            ta_uint32 iLinkedGadget;     // Set to -1 if there is no link.
        } label;

        struct  // id = 6
        {
            ta_bool32 hotornot;
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
    ta_game* pGame;
    ta_gaf_texture_group textureGroupGAF;
    ta_bool32 hasGAF;
    ta_texture* pBackgroundTexture;

    ta_uint32 gadgetCount;
    ta_gui_gadget* pGadgets;    // This is an offset of _pPayload.
    ta_uint32 heldGadgetIndex;
    ta_uint32 hoveredGadgetIndex;
    ta_uint32 focusedGadgetIndex;

    // Memory for each GUI is allocated in one big chunk which is stored in this buffer.
    ta_uint8* _pPayload;
};

ta_result ta_gui_load(ta_game* pGame, const char* filePath, ta_gui* pGUI);
ta_result ta_gui_unload(ta_gui* pGUI);

// Retrieves information about how the GUI is mapped to the screen of a specific resolution.
//
// Most GUIs are built based on a 640x480 resolution. When a GUI is drawn on the screen that is of a different resolution to this,
// it needs to be scaled. This function is used to retrieve the necessary information needed to draw the GUI at a given screen
// resolution.
ta_result ta_gui_get_screen_mapping(ta_gui* pGUI, ta_uint32 screenSizeX, ta_uint32 screenSizeY, float* pScale, float* pOffsetX, float* pOffsetY);

// Converts a position in screen coordinates to GUI coordinates.
ta_result ta_gui_map_screen_position(ta_gui* pGUI, ta_uint32 screenSizeX, ta_uint32 screenSizeY, ta_int32 screenPosX, ta_int32 screenPosY, ta_int32* pGUIPosX, ta_int32* pGUIPosY);

// Finds the gadget under the given point, in GUI coordinates. Returns false if the mouse is not under any gadget. This will
// include the root gadget.
ta_bool32 ta_gui_get_gadget_under_point(ta_gui* pGUI, ta_int32 posX, ta_int32 posY, ta_uint32* pGadgetIndex);

// Marks the given gadget as held.
ta_result ta_gui_hold_gadget(ta_gui* pGUI, ta_uint32 gadgetIndex, ta_uint32 mouseButton);

// Releases the hold on the given gadget.
ta_result ta_gui_release_hold(ta_gui* pGUI, ta_uint32 gadgetIndex);

// Retrieves the index of the gadget that's currently being held. Returns false if no gadget is held; true otherwise.
ta_bool32 ta_gui_get_held_gadget(ta_gui* pGUI, ta_uint32* pGadgetIndex);

// Finds a gadget by it's name.
ta_bool32 ta_gui_find_gadget_by_name(ta_gui* pGUI, const char* name, ta_uint32* pGadgetIndex);

// Retrieves the focused gadget, if any.
ta_bool32 ta_gui_get_focused_gadget(ta_gui* pGUI, ta_uint32* pGadgetIndex);

// Gives keyboard focus to the next gadget.
void ta_gui_focus_next_gadget(ta_gui* pGUI);

// Gives keyboard focus to the previous gadget.
void ta_gui_focus_prev_gadget(ta_gui* pGUI);

// Retrieves the text for the given button for the given stage. Returns null if an error occurs.
const char* ta_gui_get_button_text(ta_gui_gadget* pGadget, ta_uint32 stage);

// Sets the items in a listbox gadget.
//
// This will make it's own local copy of each item.
ta_result ta_gui_set_listbox_items(ta_gui_gadget* pGadget, const char** pItems, ta_uint32 count);

// Retrieves the text of the listbox item at the given index.
const char* ta_gui_get_listbox_item(ta_gui_gadget* pGadget, ta_uint32 index);


typedef struct
{
    ta_uint32 frameIndex;
} ta_common_gui_texture_button;

typedef struct
{
    ta_game* pGame;
    ta_gaf_texture_group textureGroup;

    // Button backgrounds for each size.
    ta_common_gui_texture_button buttons[6];

    struct
    {
        ta_uint32 arrowUpFrameIndex;
        ta_uint32 arrowUpPressedFrameIndex;
        ta_uint32 arrowDownFrameIndex;
        ta_uint32 arrowDownPressedFrameIndex;
        ta_uint32 arrowLeftFrameIndex;
        ta_uint32 arrowLeftPressedFrameIndex;
        ta_uint32 arrowRightFrameIndex;
        ta_uint32 arrowRightPressedFrameIndex;
        ta_uint32 trackTopFrameIndex;
        ta_uint32 trackBottomFrameIndex;
        ta_uint32 trackMidVertFrameIndex;
        ta_uint32 trackLeftFrameIndex;
        ta_uint32 trackRightFrameIndex;
        ta_uint32 trackMidHorzFrameIndex;
        ta_uint32 thumbVertFrameIndex;
        ta_uint32 thumbHorzFrameIndex;
        ta_uint32 thumbCapTopFrameIndex;
        ta_uint32 thumbCapBotFrameIndex;
    } scrollbar;
} ta_common_gui;

ta_result ta_common_gui_load(ta_game* pGame, ta_common_gui* pCommonGUI);
ta_result ta_common_gui_unload(ta_common_gui* pCommonGUI);
ta_result ta_common_gui_get_button_frame(ta_common_gui* pCommonGUI, ta_uint32 width, ta_uint32 height, ta_uint32* pFrameIndex);
ta_result ta_common_gui_get_multistage_button_frame(ta_common_gui* pCommonGUI, ta_uint32 stages, ta_uint32* pFrameIndex);