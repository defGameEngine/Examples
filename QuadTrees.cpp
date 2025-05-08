#include "defGameEngine.hpp"

#define DEF_GEOMETRY2D_IMPL
#include "../Include/defGeometry2D.hpp"

#define DGE_AFFINE_TRANSFORMS
#include "../Extensions/DGE_AffineTransforms.hpp"

#include <array>

bool overlaps(const def::rectf& r1, const def::rectf& r2)
{
    return r1.pos < r2.pos + r2.size && r1.pos + r1.size >= r2.pos;
}

template <class T>
class QuadTree
{
public:
    struct Item
    {
        def::rectf area;
        T data;
    };

public:
    QuadTree(const def::rectf& area = { {0.0f, 0.0f}, {128.0f, 128.0f} }, size_t level = 0)
    {
        create(area, level);
    }

    void create(const def::rectf& area, size_t level = 0)
    {
        m_Level = level;
        resize(area);
    }

    void resize(const def::rectf& area)
    {
        clear();

        m_Area = area;
        def::vf2d childSize = area.size * 0.5f;

        m_ChildrenAreas =
        {
            def::rectf(area.pos, childSize),
            def::rectf({ area.pos.x + childSize.x, area.pos.y }, childSize),
            def::rectf({ area.pos.x, area.pos.y + childSize.y }, childSize),
            def::rectf(area.pos + childSize, childSize)
        };
    }

    void clear()
    {
        m_Items.clear();

        for (auto& child : m_Children)
        {
            if (child) child->clear();
            child.reset();
        }
    }

    size_t size() const
    {
        size_t count = m_Items.size();

        for (auto& child : m_Children)
        {
            if (child)
                count += child.size();
        }

        return count;
    }

    void insert(const T& item, const def::rectf& area)
    {
        for (size_t i = 0; i < 4; i++)
        {
            if (def::contains(m_ChildrenAreas[i], area))
            {
                if (!m_Children[i])
                    m_Children[i] = std::make_shared<QuadTree<T>>(m_ChildrenAreas[i], m_Level + 1);

                m_Children[i]->insert(item, area);
                return;
            }
        }

        // It fits within the area of the current child
        // but it doesn't fit within any area of the children
        // so stop here
        m_Items.push_back({ area, item });
    }

    void find(const def::rectf& area, std::list<T>& data)
    {
        for (const auto& item : m_Items)
        {
            if (overlaps(area, item.area))
                data.push_back(item.data);
        }

        for (size_t i = 0; i < 4; i++)
        {
            if (m_Children[i])
            {
                if (def::contains(area, m_ChildrenAreas[i]))
                    m_Children[i]->collect_items(data);

                else if (overlaps(m_ChildrenAreas[i], area))
                    m_Children[i]->find(area, data);
            }
        }
    }

    bool remove(const T& item)
    {
        auto it = std::find_if(m_Items.begin(), m_Items.end(),
            [&](const Item& i) { return i.data == item; });

        if (it != m_Items.end())
        {
            m_Items.erase(it);
            return true;
        }

        for (size_t i = 0; i < 4; i++)
        {
            if (m_Children[i])
            {
                if (m_Children[i]->remove(item))
                    return true;
            }
        }

        return false;
    }

    void collect_items(std::list<T>& items)
    {
        for (const auto& item : m_Items)
            items.push_back(item.data);

        for (auto& child : m_Children)
        {
            if (child)
                 child->collect_items(items);
        }
    }

    void collect_areas(std::list<def::rectf>& areas)
    {
        areas.push_back(m_Area);

        for (size_t i = 0; i < 4; i++)
        {
            if (m_Children[i])
                m_Children[i]->collect_areas(areas);
        }
    }

private:
    size_t m_Level;

    def::rectf m_Area;

    // All 4 children of a current quad
    std::array<std::shared_ptr<QuadTree<T>>, 4> m_Children;

    // Cached areas of each item from the m_Children array
    std::array<def::rectf, 4> m_ChildrenAreas;

    // Items in the current quad
    std::vector<Item> m_Items;
};

template <class T>
class QuadTreeContainer
{
public:
    using PointerType = typename std::list<T>::iterator;

    QuadTreeContainer(const def::rectf& area = { {0.0f, 0.0f}, {128.0f, 128.0f} }, size_t level = 0)
    {
        create(area, level);
    }

