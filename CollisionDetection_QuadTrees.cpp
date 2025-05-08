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
    using Storage = std::list<std::pair<T, def::rectf>>;

    struct ItemLocation
    {
        typename Storage::iterator iterator;
        Storage* container = nullptr;
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

    ItemLocation insert(const T& item, const def::rectf& area)
    {
        for (size_t i = 0; i < 4; i++)
        {
            if (def::contains(m_ChildrenAreas[i], area))
            {
                if (!m_Children[i])
                    m_Children[i] = std::make_shared<QuadTree<T>>(m_ChildrenAreas[i], m_Level + 1);

                return m_Children[i]->insert(item, area);
            }
        }

        // It fits within the area of the current child
        // but it doesn't fit within any area of the children
        // so stop here
        m_Items.push_back({ item, area });

        return { std::prev(m_Items.end()), &m_Items };
    }

    void find(const def::rectf& area, std::list<T>& data)
    {
        for (const auto& item : m_Items)
        {
            if (overlaps(area, item.second))
                data.push_back(item.first);
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
                               [&](const std::pair<T, def::rectf>& i) { return i.first == item; });

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
            items.push_back(item.first);

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
    Storage m_Items;
};

template <class T>
class QuadTreeContainer
{
public:
    struct Item
    {
        T data;
        typename QuadTree<typename std::list<Item>::iterator>::ItemLocation location;
    };

    using Storage = std::list<Item>;

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
        m_Items.push_back({ item });
        m_Items.back().location = m_Root.insert(std::prev(m_Items.end()), area);
    }

    void find(const def::rectf& area, std::list<typename Storage::iterator>& data)
    {
        m_Root.find(area, data);
    }

    void remove(typename Storage::iterator item)
    {
        item->location.container->erase(item->location.iterator);
        m_Items.erase(item);
    }

    void collect_items(Storage& items)
    {
        m_Root.collect_items(items);
    }

    void collect_areas(std::list<def::rectf>& areas)
    {
        m_Root.collect_areas(areas);
    }

private:
    Storage m_Items;
    QuadTree<typename Storage::iterator> m_Root;

};

class App : public def::GameEngine
{
public:
    App()
    {
        GetWindow()->SetTitle("Quad trees");
        UseOnlyTextures(true);
    }

    enum class PlantID
    {
        LargeTree,
        LargeBush,
        SmallTree,
        SmallBush
    };

    struct Object
    {
        def::rectf area;
        PlantID id;
    };

    QuadTreeContainer<Object> tree;

    def::AffineTransforms at;

    float worldSize = 25000.0f;
    float searchAreaSize = 100.0f;

    def::Graphic plants;

protected:
    bool OnUserCreate() override
    {
        tree.create({ {0.0f, 0.0f}, {worldSize, worldSize} });
        at.SetViewArea(GetWindow()->GetScreenSize());

        auto rand_float = [](float min, float max)
            {
                return min + (float)rand() / (float)RAND_MAX * (max - min);
            };

        for (size_t i = 0; i < 1000000; i++)
        {
            Object o;

            o.area.pos = { rand_float(0.0f, worldSize), rand_float(0.0f, worldSize) };
            o.id = PlantID(rand() % 4);

            o.area.size.x = 16.0f;
            
            if (o.id == PlantID::LargeTree || o.id == PlantID::LargeBush)
                o.area.size.y = 32.0f;
            else
                o.area.size.x = 16.0f;

            tree.insert(o, o.area);
        }

        plants.Load("plants.png");

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
            std::list<std::list<QuadTreeContainer<Object>::Item>::iterator> selected;
            tree.find(selectedArea, selected);

            for (auto& item : selected)
                tree.remove(item);
        }

        def::Vector2f origin = at.GetOrigin();
        def::Vector2f size = at.GetEnd() - origin;

        def::rectf searchArea = { { origin.x, origin.y }, { size.x, size.y } };

        std::list<std::list<QuadTreeContainer<Object>::Item>::iterator> objects;
        tree.find(searchArea, objects);

        ClearTexture(def::GREEN);

        for (const auto& obj : objects)
        {
            def::Vector2f pos = { obj->data.area.pos.x, obj->data.area.pos.y };

            switch (obj->data.id)
            {
            case PlantID::LargeTree: at.DrawPartialTexture(pos, plants.texture, { 0.0f, 0.0f }, { 16.0f, 32.0f }); break;
            case PlantID::LargeBush: at.DrawPartialTexture(pos, plants.texture, { 16.0f, 0.0f }, { 16.0f, 32.0f }); break;
            case PlantID::SmallTree: at.DrawPartialTexture(pos, plants.texture, { 32.0f, 7.0f }, { 16.0f, 25.0f }); break;
            case PlantID::SmallBush: at.DrawPartialTexture(pos, plants.texture, { 48.0f, 16.0f }, { 16.0f, 16.0f }); break;
            }
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