#include "defGameEngine.hpp"

constexpr int BIG_GEAR_RADIUS = 100;
constexpr int SMALL_GEAR_RADIUS = 20;

class Spirographs : public def::GameEngine
{
public:
    Spirographs()
    {
        GetWindow()->SetTitle("Spirographs!");
    }

    def::Vector2i bigGearPos;
    float smallGearAngle = 0.0f;
    float penAngle = 0.0f;
    float speed = 10.0f;

    def::Pixel* pathScene = nullptr;

protected:
    bool OnUserCreate() override
    {
        bigGearPos = GetWindow()->GetScreenSize() / 2;

        const auto ss = GetWindow()->GetScreenSize();
        pathScene = new def::Pixel[ss.x * ss.y];

        for (int y = 0; y < ss.y; y++)
            for (int x = 0; x < ss.x; x++)
                pathScene[y * ss.x + x] = def::NONE;

        return true;
    }

    bool OnUserUpdate(float deltaTime) override
    {
        def::Vector2i smallGearPos = bigGearPos + def::Vector2f(BIG_GEAR_RADIUS - SMALL_GEAR_RADIUS, smallGearAngle).Cartesian();
        def::Vector2i penPos = smallGearPos + def::Vector2f(SMALL_GEAR_RADIUS * 2 / 3, penAngle).Cartesian();
        
        // Change the speeds of changing the angles
        smallGearAngle -= deltaTime * 0.1f * speed;
        penAngle -= deltaTime * 10.f * speed;

        Clear(def::BLACK);

        DrawCircle(bigGearPos, BIG_GEAR_RADIUS);
        DrawCircle(smallGearPos, SMALL_GEAR_RADIUS);

        const auto ss = GetWindow()->GetScreenSize();
        pathScene[penPos.y * ss.x + penPos.x] = def::Pixel(fabs(sinf(penAngle)) * 0xFFFFFFFF);
        
        for (int y = 0; y < ss.y; y++)
            for (int x = 0; x < ss.x; x++)
            {
                if (pathScene[y * ss.x + x] != def::NONE)
                    Draw(x, y, pathScene[y * ss.x + x]);
            }

        return true;
    }
};

int main()
{
    Spirographs app;

    if (app.Construct(256, 240, 4, 4))
        app.Run();
}
