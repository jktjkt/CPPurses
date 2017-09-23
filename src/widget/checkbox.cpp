#include "widget/widgets/checkbox.hpp"
#include "painter/glyph_string.hpp"
#include "painter/painter.hpp"
#include "widget/size_policy.hpp"

namespace cppurses {

Checkbox::Checkbox(Glyph_string title, int dist) : title_{title}, dist_{dist} {
    this->height_policy.type(Size_policy::Fixed);
    this->height_policy.hint(1);
    this->Checkbox::initialize();
}

void Checkbox::initialize() {
    toggle = [this] { this->toggle_(); };
    toggle.track(this->destroyed);
    check = [this] {
        if (!checked_) {
            this->toggle_();
        }
    };
    check.track(this->destroyed);
    uncheck = [this] {
        if (checked_) {
            this->toggle_();
        }
    };
    uncheck.track(this->destroyed);
}

bool Checkbox::paint_event() {
    Painter p{this};
    if (checked_) {
        p.put(checked_box_);
    } else {
        p.put(empty_box_);
    }
    p.put(title_, dist_, 0);
    return Widget::paint_event();
}

bool Checkbox::mouse_press_event(Mouse_button button,
                                 std::size_t global_x,
                                 std::size_t global_y,
                                 std::size_t local_x,
                                 std::size_t local_y,
                                 std::uint8_t device_id) {
    if (button == Mouse_button::Left) {
        this->toggle_();
        this->update();
    }
    return true;
}

void Checkbox::toggle_() {
    checked_ = !checked_;
    toggled();
    checked_ ? checked() : unchecked();
}

}  // namespace cppurses
