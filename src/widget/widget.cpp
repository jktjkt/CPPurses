#include "widget/widget.hpp"
#include "painter/brush.hpp"
#include "painter/color.hpp"
#include "painter/painter.hpp"
#include "system/events/child_event.hpp"
#include "system/events/clear_screen_event.hpp"
#include "system/events/close_event.hpp"
#include "system/events/enable_event.hpp"
#include "system/events/focus_event.hpp"
#include "system/events/hide_event.hpp"
#include "system/events/key_event.hpp"
#include "system/events/mouse_event.hpp"
#include "system/events/move_event.hpp"
#include "system/events/paint_event.hpp"
#include "system/events/resize_event.hpp"
#include "system/events/show_event.hpp"
#include "system/events/deferred_delete_event.hpp"
#include "system/key.hpp"
#include "system/system.hpp"
#include "system/focus.hpp"
#include "widget/border.hpp"
#include "widget/coordinates.hpp"
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace cppurses {

Widget::Widget() {
    this->Widget::initialize();
    this->update();
}

Widget::~Widget() {
    // if (valid_) {
    if (Focus::focus_widget() == this) {
        Focus::clear_focus();
    }
    Widget* parent = this->parent();
    if (parent != nullptr) {
        System::send_event(Child_removed_event{parent, this});
    }
    // }
}

void Widget::initialize() {
    // Slots
    this->delete_later = [this] {
        System::post_event<Deferred_delete_event>(this);
    };
    this->delete_later.track(this->destroyed);

    this->enable = [this] { this->set_enabled(true); };
    this->enable.track(this->destroyed);

    this->disable = [this] { this->set_enabled(false); };
    this->disable.track(this->destroyed);

    close = [this]() { System::post_event<Close_event>(this); };
    close.track(this->destroyed);

    hide = [this]() { this->set_visible(false); };
    hide.track(this->destroyed);

    show = [this]() { this->set_visible(true); };
    show.track(this->destroyed);

    repaint = [this]() { System::send_event(Paint_event{this}); };
    repaint.track(this->destroyed);

    // give_focus = [this]() { this->set_focus(true); };
    // give_focus.track(this->destroyed);

    update_me = [this]() { this->update(); };
    update_me.track(this->destroyed);

    click_me = [this](Mouse_button b, Coordinates c) {
        System::send_event(Mouse_press_event{this, b, this->x() + c.x,
                                             this->y() + c.y, c.x, c.y, 0});
    };
    click_me.track(this->destroyed);

    keypress_me = [this](Key key) {
        System::send_event(Key_press_event{this, key});
    };
    keypress_me.track(this->destroyed);

    set_background = [this](Color c) { this->set_background_(c); };
    set_background.track(this->destroyed);
    set_foreground = [this](Color c) { this->set_foreground_(c); };
    set_foreground.track(this->destroyed);
}

void Widget::set_name(const std::string& name) {
    widget_name_ = name;
    name_changed(name);
}

std::string Widget::name() const {
    return widget_name_;
}

void Widget::set_parent(Widget* parent) {
    parent_ = parent;
}

Widget* Widget::parent() const {
    return parent_;
}

void Widget::add_child(std::unique_ptr<Widget> child) {
    children_.emplace_back(std::move(child));
    children_.back()->set_parent(this);
    System::post_event<Child_added_event>(this, children_.back().get());
}

void Widget::delete_child(Widget* child) {
    auto end_iter = std::end(children_);
    auto at = std::find_if(std::begin(children_), end_iter,
                           [child](auto& c) { return c.get() == child; });
    if (at != end_iter) {
        System::post_event<Child_removed_event>(this, at->get());
        children_.erase(at);
    }
}

bool Widget::has_child(Widget* child) {
    for (const auto& w_ptr : children_) {
        if (w_ptr.get() == child || w_ptr->has_child(child)) {
            return true;
        }
    }
    return false;
}

std::vector<Widget*> Widget::children() const {
    std::vector<Widget*> ret;
    std::transform(std::begin(children_), std::end(children_),
                   std::back_inserter(ret), [](auto& up) { return up.get(); });
    return ret;
}

void Widget::set_x(std::size_t global_x) {
    auto parent = this->parent();
    auto screen_width = System::max_width();
    if (global_x >= screen_width && screen_width > 0) {
        global_x = screen_width - 1;
    } else if (screen_width == 0) {
        global_x = 0;
    }
    if (parent != nullptr) {
        if (global_x >= parent->x()) {
            position_.x = global_x - parent->x();
        } else {
            position_.x = 0;
        }
    } else {
        position_.x = global_x;
    }
}

void Widget::set_y(std::size_t global_y) {
    auto parent = this->parent();
    auto screen_height = System::max_height();
    if (global_y >= screen_height && screen_height > 0) {
        global_y = screen_height - 1;
    } else if (screen_height == 0) {
        global_y = 0;
    }
    if (parent != nullptr) {
        if (global_y >= parent->y()) {
            position_.y = global_y - parent->y();
        } else {
            position_.y = 0;
        }
    } else {
        position_.y = global_y;
    }
}

void Widget::set_background_(Color c) {
    this->brush().set_background(c);
    this->update();
}

void Widget::set_foreground_(Color c) {
    this->brush().set_foreground(c);
    this->update();
}

bool Widget::has_coordinates(std::size_t global_x, std::size_t global_y) {
    if (!this->enabled() || !this->visible()) {
        return false;
    }
    bool within_west =
        global_x >= (this->x() + west_border_offset(this->border));
    bool within_east = global_x < (this->x() + this->width() +
                                   west_border_offset(this->border));
    bool within_north = global_y >= (this->y() + north_border_offset(this->border));
    bool within_south = global_y < (this->y() + this->height() +
                                    north_border_offset(this->border));
    return within_west && within_east && within_north && within_south;
}

void Widget::update() {
    this->clear_screen();
    System::post_event<Paint_event>(this);
}

void Widget::set_brush(Brush brush) {
    default_brush_ = std::move(brush);
    this->update();
}

void Widget::clear_screen() {
    System::post_event<Clear_screen_event>(this);
}

void Widget::move_cursor(Coordinates c) {
    this->move_cursor(c.x, c.y);
}

void Widget::move_cursor(std::size_t x, std::size_t y) {
    this->move_cursor_x(x);
    this->move_cursor_y(y);
}

void Widget::move_cursor_x(std::size_t x) {
    if (x < this->width()) {
        cursor_position_.x = x;
    } else if (this->width() != 0) {
        cursor_position_.x = this->width() - 1;
    }
}

void Widget::move_cursor_y(std::size_t y) {
    if (y < this->height()) {
        cursor_position_.y = y;
    } else if (this->height() != 0) {
        cursor_position_.y = this->height() - 1;
    }
}

bool Widget::move_event(std::size_t new_x,
                        std::size_t new_y,
                        std::size_t old_x,
                        std::size_t old_y) {
    this->set_x(new_x);
    this->set_y(new_y);
    this->update();
    return true;
}

bool Widget::resize_event(std::size_t new_width,
                          std::size_t new_height,
                          std::size_t old_width,
                          std::size_t old_height) {
    width_ = new_width;
    height_ = new_height;
    this->update();
    return true;
}

bool Widget::close_event() {
    this->delete_later();
    return true;
}

bool Widget::focus_in_event() {
    System::paint_engine()->move(this->x() + this->cursor_x(),
                                 this->y() + this->cursor_y());
    focused_in();
    return true;
}

bool Widget::focus_out_event() {
    focused_out();
    return true;
}

bool Widget::paint_event() {
    if (border.enabled) {
        Painter p{this};
        p.border(border);
    }
    // Might not need below if focus widget sets this afterwards, on no focus?
    System::paint_engine()->move(this->x() + this->cursor_x(),
                                 this->y() + this->cursor_y());
    return true;
}

bool Widget::clear_screen_event() {
    Painter p{this};
    p.clear_screen();
    return true;
}

bool Widget::deferred_delete_event(Event_handler* to_delete) {
    this->delete_child(static_cast<Widget*>(to_delete));
    return true;
}

bool Widget::child_added_event(Event_handler* child) {
    this->update();
    return true;
}

bool Widget::child_removed_event(Event_handler* child) {
    this->update();
    return true;
}

bool Widget::child_polished_event(Event_handler* child) {
    this->update();
    return true;
}

void Widget::set_visible(bool visible) {
    this->visible_ = visible;
    if (visible) {
        System::send_event(Show_event{this});
    } else {
        System::send_event(Hide_event{this});
    }
    for (Widget* c : this->children()) {
        c->set_visible(visible);
    }
}

std::size_t Widget::x() const {
    if (this->parent() == nullptr) {
        return this->position_.x;
    }
    Widget* parent = this->parent();
    return this->position_.x + parent->x();
}

std::size_t Widget::y() const {
    if (this->parent() == nullptr) {
        return this->position_.x;
    }
    Widget* parent = this->parent();
    return this->position_.y + parent->y();
}

std::size_t Widget::width() const {
    std::size_t width_border_offset =
        west_border_offset(this->border) + east_border_offset(this->border);
    if (width_border_offset > width_) {
        return 0;
    }
    return width_ - width_border_offset;
}

std::size_t Widget::height() const {
    std::size_t height_border_offset =
        north_border_offset(this->border) + south_border_offset(this->border);
    if (height_border_offset > height_) {
        return 0;
    }
    return height_ - height_border_offset;
}

// - - - - - - - - - - - - - - Free Functions - - - - - - - - - - - - - - - - -

bool has_border(const Widget& w) {
    return w.border.enabled;
}

void enable_border(Widget& w) {
    w.border.enabled = true;
    System::post_event<Child_polished_event>(w.parent(), &w);
}

void disable_border(Widget& w) {
    w.border.enabled = false;
    System::post_event<Child_polished_event>(w.parent(), &w);
}

}  // namespace cppurses
