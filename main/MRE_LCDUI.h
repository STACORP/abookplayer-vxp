// ============================================================================
// File: MRE_LCDUI.h
// Description: J2ME MIDP 2.0 LCDUI Emulation Layer for MRE (MediaTek)
// ============================================================================

#ifndef _MRE_LCDUI_H_
#define _MRE_LCDUI_H_

#include "vmsys.h"
#include "vmio.h"
#include "vmgraph.h"
#include "vmtimer.h"
#include "vmstdlib.h"
#include "vmchset.h"

#define MAX_ITEMS 32
#define MAX_COMMANDS 4
#define FONT_HEIGHT 16

// Forward Declarations
class Displayable;
class Graphics;
class Item;

// ----------------------------------------------------------------------------
// MIDP Command Types & Commands
// ----------------------------------------------------------------------------
enum CommandType {
    CMD_SCREEN,
    CMD_BACK,
    CMD_CANCEL,
    CMD_OK,
    CMD_HELP,
    CMD_STOP,
    CMD_EXIT,
    CMD_ITEM
};

class Command {
public:
    VMWCHAR label[32];
    CommandType type;
    int priority;

    Command() : type(CMD_SCREEN), priority(0) { label[0] = 0; }
    Command(const char* aLabel, CommandType aType, int aPriority) : type(aType), priority(aPriority) {
        vm_ascii_to_ucs2(label, sizeof(label), (VMSTR)aLabel);
    }
};

class CommandListener {
public:
    virtual void commandAction(Command* c, Displayable* d) = 0;
};

// ----------------------------------------------------------------------------
// Graphics Context
// ----------------------------------------------------------------------------
class Graphics {
private:
    VMINT layer_handle;
    int transX;
    int transY;
    int clipX, clipY, clipW, clipH;

public:
    Graphics(VMINT layer) : layer_handle(layer), transX(0), transY(0) {
        clipX = 0;
        clipY = 0;
        clipW = vm_graphic_get_screen_width();
        clipH = vm_graphic_get_screen_height();
    }

    void translate(int x, int y) {
        transX += x;
        transY += y;
    }

    void setClip(int x, int y, int w, int h) {
        clipX = x + transX;
        clipY = y + transY;
        clipW = w;
        clipH = h;
        vm_graphic_set_layer_clip(layer_handle, (VMINT16)clipX, (VMINT16)clipY, 
                                  (VMINT16)(clipX + clipW), (VMINT16)(clipY + clipH));
    }

    void resetClip() {
        clipX = 0;
        clipY = 0;
        clipW = vm_graphic_get_screen_width();
        clipH = vm_graphic_get_screen_height();
        vm_graphic_reset_clip();
    }

    void setColor(VMUINT8 r, VMUINT8 g, VMUINT8 b) {
        vm_graphic_color c;
        c.vm_color_565 = VM_COLOR_888_TO_565(r, g, b);
        c.vm_color_888 = (r << 16) | (g << 8) | b;
        vm_graphic_setcolor(&c);
    }

    void fillRect(int x, int y, int w, int h) {
        vm_graphic_fill_rect_ex(layer_handle, x + transX, y + transY, w, h);
    }

    void drawRect(int x, int y, int w, int h) {
        vm_graphic_rect_ex(layer_handle, x + transX, y + transY, w, h);
    }

    void drawLine(int x1, int y1, int x2, int y2) {
        vm_graphic_line_ex(layer_handle, x1 + transX, y1 + transY, x2 + transX, y2 + transY);
    }

    void drawString(const VMWCHAR* str, int x, int y) {
        vm_graphic_textout_to_layer(layer_handle, x + transX, y + transY, (VMWSTR)str, vm_wstrlen((VMWSTR)str));
    }
};

// ----------------------------------------------------------------------------
// Base Item
// ----------------------------------------------------------------------------
class Item {
protected:
    VMWCHAR label[32];
    int width;
    int height;
    bool focusable;

public:
    Item(const char* aLabel) : width(0), height(20), focusable(false) {
        label[0] = 0;
        if (aLabel) {
            vm_ascii_to_ucs2(label, sizeof(label), (VMSTR)aLabel);
        }
    }
    virtual ~Item() {}

