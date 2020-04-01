/*
    Borealis, a Nintendo Switch UI Library
    Copyright (C) 2019-2020  natinusala
    Copyright (C) 2019  p-sam

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <borealis/application.hpp>
#include <borealis/label.hpp>

namespace brls
{

Label::Label(LabelStyle labelStyle, std::string text, bool multiline)
    : text(text)
    , textTicker(text + "          " + text)
    , multiline(multiline)
    , labelStyle(labelStyle)
{
    this->lineHeight = this->getLineHeight(labelStyle);
    this->fontSize   = this->getFontSize(labelStyle);

    this->parentFocusSubscription = Application::getGlobalFocusChangeEvent()->subscribe([this](View *view) {
        if (view == this->getParent())
            this->onParentFocus();
        else
            this->onParentUnfocus();
    });
}

Label::~Label() {
    menu_animation_ctx_tag tag = (uintptr_t) & this->tickerOffset;
    menu_animation_kill_by_tag(&tag);

    tag = (uintptr_t) & this->textAnimation;
    menu_animation_kill_by_tag(&tag);

    Application::getGlobalFocusChangeEvent()->unsubscribe(this->parentFocusSubscription);
}

void Label::setHorizontalAlign(NVGalign align)
{
    this->horizontalAlign = align;
}

void Label::setVerticalAlign(NVGalign align)
{
    this->verticalAlign = align;
}

void Label::setFontSize(unsigned size)
{
    this->fontSize = size;

    if (this->getParent())
        this->getParent()->invalidate();
}

void Label::setText(std::string text)
{
    this->text       = text;
}

void Label::setStyle(LabelStyle style)
{
    this->labelStyle = style;
}

void Label::layout(NVGcontext* vg, Style* style, FontStash* stash)
{
    nvgSave(vg);
    nvgReset(vg);

    nvgFontSize(vg, this->fontSize);
    nvgTextAlign(vg, this->horizontalAlign | NVG_ALIGN_TOP);
    nvgFontFaceId(vg, this->getFont(stash));
    nvgTextLineHeight(vg, this->lineHeight);

    float bounds[4];

    // Update width or height to text bounds
    if (this->multiline)
    {
        nvgTextBoxBounds(vg, this->x, this->y, this->width, this->text.c_str(), nullptr, bounds);

        this->textHeight = bounds[3] - bounds[1]; // ymax - ymin
    }
    else
    {
        nvgTextBounds(vg, this->x, this->y, this->text.c_str(), nullptr, bounds);

        this->textWidth = bounds[2] - bounds[0]; // xmax - xmin
        this->textHeight = bounds[3] - bounds[1]; // ymax - ymin

        // offset the position to compensate the width change
        // and keep right alignment
        if (this->horizontalAlign == NVG_ALIGN_RIGHT)
            this->x += this->width - this->textWidth;
        nvgTextBounds(vg, this->x, this->y, "…", nullptr, bounds); 
        const unsigned ellipsisWidth = bounds[2] - bounds[0];

        this->textEllipsis = this->text.substr(0, this->text.size() * std::min(1.0F, (static_cast<float>(this->getWidth() - ellipsisWidth) / (this->getTextWidth()))));
        this->textEllipsis += "…";
    }

    if (this->getWidth() == 0)
        this->width = this->textWidth;

    if (this->getHeight() == 0)
        this->height = this->textHeight;

    nvgRestore(vg);
}

void Label::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, Style* style, FrameContext* ctx)
{
    nvgFillColor(vg, a(this->getColor(ctx->theme)));

    // Draw

    nvgFontSize(vg, this->fontSize);
    nvgFontFaceId(vg, this->getFont(ctx->fontStash));

    if (this->multiline)
    {
        nvgTextLineHeight(vg, this->lineHeight);
        nvgTextAlign(vg, this->horizontalAlign | NVG_ALIGN_TOP);
        nvgBeginPath(vg);

        nvgTextBox(vg, x, y, width, this->text.c_str(), nullptr);
    }
    else
    {
        nvgTextLineHeight(vg, 1.0f);
        nvgTextAlign(vg, this->horizontalAlign | this->verticalAlign);
        nvgBeginPath(vg);

        if (this->horizontalAlign == NVG_ALIGN_RIGHT)
            x += width;
        else if (this->horizontalAlign == NVG_ALIGN_CENTER)
            x += width / 2;

        if (this->textWidth == 0) {
            float bounds[4];
            nvgTextBounds(vg, x, y, this->text.c_str(), nullptr, bounds);
            this->textWidth = bounds[2] - bounds[0];
        }

        if (this->tickerActive && this->textTickerWidth == 0 && this->textWidth != 0) {
            float bounds[4];
            nvgTextBounds(vg, x, y, this->textTicker.c_str(), nullptr, bounds);
            this->textTickerWidth = (bounds[2] - bounds[0]) - this->textWidth;
            this->startTickerAnimation();
        }

        if (this->textAnimation < 1.0F) {
            NVGcolor valueColor = a(this->getColor(ctx->theme));
            valueColor.a *= this->textAnimation;
            nvgFillColor(vg, valueColor);
            nvgFontSize(vg, style->List.Item.valueSize * this->textAnimation);

            if (this->textWidth > this->getParent()->getWidth()) {
                if (this->verticalAlign == NVG_ALIGN_BOTTOM)
                    nvgText(vg, x, y + height, this->textEllipsis.c_str(), nullptr);
                else if (this->verticalAlign == NVG_ALIGN_TOP)
                    nvgText(vg, x, y, this->textEllipsis.c_str(), nullptr);
                else
                    nvgText(vg, x, y + height / 2, this->textEllipsis.c_str(), nullptr);
            } else {
                if (this->verticalAlign == NVG_ALIGN_BOTTOM)
                    nvgText(vg, x, y + height, this->text.c_str(), nullptr);
                else if (this->verticalAlign == NVG_ALIGN_TOP)
                    nvgText(vg, x, y, this->text.c_str(), nullptr);
                else
                    nvgText(vg, x, y + height / 2, this->text.c_str(), nullptr);
            }

            return;
        }

        if (this->tickerActive && this->getParent() && this->textWidth > this->getParent()->getWidth())
        {   
            nvgSave(vg);
            nvgIntersectScissor(vg, x, y, width, height);

            if (this->verticalAlign == NVG_ALIGN_BOTTOM)
                nvgText(vg, x - tickerOffset, y + height, this->textTicker.c_str(), nullptr);
            else if (this->verticalAlign == NVG_ALIGN_TOP)
                nvgText(vg, x - tickerOffset, y, this->textTicker.c_str(), nullptr);
            else
                nvgText(vg, x - tickerOffset, y + height / 2, this->textTicker.c_str(), nullptr);   

            nvgRestore(vg);
        } 
        else if (!this->tickerActive && this->getParent() && this->textWidth > this->getParent()->getWidth()) {
            if (this->verticalAlign == NVG_ALIGN_BOTTOM)
                nvgText(vg, x, y + height, this->textEllipsis.c_str(), nullptr);
            else if (this->verticalAlign == NVG_ALIGN_TOP)
                nvgText(vg, x, y, this->textEllipsis.c_str(), nullptr);
            else
                nvgText(vg, x, y + height / 2, this->textEllipsis.c_str(), nullptr);
        }
        else 
        {
            if (this->verticalAlign == NVG_ALIGN_BOTTOM)
                nvgText(vg, x, y + height, this->text.c_str(), nullptr);
            else if (this->verticalAlign == NVG_ALIGN_TOP)
                nvgText(vg, x, y, this->text.c_str(), nullptr);
            else
                nvgText(vg, x, y + height / 2, this->text.c_str(), nullptr);
        }
    }
}

void Label::willAppear()
{

}

void Label::willDisappear()
{
    this->setTickerState(false);
}

void Label::startTickerAnimation()
{
    this->tickerWaitTimerCtx.duration = 1500;
    this->tickerWaitTimerCtx.cb = [&](void *userdata) {
        menu_animation_ctx_tag tag = (uintptr_t) & this->tickerOffset;
        menu_animation_kill_by_tag(&tag);

        this->tickerOffset = 0.0f;

        menu_animation_ctx_entry_t entry;
        entry.cb           = [&](void *userdata) { menu_timer_start(&this->tickerWaitTimer, &this->tickerWaitTimerCtx); };
        entry.duration     = this->textTickerWidth * 15;
        entry.easing_enum  = EASING_LINEAR;
        entry.subject      = &this->tickerOffset;
        entry.tag          = tag;
        entry.target_value = this->textTickerWidth;
        entry.tick         = [](void* userdata) {};
        entry.userdata     = nullptr;

        menu_animation_push(&entry);
    };

    menu_timer_start(&this->tickerWaitTimer, &this->tickerWaitTimerCtx);
}

void Label::stopTickerAnimation()
{      
    menu_animation_ctx_tag tag = (uintptr_t) & this->tickerOffset;
    menu_animation_kill_by_tag(&tag);
    this->tickerOffset = 0.0f;
}


unsigned Label::getTextWidth()
{
    return this->textWidth;
}

unsigned Label::getTextHeight()
{
    return this->textHeight;
}

const std::string& Label::getText() {
    return this->text;
}


void Label::setColor(NVGcolor color)
{
    this->customColor    = color;
    this->useCustomColor = true;
}

void Label::unsetColor()
{
    this->useCustomColor = false;
}

NVGcolor Label::getColor(ThemeValues* theme)
{
    // Use custom color if any
    if (this->useCustomColor)
        return a(this->customColor);

    switch (this->labelStyle)
    {
        case LabelStyle::DESCRIPTION:
            return a(theme->descriptionColor);
        case LabelStyle::CRASH:
            return RGB(255, 255, 255);
        case LabelStyle::BUTTON_PLAIN:
            return a(theme->buttonPlainEnabledTextColor);
        case LabelStyle::BUTTON_PLAIN_DISABLED:
            return a(theme->buttonPlainDisabledTextColor);
        case LabelStyle::NOTIFICATION:
            return a(theme->notificationTextColor);
        case LabelStyle::BUTTON_DIALOG:
            return a(theme->dialogButtonColor);
        case LabelStyle::LIST_ITEM_VALUE:
            return a(theme->listItemValueColor);
        case LabelStyle::LIST_ITEM_VALUE_FAINT:
            return a(theme->listItemFaintValueColor);
        default:
            return a(theme->textColor);
    }
}

void Label::setFont(int font)
{
    this->customFont    = font;
    this->useCustomFont = true;
}

void Label::unsetFont()
{
    this->useCustomFont = false;
}

int Label::getFont(FontStash* stash)
{
    if (this->useCustomFont)
        return this->customFont;

    return stash->regular;
}

void Label::setTickerState(bool active)
{   
    this->tickerActive = active;
}


unsigned Label::getFontSize(LabelStyle labelStyle)
{
    Style* style = Application::getStyle();

    switch (labelStyle)
    {
        case LabelStyle::REGULAR:
            return style->Label.regularFontSize;
        case LabelStyle::MEDIUM:
            return style->Label.mediumFontSize;
        case LabelStyle::SMALL:
            return style->Label.smallFontSize;
        case LabelStyle::DESCRIPTION:
            return style->Label.descriptionFontSize;
        case LabelStyle::CRASH:
            return style->Label.crashFontSize;
        case LabelStyle::BUTTON_PLAIN_DISABLED:
        case LabelStyle::BUTTON_PLAIN:
        case LabelStyle::BUTTON_BORDERLESS:
        case LabelStyle::BUTTON_DIALOG:
            return style->Label.buttonFontSize;
        case LabelStyle::LIST_ITEM:
            return style->Label.listItemFontSize;
        case LabelStyle::NOTIFICATION:
            return style->Label.notificationFontSize;
        case LabelStyle::DIALOG:
            return style->Label.dialogFontSize;
        case LabelStyle::LIST_ITEM_VALUE:
        case LabelStyle::LIST_ITEM_VALUE_FAINT:
            return style->List.Item.valueSize;
        default:
            return 0;
    }
}

float Label::getLineHeight(LabelStyle labelStyle)
{
    Style* style = Application::getStyle();

    switch (labelStyle)
    {
        case LabelStyle::REGULAR:
        case LabelStyle::MEDIUM:
        case LabelStyle::SMALL:
        case LabelStyle::DESCRIPTION:
        case LabelStyle::CRASH:
        case LabelStyle::BUTTON_PLAIN_DISABLED:
        case LabelStyle::BUTTON_PLAIN:
        case LabelStyle::BUTTON_BORDERLESS:
        case LabelStyle::BUTTON_DIALOG:
        case LabelStyle::LIST_ITEM:
        case LabelStyle::DIALOG:
            return style->Label.lineHeight;
        case LabelStyle::NOTIFICATION:
            return style->Label.notificationLineHeight;
        default:
            return 0;
    }
}

void Label::animate(LabelAnimation animation)
{
    Style* style = Application::getStyle();

    menu_animation_ctx_tag tag = (uintptr_t) &this->textAnimation;
    menu_animation_kill_by_tag(&tag);

    this->textAnimation = animation == LabelAnimation::EASE_IN ? 0.0F : 1.0F;

    menu_animation_ctx_entry_t entry;
    entry.cb           = [this](void *userdata) { this->startTickerAnimation(); };
    entry.duration     = style->AnimationDuration.highlight;
    entry.easing_enum  = EASING_IN_OUT_QUAD;
    entry.subject      = &this->textAnimation;
    entry.tag          = tag;
    entry.target_value = animation == LabelAnimation::EASE_IN ? 1.0F : 0.0F;
    entry.tick         = [](void* userdata) {};
    entry.userdata     = nullptr;

    this->stopTickerAnimation();
    menu_animation_push(&entry);
}

void Label::onParentFocus() {
    this->startTickerAnimation();
    this->setTickerState(true);
}

void Label::onParentUnfocus() {
    this->stopTickerAnimation();
    this->setTickerState(false);
}

} // namespace brls
