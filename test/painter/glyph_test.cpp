#include <cppurses/painter/attribute.hpp>
#include <cppurses/painter/brush.hpp>
#include <cppurses/painter/color.hpp>
#include <cppurses/painter/glyph.hpp>

#include <gtest/gtest.h>

#include <string>

using cppurses::Attribute;
using cppurses::background;
using cppurses::Brush;
using cppurses::Color;
using cppurses::foreground;
using cppurses::Glyph;

std::string foo(const Glyph& g) {
    return g.str();
}

TEST(GlyphTest, StringConstructor) {
    EXPECT_NO_THROW(Glyph("H"));
    EXPECT_NO_THROW(Glyph("5"));
    EXPECT_NO_THROW(Glyph('j'));
    const char cp[] = "₹";
    EXPECT_NO_THROW(Glyph{cp});
    EXPECT_NO_THROW(Glyph("ↈ"));
}

TEST(GlyphTest, StringAndAttributesConstructor) {
    EXPECT_NO_THROW(Glyph("#", Attribute::Bold));
    EXPECT_NO_THROW(Glyph("1", background(Color::Blue), Attribute::Underline,
                          Attribute::Bold, Attribute::Underline));
    EXPECT_NO_THROW(Glyph("ޒ", Attribute::Standout));
}

TEST(GlyphTest, Symbol) {
    Glyph glf_1("H");
    Glyph glf_2("ↈ");
    Glyph glf_3("6");

    EXPECT_EQ("H", glf_1.str());
    EXPECT_EQ("ↈ", glf_2.str());
    EXPECT_EQ("6", glf_3.str());
}

TEST(GlyphTest, SetSymbol) {
    Glyph g1 = "ੴ";
    Glyph g2 = {"ആ", Attribute::Italic, background(Color::Green)};
    Glyph g3{"g", foreground(Color::Red), Attribute::Dim};

    EXPECT_EQ("ੴ", g1.str());
    EXPECT_EQ("ആ", g2.str());
    EXPECT_EQ("g", g3.str());

    g1.set_symbol(" ");
    g2.set_symbol("ᆅ");
    g3.set_symbol("ᛥ");

    EXPECT_EQ(" ", g1.str());
    EXPECT_EQ("ᆅ", g2.str());
    EXPECT_EQ("ᛥ", g3.str());
}

TEST(GlyphTest, SetBrush) {
    Glyph g1{"ៀ"};
    Glyph g2("ᯣ", Attribute::Italic, background(Color::White));

    EXPECT_EQ(Brush{}, g1.brush());
    EXPECT_EQ((Brush{Attribute::Italic, background(Color::White)}), g2.brush());

    g2.brush().set_foreground(Color::Yellow);
    g2.brush().add_attributes(Attribute::Standout);

    EXPECT_EQ((Brush{Attribute::Standout, Attribute::Italic,
                     background(Color::White), foreground(Color::Yellow)}),
              g2.brush());
}
