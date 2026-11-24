#pragma once
#include "PRS/UI/Types.h"

namespace PRS::UI
{
// Metrics are authored in density-independent pixels (dp) and multiplied by
// Density so the same layout looks right on a 2x or 3x phone screen.
struct Theme
{
    f32 density = 1.0f;

    // Layout
    f32 minTouchTarget = 48.0f; // dp - Android accessibility minimum
    f32 padding = 16.0f;        // dp - view padding
    f32 gap = 12.0f;            // dp - space between rows
    f32 radius = 14.0f;         // dp - corner radius
    f32 fontSize = 14.0f;       // dp - body text
    f32 headingSize = 22.0f;    // dp
    f32 titleSize = 30.0f;      // dp
    f32 hairline = 1.0f;        // dp

    // Palette (dark, high contrast for outdoor phone use)
    Color background = Color::FromBytes(11, 13, 18);
    Color backgroundAlt = Color::FromBytes(17, 20, 27);
    Color surface = Color::FromBytes(24, 28, 37);
    Color surfacePressed = Color::FromBytes(38, 44, 57);
    Color primary = Color::FromBytes(88, 132, 255);
    Color primaryPressed = Color::FromBytes(64, 106, 226);
    Color onPrimary = Color::FromBytes(255, 255, 255);
    Color text = Color::FromBytes(238, 241, 248);
    Color textDim = Color::FromBytes(152, 161, 179);
    Color border = Color::FromBytes(45, 51, 64);
    Color success = Color::FromBytes(74, 200, 145);
    Color warning = Color::FromBytes(240, 184, 84);

    f32 Dp(f32 value) const { return value * density; }
    f32 FontPx() const { return Dp(fontSize); }
    f32 HeadingPx() const { return Dp(headingSize); }
    f32 TitlePx() const { return Dp(titleSize); }
    f32 TouchTargetPx() const { return Dp(minTouchTarget); }
    f32 RadiusPx() const { return Dp(radius); }
    f32 PaddingPx() const { return Dp(padding); }
    f32 GapPx() const { return Dp(gap); }

    static Theme PhoneDark(f32 density = 3.0f)
    {
        Theme t;
        t.density = density;
        return t;
    }

    static Theme TabletDark(f32 density = 2.0f)
    {
        Theme t = PhoneDark(density);
        t.padding = 24.0f;
        t.gap = 16.0f;
        return t;
    }
};
} // namespace PRS::UI
