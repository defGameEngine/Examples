#include "defGameEngine.hpp"

// L(x) = sum(k=0..n, y_k * l_k(x))
// l_k(x) = prod(j=0..n, k!=j, (x-x_j)/(x_k-x_j))

float Lagrange_Basis(float x, int k, const std::vector<def::Vector2f>& points)
{
    float prod = 1.0f;

    for (int j = 0; j < points.size(); j++)
    {
        if (k != j)
            prod *= (x - points[j].x) / (points[k].x - points[j].x);
    }

    return prod;
}

float Lagrange_Polynome(float x, const std::vector<def::Vector2f>& points)
{
    float sum = 0.0f;

    for (int k = 0; k < points.size(); k++)
        sum += points[k].y * Lagrange_Basis(x, k, points);

    return sum;
}

class App : public def::GameEngine
{
public:
    App()
    {
        Window().SetTitle("Lagrange");
    }

private:
    std::vector<def::Vector2f> m_Points;
    std::vector<def::Vector2i> m_Polynome;

    def::Vector2f* m_Control = nullptr;

protected:
    int SearchPoint(const def::Vector2i& pos)
    {
        for (int i = 0; i < m_Points.size(); i++)
        {
            if (def::Vector2i(m_Points[i]) == pos)
                return i;
        }

        return -1;
    }

    void UpdatePolynome()
    {
        std::ranges::sort(m_Points, [](def::Vector2f& p1, def::Vector2f& p2) { return p1.x < p2.x; });

        int start = m_Points.front().x;
        int end = m_Points.back().x;

        m_Polynome.clear();

        int i = 0;
        for (int x = start; x <= end; x++, i++)
        {
            int y = Lagrange_Polynome(x, m_Points);
            m_Polynome.push_back({ x, y });
        }
    }

    bool OnUserCreate() override
    {
        return true;
    }

    bool OnUserUpdate(float deltaTime) override
    {
        auto inp = GetInput();

        if (inp->GetButtonState(def::Button::LEFT).pressed)
        {
            m_Points.push_back(inp->GetMousePosition());

            if (m_Points.size() >= 2)
                UpdatePolynome();
        }

        if (inp->GetKeyState(def::Key::LEFT_SHIFT).pressed)
        {
            int i = SearchPoint(inp->GetMousePosition());

            if (i != -1)
            {
                m_Control = &m_Points[i]; // dangerous but OK!
                UpdatePolynome();
            }
        }

        if (inp->GetKeyState(def::Key::LEFT_SHIFT).held)
        {
            if (m_Control)
            {
                *m_Control = inp->GetMousePosition();
                UpdatePolynome();
            }
        }

        if (inp->GetKeyState(def::Key::LEFT_SHIFT).released)
            m_Control = nullptr;

        Clear(def::BLACK);

        if (m_Polynome.size() >= 2)
        {
            for (int i = 1; i < m_Polynome.size(); i++)
                DrawLine(m_Polynome[i - 1], m_Polynome[i], def::WHITE);
        }

        for (const auto& p : m_Points)
            Draw(p, def::RED);

        return true;
    }
};

int main()
{
    App app;

    if (app.Construct(160, 100, 8, 8))
        app.Run();
}
