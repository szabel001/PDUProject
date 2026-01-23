#ifndef EDITABLE_FIELD_H
#define EDITABLE_FIELD_H

class EditableField {
public:
    EditableField(){}

    EditableField(int initialValue)
      : buffer(initialValue) {}

    void begin(int initialValue) {
        buffer = initialValue;
    }

    void nextInt() { buffer++; }

    void prevInt() { if (buffer > 0) buffer--; }

    String get() const { return String(buffer); }

private:
    int buffer;
};

#endif