    virtual void paint(Graphics& g, int w, int h, bool isFocused) = 0;
    virtual int getPreferredHeight(int w) { return height; }
    virtual bool isFocusable() const { return focusable; }
    virtual void keyPressed(int keyCode) {}
    virtual void pointerPressed(int x, int y) {}
};

// ----------------------------------------------------------------------------
// StringItem
// ----------------------------------------------------------------------------
class StringItem : public Item {
private:
    VMWCHAR text[128];

public:
    StringItem(const char* aLabel, const char* aText) : Item(aLabel) {
        focusable = false;
        setText(aText);
    }

    void setText(const char* aText) {
        if (aText) {
            vm_ascii_to_ucs2(text, sizeof(text), (VMSTR)aText);
        } else {
            text[0] = 0;
        }
    }

    virtual int getPreferredHeight(int w) {
        int lines = 1;
        if (label[0] != 0) lines++;
        return lines * FONT_HEIGHT + 4;
    }

    virtual void paint(Graphics& g, int w, int h, bool isFocused) {
        int y = 2;
        if (label[0] != 0) {
            g.setColor(120, 120, 120);
            g.drawString(label, 4, y);
            y += FONT_HEIGHT;
        }
        g.setColor(0, 0, 0);
        g.drawString(text, 4, y);
    }
};

// ----------------------------------------------------------------------------
// Gauge Item (Progress & Interactive)
// ----------------------------------------------------------------------------
class Gauge : public Item {
private:
    int maxValue;
    int curValue;
    bool interactive;

public:
    Gauge(const char* aLabel, bool aInteractive, int aMax, int aInit)
        : Item(aLabel), interactive(aInteractive), maxValue(aMax), curValue(aInit) {
        focusable = aInteractive;
        height = 36;
    }

    int getValue() const { return curValue; }
    void setValue(int val) {
        curValue = (val < 0) ? 0 : (val > maxValue ? maxValue : val);
    }

    virtual void keyPressed(int keyCode) {
        if (!interactive) return;
        if (keyCode == VM_KEY_LEFT) {
            setValue(curValue - 1);
        } else if (keyCode == VM_KEY_RIGHT) {
            setValue(curValue + 1);
        }
    }

    virtual void pointerPressed(int x, int y) {
        if (!interactive) return;
        // Map pointer position inside the bar
        int barWidth = vm_graphic_get_screen_width() - 20;
        int hitX = x - 10;
        if (hitX >= 0 && hitX <= barWidth) {
            setValue((hitX * maxValue) / barWidth);
        }
    }

    virtual void paint(Graphics& g, int w, int h, bool isFocused) {
        g.setColor(100, 100, 100);
        g.drawString(label, 4, 2);

        int barX = 10;
        int barY = 20;
        int barW = w - 20;
        int barH = 12;

        // Border & Focus outline
        if (isFocused) {
            g.setColor(255, 140, 0); // Focus highlight
            g.drawRect(barX - 2, barY - 2, barW + 4, barH + 4);
        }
        g.setColor(180, 180, 180);
        g.drawRect(barX, barY, barW, barH);

        // Fill progress
        int fillW = (maxValue > 0) ? (curValue * (barW - 2)) / maxValue : 0;
        g.setColor(0, 120, 215);
        g.fillRect(barX + 1, barY + 1, fillW, barH - 2);
    }
};

// ----------------------------------------------------------------------------
// TextField Item
// ----------------------------------------------------------------------------
class TextField : public Item {
public:
    VMWCHAR text[128];
    int maxSize;

    TextField(const char* aLabel, const char* aInitialText, int aMaxSize)
        : Item(aLabel), maxSize(aMaxSize) {
        focusable = true;
        height = 40;
        if (aInitialText) {
            vm_ascii_to_ucs2(text, sizeof(text), (VMSTR)aInitialText);
        } else {
            text[0] = 0;
        }
    }

    void handleNativeInput();

    virtual void keyPressed(int keyCode) {
        if (keyCode == VM_KEY_OK) {
            handleNativeInput();
        }
    }

    virtual void pointerPressed(int x, int y) {
        handleNativeInput();
    }

