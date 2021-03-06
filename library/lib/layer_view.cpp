/*
    Borealis, a Nintendo Switch UI Library
    Copyright (C) 2019  WerWolv

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
#include <borealis/layer_view.hpp>

namespace brls
{

LayerView::LayerView()
{
}

LayerView::~LayerView()
{
    for (unsigned int i = 0; i < this->layers.size(); i++)
        delete this->layers[i];

    this->layers.clear();
}

void LayerView::addLayer(View* view)
{
    if (view)
    {
        view->setParent(this);
        this->layers.push_back(view);
    }
}

void LayerView::changeLayer(int index, bool focus)
{
    if (index >= 0 && index < static_cast<int>(this->layers.size()))
    {
        Application::blockInputs();

        if (this->selectedIndex >= 0)
        {
            this->layers[this->selectedIndex]->willDisappear();
            this->layers[this->selectedIndex]->hide([]() {});
        }

        this->selectedIndex = index;
        this->layers[this->selectedIndex]->willAppear();
        this->layers[this->selectedIndex]->show([=]() {
            if (focus)
                Application::requestFocus(this->layers[this->selectedIndex], FocusDirection::NONE);
            Application::unblockInputs();
        });

        this->invalidate();
    }

    if (index == -1)
    {
        if (this->selectedIndex > 0)
        {
            this->layers[this->selectedIndex]->willDisappear();
            this->layers[this->selectedIndex]->hide([]() {});
        }

        this->selectedIndex = index;
    }
}

int LayerView::getLayerIndex()
{
    return this->selectedIndex;
}

View* LayerView::getLayer(unsigned index)
{
    if (index >= this->layers.size())
        return nullptr;
    
    return this->layers[index];
}

View* LayerView::requestFocus(FocusDirection direction, View* oldFocus, bool fromUp)
{
    if (fromUp)
        return View::requestFocus(direction, oldFocus);

    if (this->selectedIndex >= 0 && this->selectedIndex < static_cast<int>(this->layers.size()))
    {
        View* newFocus = this->layers[this->selectedIndex]->requestFocus(direction, oldFocus);
        if (newFocus)
        {
            return newFocus;
        }
    }

    return nullptr;
}

void LayerView::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, Style* style, FrameContext* ctx)
{
    if (this->selectedIndex >= 0 && this->selectedIndex < static_cast<int>(this->layers.size()))
        this->layers[this->selectedIndex]->frame(ctx);
}

void LayerView::layout(NVGcontext* vg, Style* style, FontStash* stash)
{
    if (this->selectedIndex >= 0 && this->selectedIndex < static_cast<int>(this->layers.size()))
    {
        this->layers[this->selectedIndex]->setBoundaries(this->getX(), this->getY(), this->getWidth(), this->getHeight());
        this->layers[this->selectedIndex]->invalidate();
    }
}

void LayerView::willAppear()
{
    if (this->selectedIndex >= 0 && this->selectedIndex < static_cast<int>(this->layers.size()))
        this->layers[this->selectedIndex]->willAppear();
}

void LayerView::willDisappear()
{
    if (this->selectedIndex >= 0 && this->selectedIndex < static_cast<int>(this->layers.size()))
        this->layers[this->selectedIndex]->willDisappear();
}

}