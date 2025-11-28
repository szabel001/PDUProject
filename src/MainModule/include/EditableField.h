#ifndef EDITABLE_FIELD_H
#define EDITABLE_FIELD_H

class EditableField {
public:
    EditableField() {}

    EditableField(const String& initialValue, const String& allowedChars)
      : buffer(initialValue), allowed(allowedChars), cursorPos(0) {}

    void begin(const String& initialValue, const String& allowedChars) {
        buffer = initialValue;
        allowed = allowedChars;
        cursorPos = 0;
    }

    bool moveRight() {
        if (cursorPos < buffer.length() - 1) { cursorPos++; return true; }
        return false;
    }

    bool moveLeft() {
        if (cursorPos > 0) { cursorPos--; return true; }
        return false;
    }

    void nextChar() {
        char c = buffer[cursorPos];
        int idx = allowed.indexOf(c);
        if (idx < 0) idx = 0;
        idx = (idx + 1) % allowed.length();
        buffer[cursorPos] = allowed[idx];
    }

    void prevChar() {
        char c = buffer[cursorPos];
        int idx = allowed.indexOf(c);
        if (idx < 0) idx = 0;
        idx = (idx - 1 + allowed.length()) % allowed.length();
        buffer[cursorPos] = allowed[idx];
    }

    const String& get() const { return buffer; }
    int getCursor() const { return cursorPos; }

private:
    String buffer;
    String allowed;
    int cursorPos = 0;
};

#endif