    virtual void paint(Graphics& g, int w, int h, bool isFocused) {
        g.setColor(120, 120, 120);
        g.drawString(label, 4, 2);

        int boxY = 18;
        int boxH = 20;
        int boxW = w - 10;

        if (isFocused) {
            g.setColor(0, 120, 215);
            g.drawRect(4, boxY, boxW, boxH);
            g.drawRect(3, boxY - 1, boxW + 2, boxH + 2);
        } else {
            g.setColor(180, 180, 180);
            g.drawRect(4, boxY, boxW, boxH);
        }

        g.setColor(255, 255, 255);
        g.fillRect(5, boxY + 1, boxW - 2, boxH - 2);

        g.setColor(0, 0, 0);
        g.drawString(text, 8, boxY + 2);
    }
};

// ----------------------------------------------------------------------------
// CustomItem (MIDP 2.0 User Defined Component)
// ----------------------------------------------------------------------------
class CustomItem : public Item {
public:
    CustomItem(const char* aLabel) : Item(aLabel) {
        focusable = true;
    }

    virtual int getMinContentWidth() = 0;
    virtual int getMinContentHeight() = 0;
    virtual int getPrefContentWidth(int height) = 0;
    virtual int getPrefContentHeight(int width) = 0;

    virtual int getPreferredHeight(int w) {
        return getPrefContentHeight(w) + (label[0] != 0 ? FONT_HEIGHT : 0);
    }

    virtual void paint(Graphics& g, int w, int h, bool isFocused) {
        int yOffset = 0;
        if (label[0] != 0) {
            g.setColor(100, 100, 100);
            g.drawString(label, 4, 0);
            yOffset = FONT_HEIGHT;
        }

        if (isFocused) {
            g.setColor(255, 165, 0);
            g.drawRect(0, yOffset, w - 1, h - yOffset - 1);
        }

        g.translate(0, yOffset);
        paintContent(g, w, h - yOffset);
        g.translate(0, -yOffset);
    }

    virtual void paintContent(Graphics& g, int w, int h) = 0;
};

// ----------------------------------------------------------------------------
// Displayable & Screen Hierarchy
// ----------------------------------------------------------------------------
class Displayable {
protected:
    VMWCHAR title[32];
    Command* commands[MAX_COMMANDS];
    int commandCount;
    CommandListener* listener;

public:
    Displayable(const char* aTitle) : commandCount(0), listener(NULL) {
        vm_ascii_to_ucs2(title, sizeof(title), (VMSTR)aTitle);
    }
    virtual ~Displayable() {}

    void setCommandListener(CommandListener* l) { listener = l; }

    void addCommand(Command* cmd) {
        if (commandCount < MAX_COMMANDS) {
            commands[commandCount++] = cmd;
        }
    }

    virtual void paint(Graphics& g) = 0;
    virtual void keyPressed(int keyCode) = 0;
    virtual void pointerPressed(int x, int y) = 0;
    virtual void pointerDragged(int x, int y) {}
    virtual void pointerReleased(int x, int y) {}

    void triggerCommand(int slot) {
        if (slot < commandCount && listener && commands[slot]) {
            listener->commandAction(commands[slot], this);
        }
    }
};

// ----------------------------------------------------------------------------
// Form Component
// ----------------------------------------------------------------------------
class Form : public Displayable {
private:
    Item* items[MAX_ITEMS];
    int itemCount;
    int focusedIndex;
    int scrollY;
    int contentHeight;

public:
    Form(const char* aTitle);
    virtual ~Form() {}

    int append(Item* item);
    virtual void paint(Graphics& g);
    virtual void keyPressed(int keyCode);
    virtual void pointerPressed(int x, int y);

    void invalidate();
};

// ----------------------------------------------------------------------------
// Display (Engine Core & Native Callback Manager)
// ----------------------------------------------------------------------------
class Display {
private:
    static Display* instance;
    Displayable* current;
    VMINT layer_handle;

public:
    static Display* getDisplay() {
        if (!instance) instance = new Display();
        return instance;
    }

    void setCurrent(Displayable* next) {
        current = next;
        repaint();
    }

    Displayable* getCurrent() { return current; }

    void init();
    void repaint();
    void handleKeyEvent(VMINT event, VMINT keycode);
    void handlePenEvent(VMINT event, VMINT x, VMINT y);
};

#endif // _MRE_LCDUI_H_