    void create(const def::rectf& area = { {0.0f, 0.0f}, {128.0f, 128.0f} }, size_t level = 0)
    {
        m_Root.create(area, level);
    }

    void clear()
    {
        m_Root.clear();
    }

    size_t size() const
    {
        return m_Root.size();
    }

    void insert(const T& item, const def::rectf& area)
    {
        m_Items.push_back(item);
        m_Root.insert(std::prev(m_Items.end()), area);
    }

    void find(const def::rectf& area, std::list<PointerType>& data)
    {
        m_Root.find(area, data);
    }

    void remove(PointerType item)
    {
        m_Root.remove(item);
        m_Items.erase(item);
    }

    void collect_items(std::list<PointerType>& items)
    {
        m_Root.collect_items(items);
    }

    void collect_areas(std::list<def::rectf>& areas)
    {
        m_Root.collect_areas(areas);
    }

private:
    std::list<T> m_Items;
    QuadTree<PointerType> m_Root;

};

class App : public def::GameEngine
{
public:
    App()
    {
        GetWindow()->SetTitle("Quad trees");
        UseOnlyTextures(true);
    }

    struct Object
    {
        def::rectf area;
        def::Pixel colour;
    };

    QuadTreeContainer<Object> tree;

    def::AffineTransforms at;

    float worldSize = 10000.0f;
    float searchAreaSize = 100.0f;

protected:
    bool OnUserCreate() override
    {
        tree.create({ {0.0f, 0.0f}, {worldSize, worldSize} });
        at.SetViewArea(GetWindow()->GetScreenSize());

        auto rand_float = [](float min, float max)
            {
                return min + (float)rand() / (float)RAND_MAX * (max - min);
            };

        for (size_t i = 0; i < 10000; i++)
        {
            Object o;

            o.area.pos = { rand_float(0.0f, worldSize), rand_float(0.0f, worldSize) };
            o.area.size = { rand_float(10.0f, 100.0f), rand_float(10.0f, 100.0f) };
            o.colour = def::Pixel(rand() % 256, rand() % 256, rand() % 256);

            tree.insert(o, o.area);
        }

        return true;
    }

    bool OnUserUpdate(float) override
    {
        auto i = GetInput();

        if (i->GetButtonState(def::Button::WHEEL).pressed)
            at.StartPan(GetInput()->GetMousePosition());

        if (i->GetButtonState(def::Button::WHEEL).held)
            at.UpdatePan(GetInput()->GetMousePosition());

        if (i->GetScrollDelta() > 0)
        {
            if (i->GetKeyState(def::Key::LEFT_CONTROL).held)
                searchAreaSize *= 0.9f;
            else
                at.Zoom(1.1f, i->GetMousePosition());
        }

        if (i->GetScrollDelta() < 0)
        {
            if (i->GetKeyState(def::Key::LEFT_CONTROL).held)
                searchAreaSize *= 1.1f;
            else
                at.Zoom(0.9f, i->GetMousePosition());
        }

        def::Vector2f mouse = at.ScreenToWorld(i->GetMousePosition());
        def::rectf selectedArea({ mouse.x - searchAreaSize * 0.5f, mouse.y - searchAreaSize * 0.5f }, { searchAreaSize, searchAreaSize });

        if (i->GetButtonState(def::Button::LEFT).held)
        {
            std::list<QuadTreeContainer<Object>::PointerType> selected;
            tree.find(selectedArea, selected);

            for (auto& item : selected)
                tree.remove(item);
        }

        def::Vector2f origin = at.GetOrigin();
        def::Vector2f size = at.GetEnd() - origin;

        def::rectf searchArea = { { origin.x, origin.y }, { size.x, size.y } };

        std::list<QuadTreeContainer<Object>::PointerType> objects;
        tree.find(searchArea, objects);

        ClearTexture(def::BLACK);

        for (const auto& obj : objects)
        {
            at.FillTextureRectangle(
                { obj->area.pos.x, obj->area.pos.y },
                { obj->area.size.x, obj->area.size.y },
                obj->colour);
        }

        DrawTextureString({ 0, 0 }, std::to_string(objects.size()));
        at.FillTextureRectangle(
            { selectedArea.pos.x, selectedArea.pos.y },
            { selectedArea.size.x, selectedArea.size.y },
            def::Pixel(255, 255, 255, 100));
        
        return true;
    }
};

int main()
{
    App app;

    if (app.Construct(1280, 960, 1, 1, false, true))
        app.Run();

    return 0;
}
